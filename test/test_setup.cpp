// setup: the flow from a scaffolded case directory to a Model ready for runsim
// (setup_dir -> input_verify -> build_model/setup_sim), the validity of that Model, and
// a short headless run of it checked with the whole-run invariants (the sniff-tier
// macro test).
#include "test_support.h"
#include "test_invariants.h"

#include "../src/r0_simulation.h"
#include "../src/random.h"

namespace {

constexpr std::string_view GROUP = "setup";
using test_support::RuntimeNamesGuard;
namespace fs = std::filesystem;

// a scaffolded case (template inputs) builds a Model whose pieces agree with the inputs
void test_use_dir_builds_valid_model() {
  RuntimeNamesGuard guard;
  const fs::path dir = test_support::make_case_dir("setup_valid");
  test_support::require_inputs_verify(dir);
  Model m = [&] {
    test_support::StdoutCapture quiet;
    return use_dir(dir.string());
  }();
  const json cfg = test_support::read_case_json(dir, "config.json");

  CHECK_EQ(m.ndays, cfg["days"].get<int>());
  CHECK_EQ(m.locale, cfg["locale"].get<int>());
  CHECK_EQ(m.case_label, dir.filename().string());
  CHECK_EQ(m.output_dir, dir / "output");
  CHECK_EQ(m.caldays.size(), size_t(m.ndays));
  CHECK_EQ(m.caldays.front(), absl::CivilDay(2020, 1, 1));
  CHECK_EQ(m.indoor_seq.size(), size_t(m.ndays));

  const size_t li = test_support::require_locale_index(m.mp.geodata, m.locale);
  CHECK_EQ(m.pop.popn, size_t(m.mp.geodata.pop[li]));
  // ages: apportioned from age_dist, contiguous blocks
  const auto age_dist = cfg["age_dist"].get<vector<double>>();
  std::array<size_t, 6> ages{};
  for (size_t p = 1; p <= m.pop.popn; ++p) ++ages[m.pop.agegrp[p].v];
  for (size_t a = 1; a <= 5; ++a) CHECK_NEAR(ages[a], m.pop.popn * age_dist[a - 1], 1.0);

  // indoor season 09-15 .. 05-30 gets indoor_uplift, other days 1.0
  int bad_indoor = 0;
  for (size_t d = 0; d < m.caldays.size(); ++d) {
    const auto c = m.caldays[d];
    const bool indoor = c.month() >= 10 || c.month() <= 4 || (c.month() == 9 && c.day() >= 15) ||
                        (c.month() == 5 && c.day() <= 30);
    bad_indoor += std::abs(m.indoor_seq[d] - (indoor ? m.mp.socialdata.indoor_uplift : 1.0f)) > 1e-6f;
  }
  CHECK_MSG(bad_indoor == 0, "{} days have the wrong indoor factor", bad_indoor);

  // runtime names come from the loaded parameters
  CHECK_EQ(Variant::names, m.mp.variant_names);
  CHECK_EQ(Variant::names[1], string("base"));
  CHECK_EQ(Ring::names, (vector<string>{"error", "population"}));
  CHECK(m.ring_members.empty());
  int not_ring1 = 0;
  for (size_t p = 1; p <= m.pop.popn; ++p) not_ring1 += m.pop.ring[p].v != 1;
  CHECK_EQ(not_ring1, 0);
  CHECK_EQ(m.seedcases.size(), test_support::read_case_json(dir, "seed.json").size());
  CHECK(m.sd_cases.empty());
  CHECK(!m.dovax);
  CHECK_EQ(m.mp.vaxset.size(), size_t{0});
  fs::remove_all(dir);
}

// optional features load when switched on in config.json
void test_use_dir_with_rings_vax_and_distancing() {
  RuntimeNamesGuard guard;
  const fs::path dir = test_support::make_case_dir("setup_all_on");
  test_support::edit_config(dir, [](json& c) {
    c["do_rings"] = true;
    c["dovax"] = true;
    c["do_social_distancing"] = true;
  });
  test_support::require_inputs_verify(dir);
  xo::seed(31);
  Model m = [&] {
    test_support::StdoutCapture quiet;
    return use_dir(dir.string());
  }();
  const json rings = test_support::read_case_json(dir, "rings.json")["rings"];
  CHECK(m.dovax && m.do_rings && m.do_social_distancing);
  CHECK_EQ(m.mp.vaxset.size(), test_support::read_case_json(dir, "vaccines.json").size());
  CHECK_EQ(m.mp.vaxschedset.size(), size_t{2});
  CHECK_EQ(m.sd_cases.size(), test_support::read_case_json(dir, "soc_dist.json").size());
  CHECK_EQ(SDCase::names.size(), m.sd_cases.size() + 1);
  REQUIRE(m.mp.ringtraits.ring_count() == rings.size());
  CHECK_EQ(size_t{Ring::count}, rings.size());

  // ring membership partitions the population; lengths match; stratified by age
  REQUIRE(m.ring_members.size() == rings.size() + 1);
  CHECK(m.ring_members[0].empty());
  size_t members = 0;
  int bad = 0;
  for (size_t r = 1; r <= rings.size(); ++r) {
    members += m.ring_members[r].size();
    CHECK_EQ(m.ring_lengths[r], m.ring_members[r].size());
    CHECK(std::ranges::is_sorted(m.ring_members[r]));
    for (size_t p : m.ring_members[r]) bad += m.pop.ring[p].v != r;
    const double pct = rings[r - 1]["pct_of_population"].get<double>();
    for (uint8_t a = 1; a <= 5; ++a) {
      size_t in_age = 0, in_ring = 0;
      for (size_t p = 1; p <= m.pop.popn; ++p)
        if (m.pop.agegrp[p].v == a) { ++in_age; in_ring += m.pop.ring[p].v == r; }
      CHECK_MSG(std::abs(double(in_ring) - pct * in_age) <= 1.0, "ring {} age {}: {} of {} (pct {})", r,
                Agegrp::names[a], in_ring, in_age, pct);
    }
  }
  CHECK_EQ(members, m.pop.popn);
  CHECK_MSG(bad == 0, "{} ring members have a different ring in PopData", bad);
  fs::remove_all(dir);
}

// setup_sim reports bad configuration with exceptions
void test_setup_sim_rejects_bad_config() {
  RuntimeNamesGuard guard;
  auto quiet_setup = [](Config c) {
    test_support::StdoutCapture quiet;
    return setup_sim(c);
  };
  Config c = test_support::sample_config();
  c.locale = 1;
  CHECK_THROWS(std::runtime_error, (void)quiet_setup(c), "Invalid locale input");
  c = test_support::sample_config();
  c.do_social_distancing = true;
  c.social_dist = "";
  CHECK_THROWS(std::runtime_error, (void)quiet_setup(c), "do_social_distancing is true");
  c = test_support::sample_config();
  c.variants = "/no/such/variants.json";
  CHECK_THROWS(std::invalid_argument, (void)quiet_setup(c), "Invalid file path");
  c = test_support::sample_config();
  c.do_rings = true;
  const auto bad = fs::temp_directory_path() / test_support::unique_name("epi_rings_") ;
  test_support::write_file_text(bad / "rings.json", R"({"not_rings": []})");
  c.rings = (bad / "rings.json").string();
  CHECK_THROWS(std::runtime_error, (void)quiet_setup(c), "top-level 'rings'");
  fs::remove_all(bad);

  c = test_support::sample_config();
  c.do_rings = false;
  c.rings = "";
  bool threw = false;
  try { (void)quiet_setup(c); } catch (const std::exception&) { threw = true; }
  CHECK_MSG(!threw, "do_rings=false with no rings path must be allowed (was B5)");
  c.do_rings = true;
  CHECK_THROWS(std::runtime_error, (void)quiet_setup(c), "do_rings is true but rings is not configured");
}

// rings rules: do_rings=true needs a non-empty rings file; with do_rings=false the rings
// file is optional and never read, and every person is in ring 1 = "population"
void test_rings_config_rules() {
  RuntimeNamesGuard guard;
  struct Case {
    const char* name;
    std::function<void(const fs::path&)> edit;
  };
  auto rings_off = [](const fs::path& d) { test_support::edit_config(d, [](json& c) { c["do_rings"] = false; }); };
  const vector<Case> allowed{
      {"off_template_file", rings_off},
      {"off_unreadable_file", [&](const fs::path& d) {
         rings_off(d);
         test_support::write_file_text(d / "input" / "rings.json", "this is not json");
       }},
      {"off_no_tag", [&](const fs::path& d) {
         rings_off(d);
         test_support::edit_config(d, [](json& c) { c.erase("rings"); });
       }},
      {"off_blank_tag", [&](const fs::path& d) {
         rings_off(d);
         test_support::edit_config(d, [](json& c) { c["rings"] = ""; });
       }},
      {"off_empty_array", [&](const fs::path& d) {
         rings_off(d);
         test_support::write_case_json(d, "rings.json", json{{"rings", json::array()}});
       }},
  };
  for (const auto& c : allowed) {
    std::string err;
    try {
      Model m = test_support::build_small_case(c.name, 500, 5, c.edit);
      int not_ring1 = 0;
      for (size_t p = 1; p <= m.pop.popn; ++p) not_ring1 += m.pop.ring[p].v != 1;
      CHECK_MSG(Ring::names == (vector<string>{"error", "population"}) && Ring::count == 1 && not_ring1 == 0 &&
                    m.ring_members.empty(),
                "{}: rings not set up as population only (names {}, {} people off ring 1)", c.name, Ring::names,
                not_ring1);
    } catch (const std::exception& e) { err = e.what(); }
    CHECK_MSG(err.empty(), "{}: should be allowed, got '{}'", c.name, err);
  }

  auto rings_on = [](const fs::path& d) { test_support::edit_config(d, [](json& c) { c["do_rings"] = true; }); };
  const vector<Case> rejected{
      {"on_no_tag", [&](const fs::path& d) {
         rings_on(d);
         test_support::edit_config(d, [](json& c) { c.erase("rings"); });
       }},
      {"on_empty_array", [&](const fs::path& d) {
         rings_on(d);
         test_support::write_case_json(d, "rings.json", json{{"rings", json::array()}});
       }},
  };
  for (const auto& c : rejected) {
    const fs::path dir = test_support::make_case_dir(c.name);
    c.edit(dir);
    bool threw = false;
    try { test_support::require_inputs_verify(dir); } catch (const std::exception&) { threw = true; }
    CHECK_MSG(threw, "{}: input_verify accepted do_rings=true without ring parameters", c.name);
    fs::remove_all(dir);
  }
}

// input_verify rejects a bad case (it throws; run in a child in case it ever exits) and logs errors
void test_input_verify_exits_with_log_for_bad_case() {
  const fs::path dir = test_support::make_case_dir("setup_bad");
  test_support::edit_config(dir, [](json& c) {
    c["days"] = 0;
    c["locale"] = 12345;
  });
  const int rc = test_support::child_exit_code([&] {
    fs::current_path(dir);
    test_support::StdoutCapture quiet;
    input_verify(dir / "input");
  });
  CHECK_MSG(rc != 0, "input_verify returned normally for an invalid case");
  const auto log = test_support::read_file_text(dir / "input-error-log.txt");
  CHECK(log.contains("'days' must be > 0"));
  CHECK(log.contains("12345"));
  fs::remove_all(dir);
}

// sniff-tier macro test: short headless runs through the real setup path, checked with
// the whole-run invariants. A failure here names the invariant and the unit tests to run.
void test_short_runs_hold_invariants() {
  RuntimeNamesGuard guard;
  struct Scenario {
    const char* name;
    std::function<void(const fs::path&)> edit;
  };
  const vector<Scenario> scenarios{
      {"base", {}},
      {"all_on", [](const fs::path& d) {
         test_support::edit_config(d, [](json& c) {
           c["dovax"] = true;
           c["do_rings"] = true;
           c["do_social_distancing"] = true;
         });
         test_support::move_vax_schedules(d, 10, 60);
         json sd = test_support::read_case_json(d, "soc_dist.json");
         sd[0]["startday"] = 20;
         sd[0]["endday"] = 70;
         sd[1]["startday"] = 40;
         test_support::write_case_json(d, "soc_dist.json", sd);
       }},
  };
  for (const auto& sc : scenarios) {
    xo::seed(77);
    Model m = test_support::build_small_case(fmt::format("sniff_{}", sc.name), 8000, 120, sc.edit);
    Histories h = test_support::run_quiet(m);
    CHECK_MSG(invariants::check_run(m.pop, h, sc.name) == 0, "scenario '{}' broke run invariants", sc.name);
    long infected = 0;
    for (size_t p = 1; p <= m.pop.popn; ++p) infected += m.pop.variant_hist[p].count > 0;
    // not an invariant, only a liveness check: the seeds must lead to further infections
    const long seeded = 6;
    CHECK_MSG(infected > 2 * seeded, "scenario '{}': only {} people ever infected -- the epidemic did not spread",
              sc.name, infected);
    if (m.dovax) {
      int vax = 0;
      for (size_t p = 1; p <= m.pop.popn; ++p) vax += m.pop.vaxstatus[p] != Vaxstat::none;
      CHECK_MSG(vax > 0, "scenario '{}': vaccination on but no one vaccinated", sc.name);
    }
  }
}

// P1: r0_sim produces finite positive R0, 0 on zero sendrisk, monotonic in sendrisk,
// and leaves Counter<Mainline> untouched.
void test_r0_sim_behavior_and_day_isolation() {
  RuntimeNamesGuard guard;
  Model m = test_support::build_small_case("r0_sim_unit", 1000, 30);

  sim::Counter<sim::Mainline>::state.reset();
  for (int i = 0; i < 17; ++i) sim::incr_day();
  REQUIRE(sim::get_day() == 17);

  auto quiet_r0 = [](Model& mdl, uint64_t seed) {
    xo::seed(seed);
    test_support::StdoutCapture quiet;
    return r0_sim(mdl);
  };

  const double r0_base = quiet_r0(m, 101);
  CHECK_MSG(std::isfinite(r0_base) && r0_base > 0.5 && r0_base < 10.0,
            "r0_sim on template params = {:.3f}, expected finite in (0.5, 10)", r0_base);
  CHECK_EQ(sim::get_day(), 17);
  CHECK_EQ(sim::alt_day(), 17);

  Model m_zero = m;
  m_zero.mp.infectparams[1].sendrisk.assign(DURATIONLIM + 1, 0.0f);
  CHECK_NEAR(quiet_r0(m_zero, 101), 0.0, 1e-9);
  CHECK_EQ(sim::get_day(), 17);

  Model m_low = m, m_high = m;
  for (float& s : m_low.mp.infectparams[1].sendrisk) s *= 0.5f;
  for (float& s : m_high.mp.infectparams[1].sendrisk) s *= 2.0f;
  const double r0_low = quiet_r0(m_low, 202);
  const double r0_high = quiet_r0(m_high, 202);
  CHECK_MSG(r0_high > r0_low, "r0_sim not monotonic in sendrisk: 2x ({:.3f}) <= 0.5x ({:.3f})", r0_high, r0_low);
}

// P1: rt_sim inside runsim (including rt_sim_interval dividing ndays to test final-day
// extension), preserving Mainline clock and showing Rt decline as immunity builds.
void test_rt_sim_in_runsim_and_immunity_decline() {
  RuntimeNamesGuard guard;
  Model m = test_support::build_small_case("rt_sim_run", 8000, 120, [](const fs::path& d) {
    test_support::edit_config(d, [](json& c) { c["rt_sim_interval"] = 30; });
  });
  test_support::StdoutCapture cap;
  Histories h = runsim(m);
  const string out = cap.finish();

  CHECK_MSG(invariants::check_run(m.pop, h, "rt_sim") == 0, "rt_sim run broke invariants");
  CHECK_EQ(sim::get_day(), 120);
  CHECK_EQ(sim::alt_day(), 120);

  std::map<int, double> rts;
  constexpr std::string_view prefix = "rt at simulation day ";
  for (const auto& line : test_support::split_trimmed_lines(out)) {
    if (!line.starts_with(prefix)) continue;
    const auto eq = line.find(" = ");
    REQUIRE(eq != string::npos);
    const int day = std::stoi(line.substr(prefix.size(), eq - prefix.size()));
    const double val = std::stod(line.substr(eq + 3));
    rts[day] = val;
  }
  REQUIRE(rts.size() == 4);
  for (int d : {30, 60, 90, 120}) {
    REQUIRE(rts.contains(d));
    CHECK_MSG(std::isfinite(rts[d]) && rts[d] >= 0.0, "rt at day {} invalid: {}", d, rts[d]);
  }
  CHECK_MSG(rts[30] > 0.5, "early rt at day 30 = {:.3f}, expected > 0.5", rts[30]);
  CHECK_MSG(rts[120] < rts[30], "rt did not decline from day 30 ({:.3f}) to day 120 ({:.3f})", rts[30], rts[120]);
}

// P2: multi-variant mini-simulation exercising derived sendrisk/riskadjust, cross-variant
// recovery_immunity, vaccination, rings, and multi-variant Histories invariants.
void test_multi_variant_mini_simulation() {
  RuntimeNamesGuard guard;
  xo::seed(77);
  Model m = test_support::build_small_case("multi_variant", 8000, 120, [](const fs::path& d) {
    test_support::edit_config(d, [](json& c) {
      c["dovax"] = true;
      c["do_rings"] = true;
    });
    test_support::move_vax_schedules(d, 10, 60);
    json seeds = test_support::read_case_json(d, "seed.json");
    for (const char* age : {"Age20_39", "Age40_59"}) {
      seeds.push_back(json{
          {"triggerday", 35},
          {"startofday", true},
          {"filter", json::array({json{{"trait", "agegrp"}, {"val", age}}})},
          {"change", json{
              {"terms", json::array({
                  json{{"trait", "status"}, {"val", "infectious"}},
                  json{{"trait", "cond"}, {"val", "nil"}},
                  json{{"trait", "variant"}, {"val", "omicron_ba4_5"}},
                  json{{"trait", "duration"}, {"val", 1}},
              })},
              {"count", 5},
          }},
      });
    }
    test_support::write_case_json(d, "seed.json", seeds);
  });
  Histories h = test_support::run_quiet(m);
  CHECK_MSG(invariants::check_run(m.pop, h, "multi_variant") == 0, "multi_variant broke run invariants");

  const uint8_t base_id = Variant{"base"}.v;
  const uint8_t omi_id = Variant{"omicron_ba4_5"}.v;
  long base_new = 0, omi_new = 0;
  for (size_t d = 1; d <= h.day_cnt; ++d) {
    base_new += h.aggregate_value(TraitID::variant, Phase::new_, base_id, d);
    omi_new += h.aggregate_value(TraitID::variant, Phase::new_, omi_id, d);
  }
  CHECK_MSG(base_new > 20, "multi_variant: base new infections = {}, expected > 20", base_new);
  CHECK_MSG(omi_new > 20, "multi_variant: omicron_ba4_5 new infections = {}, expected > 20", omi_new);

  int cross_reinf = 0;
  for (size_t p = 1; p <= m.pop.popn; ++p) {
    const auto& vh = m.pop.variant_hist[p];
    const size_t n = vh.stored_count();
    if (n < 2) continue;
    bool diff = false;
    for (size_t i = 1; i < n; ++i)
      if (vh.arr[i] != vh.arr[0]) { diff = true; break; }
    cross_reinf += diff;
  }
  CHECK_MSG(cross_reinf > 0, "multi_variant: expected cross-variant reinfections, got 0");
}

// P2: running all_on -> base -> all_on back-to-back in one process WITHOUT an intervening
// RuntimeNamesGuard leaves no static registry holdover and reproduces a clean base run.
void test_sequential_case_isolation_without_guard() {
  RuntimeNamesGuard outer_guard;
  auto status_totals = [](const PopData& pop) {
    std::array<long, 5> c{};
    for (size_t p = 1; p <= pop.popn; ++p) ++c[pop.status[p].v];
    return c;
  };
  auto all_on_edit = [](const fs::path& d) {
    test_support::edit_config(d, [](json& c) {
      c["dovax"] = true;
      c["do_rings"] = true;
      c["do_social_distancing"] = true;
    });
    test_support::move_vax_schedules(d, 10, 50);
  };

  // Reference clean base run
  std::array<long, 5> base_ref{};
  {
    RuntimeNamesGuard ref_guard;
    xo::seed(77);
    Model m = test_support::build_small_case("seq_base_ref", 3000, 60);
    Histories h = test_support::run_quiet(m);
    CHECK_EQ(invariants::check_run(m.pop, h, "seq_base_ref"), 0);
    base_ref = status_totals(m.pop);
  }

  // Sequential runs without inner guard: all_on -> base -> all_on
  xo::seed(77);
  Model m_all1 = test_support::build_small_case("seq_all_1", 3000, 60, all_on_edit);
  Histories h_all1 = test_support::run_quiet(m_all1);
  CHECK_EQ(invariants::check_run(m_all1.pop, h_all1, "seq_all_1"), 0);
  CHECK_MSG(Vax::count > 0 && Ring::count > 1 && SDCase::names.size() > 1,
            "seq_all_1 did not populate optional registries");

  xo::seed(77);
  Model m_base = test_support::build_small_case("seq_base_after", 3000, 60);
  Histories h_base = test_support::run_quiet(m_base);
  CHECK_EQ(invariants::check_run(m_base.pop, h_base, "seq_base_after"), 0);
  CHECK_EQ(Vax::count, uint16_t{0});
  CHECK(Vax::names.empty());
  CHECK_EQ(Ring::count, uint16_t{1});
  CHECK_EQ(Ring::names, (vector<string>{"error", "population"}));
  CHECK_EQ(SDCase::names, (vector<string>{"none"}));
  CHECK_MSG(status_totals(m_base.pop) == base_ref,
            "base run after all_on differed from clean base run (static registry holdover)");

  xo::seed(77);
  Model m_all2 = test_support::build_small_case("seq_all_2", 3000, 60, all_on_edit);
  Histories h_all2 = test_support::run_quiet(m_all2);
  CHECK_EQ(invariants::check_run(m_all2.pop, h_all2, "seq_all_2"), 0);
  CHECK_MSG(Vax::count > 0 && Ring::count > 1 && SDCase::names.size() > 1,
            "seq_all_2 did not repopulate optional registries");
}

// P2: full run with out_ring_prob == 0 and seeds restricted to ring_1 keeps ring_2 at 0 infections.
void test_full_run_zero_out_ring_prob_isolates_ring2() {
  RuntimeNamesGuard guard;
  xo::seed(99);
  Model m = test_support::build_small_case("zero_out_ring", 4000, 60, [](const fs::path& d) {
    test_support::edit_config(d, [](json& c) { c["do_rings"] = true; });
    json rings = test_support::read_case_json(d, "rings.json");
    for (auto& r : rings["rings"])
      for (auto& [k, v] : r["out_ring_prob_by_agegrp"].items()) v = 0.0;
    test_support::write_case_json(d, "rings.json", rings);

    json seeds = test_support::read_case_json(d, "seed.json");
    for (auto& s : seeds) s["filter"].push_back(json{{"trait", "ring"}, {"val", "ring_1"}});
    test_support::write_case_json(d, "seed.json", seeds);
  });
  Histories h = test_support::run_quiet(m);
  CHECK_EQ(invariants::check_run(m.pop, h, "zero_out_ring"), 0);
  int r1_inf = 0, r2_inf = 0;
  for (size_t p = 1; p <= m.pop.popn; ++p) {
    if (m.pop.variant_hist[p].count == 0) continue;
    if (m.pop.ring[p].v == 1) ++r1_inf;
    else if (m.pop.ring[p].v == 2) ++r2_inf;
  }
  CHECK_MSG(r1_inf > 20, "zero_out_ring: ring_1 had only {} infections", r1_inf);
  CHECK_EQ(r2_inf, 0);
}

}  // namespace

void run_setup_tests(const test_support::TestRunOptions&) {
  using tst::run_test;
  run_test("use_dir_builds_valid_model", test_use_dir_builds_valid_model);
  run_test("use_dir_with_rings_vax_distancing", test_use_dir_with_rings_vax_and_distancing);
  run_test("setup_sim_rejects_bad_config", test_setup_sim_rejects_bad_config);
  run_test("rings_config_rules", test_rings_config_rules);
  run_test("input_verify_exits_with_log", test_input_verify_exits_with_log_for_bad_case);
  run_test("short_runs_hold_invariants", test_short_runs_hold_invariants);
  run_test("r0_sim_behavior_and_day_isolation", test_r0_sim_behavior_and_day_isolation);
  run_test("rt_sim_in_runsim_and_immunity_decline", test_rt_sim_in_runsim_and_immunity_decline);
  run_test("multi_variant_mini_simulation", test_multi_variant_mini_simulation);
  run_test("sequential_case_isolation_without_guard", test_sequential_case_isolation_without_guard);
  run_test("full_run_zero_out_ring_prob_isolates_ring2", test_full_run_zero_out_ring_prob_isolates_ring2);
}
