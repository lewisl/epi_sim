// setup: the flow from a scaffolded case directory to a Model ready for runsim
// (setup_dir -> input_verify -> build_model/setup_sim), the validity of that Model, and
// a short headless run of it checked with the whole-run invariants (the sniff-tier
// macro test).
#include "test_support.h"
#include "test_invariants.h"

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

}  // namespace

void run_setup_tests(const test_support::TestRunOptions&) {
  using tst::run_test;
  run_test("use_dir_builds_valid_model", test_use_dir_builds_valid_model);
  run_test("use_dir_with_rings_vax_distancing", test_use_dir_with_rings_vax_and_distancing);
  run_test("setup_sim_rejects_bad_config", test_setup_sim_rejects_bad_config);
  run_test("rings_config_rules", test_rings_config_rules);
  run_test("input_verify_exits_with_log", test_input_verify_exits_with_log_for_bad_case);
  run_test("short_runs_hold_invariants", test_short_runs_hold_invariants);
}
