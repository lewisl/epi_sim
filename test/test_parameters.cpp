#include "test_support.h"

#include "../src/input_verify.h"
#include "../src/template.h"

namespace {

namespace fs = std::filesystem;

constexpr std::string_view GROUP = "parameters";

// Writes `content` to a fresh temp file under a uniquely-named temp dir and
// returns its path. Caller removes the parent dir when done (fs::remove_all).
fs::path write_temp_json(std::string_view label, std::string_view content) {
  const auto tmp_dir = fs::temp_directory_path() /
                       fmt::format("epi_sim_{}_{}", label, std::random_device{}());
  fs::create_directories(tmp_dir);
  const auto path = tmp_dir / "input.json";
  std::ofstream out(path);
  out << content;
  return path;
}

// Runs `action`, asserting it throws std::runtime_error with a message
// containing `expected_substring`.
void expect_throws_containing(const std::function<void()>& action,
                              std::string_view expected_substring) {
  bool threw = false;
  std::string message;
  try {
    action();
  } catch (const std::runtime_error& e) {
    threw = true;
    message = e.what();
  }
  CHECK(threw);
  CHECK(message.find(expected_substring) != std::string::npos);
}

// Minimal well-formed variant body: valid unless empty_sendrisk/empty_recvrisk
// force the "no earlier variant to derive from" cases.
json minimal_variant_body(bool empty_sendrisk = false, bool empty_recvrisk = false) {
  json body;
  body["spread"]["sendrisk"] = empty_sendrisk ? json::array() : json::array({0.1, 0.2});
  body["spread"]["recvrisk"] = empty_recvrisk ? json::array() : json::array({0.1, 0.2});
  body["spread"]["basemultiplier"] = 1.0;
  body["immunity"]["recovery_immunity"] = json::object();
  body["immunity"]["immunehalflife"] = 360;
  return body;
}

//
// load_variants_data: guards just added to parameters.cpp -- the primary
// variant must be named "base", and it can't rely on the "derive from base"
// shorthand for its own sendrisk/recvrisk since there is no earlier variant.
//

void test_load_variants_data_rejects_non_base_primary() {
  test_support::VariantNamesGuard variant_guard;
  json jdata;
  jdata["not_base"] = minimal_variant_body();

  expect_throws_containing([&] { (void)load_variants_data(jdata); },
                           "must be named 'base'");
}

void test_load_variants_data_rejects_empty_base_sendrisk() {
  test_support::VariantNamesGuard variant_guard;
  json jdata;
  jdata["base"] = minimal_variant_body(/*empty_sendrisk=*/true);

  expect_throws_containing([&] { (void)load_variants_data(jdata); },
                           "must supply non-empty sendrisk");
}

void test_load_variants_data_rejects_empty_base_recvrisk() {
  test_support::VariantNamesGuard variant_guard;
  json jdata;
  jdata["base"] = minimal_variant_body(false, /*empty_recvrisk=*/true);

  expect_throws_containing([&] { (void)load_variants_data(jdata); },
                           "must supply non-empty recvrisk");
}

// Minimal well-formed progression body for a single agegrp/breakday; the
// "severe" row is the one perturbed per test.
json minimal_progression_body(vector<float> severe_row) {
  json body;
  body["progression_tree"]["age0_19"]["5"]["nil"]    = json::array({0, 0.4, 0.5, 0.1, 0, 0});
  body["progression_tree"]["age0_19"]["5"]["mild"]   = json::array({0, 0.3, 0.6, 0.1, 0, 0});
  body["progression_tree"]["age0_19"]["5"]["sick"]   = json::array({0, 0, 0, 1, 0, 0});
  body["progression_tree"]["age0_19"]["5"]["severe"] = severe_row;
  body["progression_factors"]["riskadjust"] = json::array();
  body["progression_factors"]["vaxhalflifeadjust"] = json::object();
  return body;
}

//
// load_progression_set: guard just added -- an explicit progression_tree row
// must be exactly 6 probabilities summing to 1.0.
//

void test_load_progression_set_rejects_row_not_summing_to_one() {
  json jdata;
  jdata["base"] = minimal_progression_body({0, 0, 0, 0, 0.5, 0});  // sums to 0.5

  expect_throws_containing([&] { (void)load_progression_set(jdata); },
                           "must sum to 1.0");
}

void test_load_progression_set_rejects_wrong_row_length() {
  json jdata;
  jdata["base"] = minimal_progression_body({0, 0, 0, 1, 0});  // only 5 entries

  expect_throws_containing([&] { (void)load_progression_set(jdata); },
                           "must have exactly 6 probabilities");
}

//
// load_vax_sched: guard just added -- mix values across vaxesincluded must
// sum to 1.0 (categorical_fast silently falls back to brand index 0 otherwise).
//

void test_load_vax_sched_rejects_mix_not_summing_to_one() {
  test_support::VaxNamesGuard vax_guard;
  Vax::names = {"none", "pfizer", "moderna"};
  const auto path = write_temp_json("sched_bad_mix", R"({
    "vaxesincluded": {
      "pfizer": {"mix": 0.5, "starting_doses": 100, "pct2ndshot": 1.0, "pctboost": 1.0, "alternate": []},
      "moderna": {"mix": 0.3, "starting_doses": 100, "pct2ndshot": 1.0, "pctboost": 1.0, "alternate": []}
    },
    "dayrange": [1, 30],
    "targetpct": 1.0,
    "filtervec": ["age20_39"],
    "shotmode": "all",
    "pattern": [1.0, 1.0]
  })");
  expect_throws_containing([&] { (void)load_vax_sched(path.string()); },
                           "must sum to 1.0");
  fs::remove_all(path.parent_path());
}

//
// load_ring_traits: these guards already exist in parameters.cpp -- these
// tests pin down their current, correct behavior (no source changes here).
//

void test_load_ring_traits_happy_path() {
  test_support::RingNamesGuard ring_guard;
  const auto path = test_support::sample_paths().rings;

  RingTraits rt = load_ring_traits(path);

  CHECK(rt.ring_count() == 2);
  CHECK(Ring::names.size() == 3);  // sentinel + ring_1 + ring_2
  CHECK(Ring::names[1] == "ring_1");
  CHECK(Ring::names[2] == "ring_2");
  CHECK(approx_equal(rt.pct_of_population[1], 0.4, 1e-6));
  CHECK(approx_equal(rt.pct_of_population[2], 0.6, 1e-6));
  // out_ring_prob is indexed directly by the Agegrp's raw (1-based) value, not zidx
  CHECK(approx_equal(rt.out_ring_prob[1][AGE0_19.v], 0.05, 1e-6));
  CHECK(approx_equal(rt.out_ring_prob[2][AGE80_UP.v], 0.15, 1e-6));
}

void test_load_ring_traits_rejects_missing_rings_key() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_no_key", R"({"not_rings":[]})");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "expected top-level 'rings' key");
  fs::remove_all(path.parent_path());
}

void test_load_ring_traits_rejects_non_array_rings() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_not_array", R"({"rings": {"a":1}})");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "expected an array under top-level 'rings' key");
  fs::remove_all(path.parent_path());
}

void test_load_ring_traits_rejects_empty_rings_array() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_empty", R"({"rings": []})");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "expected at least one ring");
  fs::remove_all(path.parent_path());
}

void test_load_ring_traits_rejects_duplicate_names() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_dup_name", R"({
    "rings": [
      {"name":"dup","pct_of_population":0.5,"out_ring_prob_by_agegrp":
        {"age0_19":0.1,"age20_39":0.1,"age40_59":0.1,"age60_79":0.1,"age80_up":0.1}},
      {"name":"dup","pct_of_population":0.5,"out_ring_prob_by_agegrp":
        {"age0_19":0.1,"age20_39":0.1,"age40_59":0.1,"age60_79":0.1,"age80_up":0.1}}
    ]
  })");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "duplicate ring name");
  fs::remove_all(path.parent_path());
}

void test_load_ring_traits_rejects_missing_pct_of_population() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_no_pct", R"({
    "rings": [{"name":"ring_1","out_ring_prob_by_agegrp":
      {"age0_19":0.1,"age20_39":0.1,"age40_59":0.1,"age60_79":0.1,"age80_up":0.1}}]
  })");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "missing pct_of_population");
  fs::remove_all(path.parent_path());
}

void test_load_ring_traits_rejects_pct_out_of_range() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_pct_range", R"({
    "rings": [{"name":"ring_1","pct_of_population":1.5,"out_ring_prob_by_agegrp":
      {"age0_19":0.1,"age20_39":0.1,"age40_59":0.1,"age60_79":0.1,"age80_up":0.1}}]
  })");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "pct_of_population=");
  fs::remove_all(path.parent_path());
}

void test_load_ring_traits_rejects_missing_out_ring_prob() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_no_prob", R"({
    "rings": [{"name":"ring_1","pct_of_population":1.0}]
  })");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "missing out_ring_prob_by_agegrp");
  fs::remove_all(path.parent_path());
}

void test_load_ring_traits_rejects_unknown_agegrp() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_bad_agegrp", R"({
    "rings": [{"name":"ring_1","pct_of_population":1.0,"out_ring_prob_by_agegrp":
      {"age0_19":0.1,"age20_39":0.1,"age40_59":0.1,"age60_79":0.1,"age80_up":0.1,"bogus_age":0.1}}]
  })");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "unknown agegrp");
  fs::remove_all(path.parent_path());
}

void test_load_ring_traits_rejects_prob_out_of_range() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_prob_range", R"({
    "rings": [{"name":"ring_1","pct_of_population":1.0,"out_ring_prob_by_agegrp":
      {"age0_19":1.5,"age20_39":0.1,"age40_59":0.1,"age60_79":0.1,"age80_up":0.1}}]
  })");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "out_ring_prob=");
  fs::remove_all(path.parent_path());
}

void test_load_ring_traits_rejects_missing_agegrp_entry() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_missing_entry", R"({
    "rings": [{"name":"ring_1","pct_of_population":1.0,"out_ring_prob_by_agegrp":
      {"age0_19":0.1,"age20_39":0.1,"age40_59":0.1,"age60_79":0.1}}]
  })");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "missing out_ring_prob for agegrp");
  fs::remove_all(path.parent_path());
}

void test_load_ring_traits_rejects_pct_sum_not_one() {
  test_support::RingNamesGuard ring_guard;
  const auto path = write_temp_json("rings_bad_sum", R"({
    "rings": [
      {"name":"ring_1","pct_of_population":0.4,"out_ring_prob_by_agegrp":
        {"age0_19":0.1,"age20_39":0.1,"age40_59":0.1,"age60_79":0.1,"age80_up":0.1}},
      {"name":"ring_2","pct_of_population":0.4,"out_ring_prob_by_agegrp":
        {"age0_19":0.1,"age20_39":0.1,"age40_59":0.1,"age60_79":0.1,"age80_up":0.1}}
    ]
  })");
  expect_throws_containing([&] { (void)load_ring_traits(path.string()); },
                           "must sum to 1.0");
  fs::remove_all(path.parent_path());
}

//
// load_vax_sched_set: directory-existence guards already exist -- pin down
// current behavior (no source changes here).
//

void test_load_vax_sched_set_rejects_missing_directory() {
  const auto missing_dir = fs::temp_directory_path() /
                           fmt::format("epi_sim_missing_sched_dir_{}", std::random_device{}());
  expect_throws_containing([&] { (void)load_vax_sched_set(missing_dir.string()); },
                           "does not exist");
}

void test_load_vax_sched_set_rejects_non_directory_path() {
  const auto tmp_dir = fs::temp_directory_path() /
                       fmt::format("epi_sim_sched_file_{}", std::random_device{}());
  fs::create_directories(tmp_dir);
  const auto file_path = tmp_dir / "not_a_dir.json";
  { std::ofstream out(file_path); out << "{}"; }

  expect_throws_containing([&] { (void)load_vax_sched_set(file_path.string()); },
                           "is not a directory");
  fs::remove_all(tmp_dir);
}

void test_model_params_loading(const test_support::TestRunOptions& options) {
  fmt::print("\n=== Testing ModelParams Loading ===\n\n");

  test_support::VariantNamesGuard variant_names_guard;
  test_support::VaxNamesGuard vax_names_guard;
  Variant::names.clear();
  Vax::names.clear();
  const auto paths = test_support::sample_paths();

  GeoData geodata = load_geodata_csv(paths.geodata);
  CHECK(geodata.num_rows > 0);
  CHECK(geodata.num_rows == geodata.fips.size());
  CHECK(geodata.num_rows == geodata.county.size());
  CHECK(geodata.num_rows == geodata.pop.size());
  const size_t locale_idx = test_support::require_locale_index(geodata, 38015);
  CHECK(geodata.county[locale_idx] == "Burleigh");
  CHECK(geodata.city[locale_idx] == "Bismarck");
  CHECK(geodata.state[locale_idx] == "ND");
  CHECK(geodata.pop[locale_idx] == 95626);
  CHECK(geodata.indoor_st[locale_idx] == "0001-09-15");
  CHECK(geodata.indoor_end[locale_idx] == "0002-05-30");

  auto [infectparams, progressionset, trvec, variants] = load_infect_params(paths.variants);
  CHECK(!variants.empty());
  CHECK(variants.size() == infectparams.size());
  CHECK(variants.size() == progressionset.progression.size());
  CHECK(variants[0].show() == "none");
  CHECK(variants[1].show() == "base");
  CHECK(variants[2].show() == "alpha");
  CHECK(infectparams[1].sendrisk.size() > 20);
  CHECK(approx_equal(infectparams[1].sendrisk[1], 0.3, 1e-6));
  CHECK(approx_equal(infectparams[1].sendrisk[2], 0.65, 1e-6));
  CHECK(approx_equal(infectparams[1].sendrisk[5], 0.85, 1e-6));
  CHECK(approx_equal(infectparams[1].recvrisk[0], 0.1, 1e-6));
  CHECK(approx_equal(infectparams[1].recvrisk[4], 0.56, 1e-6));
  CHECK(progressionset.progression[1].tree.size() == 5);
  CHECK(progressionset.progression[1].tree[0].contains(5));
  const auto& base_age0_day5_nil = progressionset.progression[1].tree[0].at(5)[0];
  CHECK(base_age0_day5_nil.size() == 6);
  CHECK(approx_equal(base_age0_day5_nil[1], 0.4, 1e-6));
  CHECK(approx_equal(base_age0_day5_nil[2], 0.5, 1e-6));
  CHECK(progressionset.progression[2].factors.riskadjust.size() == 6);
  CHECK(approx_equal(progressionset.progression[2].factors.riskadjust[3], 1.1, 1e-6));
  CHECK(std::all_of(trvec.begin(), trvec.end(),
                     [](float value) { return approx_equal(value, 0.0, 1e-6); }));

  SocialParams socialdata = load_social_params(paths.social);
  CHECK(approx_equal(socialdata.gammashape, 1.0, 1e-6));
  CHECK(approx_equal(socialdata.indoor_uplift, 1.1, 1e-6));
  CHECK(approx_equal(socialdata.contactfactors[0][1], 1.995, 1e-6));
  CHECK(approx_equal(socialdata.contactfactors[3][4], 0.475, 1e-6));
  CHECK(approx_equal(socialdata.touchfactors[0][0], 0.55, 1e-6));
  CHECK(approx_equal(socialdata.touchfactors[4][0], 0.28, 1e-6));

  VaxSet vaxset = load_vax_data(paths.vaccines);
  CHECK(vaxset.size() == 3);
  CHECK(Vax::names.size() == 4);
  CHECK(Vax::names[0] == "none");
  CHECK(Vax::names[1] == "Pfizer");
  CHECK(Vax::names[2] == "Moderna");
  CHECK(Vax::names[3] == "JnJ");
  const auto& pfizer = test_support::require_vax(vaxset, "Pfizer");
  CHECK(pfizer.reqdshots == 2);
  CHECK(pfizer.delay2ndshot == 21);
  CHECK(pfizer.delaybooster == 160);
  CHECK(approx_equal(pfizer.day1_effect, 0.65, 1e-6));
  CHECK(approx_equal(test_support::require_named_factor(pfizer.infectfactor, "delta"), 0.85, 1e-6));
  CHECK(pfizer.effectiveness.size() == 3);
  CHECK(pfizer.effectiveness[1].first == "full");
  CHECK(approx_equal(test_support::require_named_factor(pfizer.effectiveness[1].second, "delta"), 0.8, 1e-6));

  VaxSchedSet vaxschedset = load_vax_sched_set(paths.vax_sched_dir);
  CHECK(vaxschedset.size() == 2);

  const auto& old_sched = test_support::require_sched(vaxschedset, "loc38015_old");
  CHECK(old_sched.vaxesincluded.size() == 3);
  const auto& jnj_sched = test_support::require_sched_vax(old_sched, "JnJ");
  CHECK(approx_equal(jnj_sched.mix, 0.05, 1e-6));
  CHECK(jnj_sched.starting_doses == 4000);
  CHECK(approx_equal(jnj_sched.pct2ndshot, 0.0, 1e-6));
  CHECK(approx_equal(jnj_sched.pctboost, 0.4, 1e-6));
  CHECK(old_sched.dayrange.first == 350);
  CHECK(old_sched.dayrange.second == 700);
  CHECK(approx_equal(old_sched.targetpct, 0.95, 1e-6));
  CHECK(old_sched.filtervec.size() == 2);
  CHECK(old_sched.filtervec[0] == AGE80_UP);
  CHECK(old_sched.filtervec[1] == AGE60_79);
  CHECK(old_sched.shotmode == "all");
  CHECK(old_sched.pattern.size() == 12);
  CHECK(approx_equal(old_sched.pattern[3], 0.1, 1e-6));
  CHECK(static_cast<bool>(old_sched.spreadfunc));

  const auto& young_sched = test_support::require_sched(vaxschedset, "loc38015_young");
  CHECK(approx_equal(young_sched.targetpct, 0.65, 1e-6));
  CHECK(young_sched.filtervec.size() == 3);
  CHECK(young_sched.filtervec[0] == AGE0_19);
  CHECK(young_sched.filtervec[1] == AGE20_39);
  CHECK(young_sched.filtervec[2] == AGE40_59);
  CHECK(static_cast<bool>(young_sched.spreadfunc));

  if (options.write_artifacts) {
    std::ostringstream artifact;
    artifact << "Parameter loader summary\n";
    artifact << "========================\n\n";
    artifact << "geodata:\n";
    artifact << "  rows: " << geodata.num_rows << "\n";
    artifact << "  locale 38015 county/city/state: "
             << geodata.county[locale_idx] << ", "
             << geodata.city[locale_idx] << ", "
             << geodata.state[locale_idx] << "\n";
    artifact << "  locale 38015 pop: " << geodata.pop[locale_idx] << "\n";
    artifact << "  indoor window: " << geodata.indoor_st[locale_idx]
             << " -> " << geodata.indoor_end[locale_idx] << "\n\n";

    artifact << "variants:\n";
    artifact << fmt::format("  names: [{}]\n", fmt::join(Variant::names, ", "));
    artifact << "  infectparams count: " << infectparams.size() << "\n";
    artifact << "  base sendrisk[2]: " << infectparams[1].sendrisk[2] << "\n";
    artifact << "  alpha riskadjust[3]: " << progressionset.progression[2].factors.riskadjust[3] << "\n\n";

    artifact << "social:\n";
    artifact << "  gammashape: " << socialdata.gammashape << "\n";
    artifact << "  indoor_uplift: " << socialdata.indoor_uplift << "\n";
    artifact << "  contactfactors[0][1]: " << socialdata.contactfactors[0][1] << "\n";
    artifact << "  touchfactors[4][0]: " << socialdata.touchfactors[4][0] << "\n\n";

    artifact << "vaccines:\n";
    artifact << fmt::format("  names: [{}]\n", fmt::join(Vax::names, ", "));
    artifact << "  set size: " << vaxset.size() << "\n";
    artifact << "  Pfizer reqdshots/delay2nd/booster: "
             << pfizer.reqdshots << "/" << pfizer.delay2ndshot << "/"
             << pfizer.delaybooster << "\n";
    artifact << "  Pfizer delta infectfactor: "
             << test_support::require_named_factor(pfizer.infectfactor, "delta") << "\n\n";

    artifact << "schedules:\n";
    artifact << "  schedule count: " << vaxschedset.size() << "\n";
    artifact << "  loc38015_old targetpct/dayrange: "
             << old_sched.targetpct << " / [" << old_sched.dayrange.first
             << ", " << old_sched.dayrange.second << "]\n";
    artifact << "  loc38015_young targetpct/filter count: "
             << young_sched.targetpct << " / " << young_sched.filtervec.size() << "\n\n";

    artifact << "assembled ModelParams inputs:\n";
    artifact << "  variants: " << variants.size() << "\n";
    artifact << "  vaxset: " << vaxset.size() << "\n";
    artifact << "  schedules: " << vaxschedset.size() << "\n";

    test_support::write_artifact_text(options, GROUP, "loader_summary.txt", artifact.str());
  }

  ModelParams mp{
      .geodata = std::move(geodata),
      .variants = std::move(variants),
      .infectparams = std::move(infectparams),
      .progressionset = std::move(progressionset),
      .trvec = std::move(trvec),
      .socialdata = std::move(socialdata),
      .vaxset = std::move(vaxset),
      .vaxschedset = std::move(vaxschedset),
  };

  CHECK(mp.geodata.pop[locale_idx] == 95626);
  CHECK(mp.variants[1].show() == "base");
  CHECK(mp.vaxset.size() == 3);
  CHECK(mp.vaxschedset.size() == 2);

  fmt::println("=== ModelParams Loading Test Completed ===");
}

//
// input_verify: throw-free structural/invariant checks over a case's input
// files. Fixtures are parsed from the scaffold templates (src/template.cpp,
// the single source of truth) and perturbed one field at a time.
//

using input_verify_detail::Errors;

bool has_error_containing(const Errors& e, std::string_view sub) {
  for (const auto& m : e.msgs) {
    if (m.find(sub) != std::string::npos) return true;
  }
  return false;
}

// config_json carries // comments (matching the on-disk scaffold), so it must
// be parsed with comment-stripping enabled, like the production loader does.
json parse_config() {
  return json::parse(config_json, nullptr, true, true);
}

// A freshly scaffolded case (all template inputs) must produce zero errors.
void test_input_verify_accepts_template_inputs() {
  using namespace input_verify_detail;
  Errors e;
  bool dovax = false, do_rings = false, do_sd = false;
  int locale = 0;
  check_config(parse_config(), e, dovax, do_rings, do_sd, locale);
  check_variants(json::parse(variants_json), e);
  check_vaccines(json::parse(vaccines_json), e);
  check_vax_sched(json::parse(vaxsched::loc38015_old_json), "loc38015_old", e);
  check_vax_sched(json::parse(vaxsched::loc38015_young_json), "loc38015_young", e);
  check_socialparams(json::parse(socialparams_json), e);
  check_seed(json::parse(seed_json), e);
  check_soc_dist(json::parse(social_dist_json), e);
  check_rings(json::parse(rings_json), e);

  CHECK(!e.any());
  CHECK(locale == 38015);
  CHECK(dovax == false);
}

void test_check_config_rejects_missing_days() {
  Errors e;
  bool dovax, dr, dsd;
  int locale;
  json cfg = parse_config();
  cfg.erase("days");
  input_verify_detail::check_config(cfg, e, dovax, dr, dsd, locale);
  CHECK(has_error_containing(e, "missing required key 'days'"));
}

void test_write_error_log_writes_complete_report() {
  const fs::path dir = fs::temp_directory_path() /
                       test_support::unique_name("epi_sim_input_error_log_");
  fs::create_directories(dir);
  const fs::path log_path = dir / "input-error-log.txt";
  const std::string report =
      "Input validation failed for '/tmp/case/input'. 1 problem(s) found:\n"
      "  1. config.json: missing required key 'days'.\n";

  CHECK(input_verify_detail::write_error_log(log_path, report));
  CHECK(test_support::read_file_text(log_path) == report);

  fs::remove_all(dir);
}

void test_check_config_rejects_nonpositive_days() {
  Errors e;
  bool dovax, dr, dsd;
  int locale;
  json cfg = parse_config();
  cfg["days"] = 0;
  input_verify_detail::check_config(cfg, e, dovax, dr, dsd, locale);
  CHECK(has_error_containing(e, "'days' must be > 0"));
}

void test_check_config_rejects_bad_calendar_start() {
  Errors e;
  bool dovax, dr, dsd;
  int locale;
  json cfg = parse_config();
  cfg["calendar_start"] = "2020/01/01";
  input_verify_detail::check_config(cfg, e, dovax, dr, dsd, locale);
  CHECK(has_error_containing(e, "'calendar_start' must be a YYYY-MM-DD date"));
}

void test_check_config_rejects_age_dist_wrong_size() {
  Errors e;
  bool dovax, dr, dsd;
  int locale;
  json cfg = parse_config();
  cfg["age_dist"] = {0.5, 0.5};
  input_verify_detail::check_config(cfg, e, dovax, dr, dsd, locale);
  CHECK(has_error_containing(e, "'age_dist' must have exactly 5 entries"));
}

void test_check_config_rejects_age_dist_bad_sum() {
  Errors e;
  bool dovax, dr, dsd;
  int locale;
  json cfg = parse_config();
  cfg["age_dist"] = {0.1, 0.1, 0.1, 0.1, 0.1};
  input_verify_detail::check_config(cfg, e, dovax, dr, dsd, locale);
  CHECK(has_error_containing(e, "'age_dist' must sum to 1.0"));
}

void test_check_config_requires_vax_keys_when_dovax() {
  Errors e;
  bool dovax = false, dr, dsd;
  int locale;
  json cfg = parse_config();
  cfg["dovax"] = true;
  cfg.erase("vaccines");
  cfg.erase("vax_sched_dir");
  input_verify_detail::check_config(cfg, e, dovax, dr, dsd, locale);
  CHECK(dovax == true);
  CHECK(has_error_containing(e, "missing required key 'vaccines'"));
  CHECK(has_error_containing(e, "missing required key 'vax_sched_dir'"));
}

void test_check_variants_rejects_non_base_first() {
  Errors e;
  json bad = json::object();
  bad["delta"] = json::parse(variants_json)["base"];
  input_verify_detail::check_variants(bad, e);
  CHECK(has_error_containing(e, "the first variant must be named 'base'"));
}

void test_check_variants_rejects_wrong_progression_row_length() {
  Errors e;
  json v = json::parse(variants_json);
  v["base"]["progression_tree"]["age0_19"]["5"]["nil"] = {0, 0.4, 0.5, 0.1, 0};
  input_verify_detail::check_variants(v, e);
  CHECK(has_error_containing(e, "must have exactly 6 probabilities"));
}

void test_check_variants_rejects_progression_row_bad_sum() {
  Errors e;
  json v = json::parse(variants_json);
  v["base"]["progression_tree"]["age0_19"]["5"]["nil"] = {0, 0.4, 0.5, 0.2, 0, 0};
  input_verify_detail::check_variants(v, e);
  CHECK(has_error_containing(e, "probabilities must sum to 1.0"));
}

void test_check_vaccines_rejects_missing_int_key() {
  Errors e;
  json vac = json::parse(vaccines_json);
  vac["Pfizer"].erase("halflife");
  input_verify_detail::check_vaccines(vac, e);
  CHECK(has_error_containing(e, "missing required key 'halflife'"));
}

void test_check_vax_sched_rejects_mix_bad_sum() {
  Errors e;
  json s = json::parse(vaxsched::loc38015_old_json);
  s["vaxesincluded"]["Pfizer"]["mix"] = 0.9;
  input_verify_detail::check_vax_sched(s, "loc38015_old", e);
  CHECK(has_error_containing(e, "'mix' values must sum to 1.0"));
}

void test_check_vax_sched_rejects_bad_dayrange_length() {
  Errors e;
  json s = json::parse(vaxsched::loc38015_old_json);
  s["dayrange"] = {350};
  input_verify_detail::check_vax_sched(s, "loc38015_old", e);
  CHECK(has_error_containing(e, "'dayrange' must have exactly 2 entries"));
}

void test_check_socialparams_rejects_missing_key() {
  Errors e;
  json sp = json::parse(socialparams_json);
  sp.erase("gammashape");
  input_verify_detail::check_socialparams(sp, e);
  CHECK(has_error_containing(e, "missing required key 'gammashape'"));
}

void test_check_seed_rejects_entry_missing_key() {
  Errors e;
  json seed = json::parse(seed_json);
  seed[0].erase("triggerday");
  input_verify_detail::check_seed(seed, e);
  CHECK(has_error_containing(e, "missing required key 'triggerday'"));
}

void test_check_soc_dist_rejects_bad_contact_delta_length() {
  Errors e;
  json sd = json::parse(social_dist_json);
  sd[0]["contact_delta"] = {0.2};
  input_verify_detail::check_soc_dist(sd, e);
  CHECK(has_error_containing(e, "'contact_delta' must have exactly 2 entries"));
}

void test_check_rings_rejects_pct_bad_sum() {
  Errors e;
  json r = json::parse(rings_json);
  r["rings"][0]["pct_of_population"] = 0.1;
  input_verify_detail::check_rings(r, e);
  CHECK(has_error_containing(e, "'pct_of_population' values must sum to 1.0"));
}

void test_check_rings_rejects_missing_agegrp() {
  Errors e;
  json r = json::parse(rings_json);
  r["rings"][0]["out_ring_prob_by_agegrp"].erase("age80_up");
  input_verify_detail::check_rings(r, e);
  CHECK(has_error_containing(e, "missing agegrp 'age80_up'"));
}

}  // namespace

void run_parameter_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running parameter tests...");
  test_model_params_loading(options);
  test_load_variants_data_rejects_non_base_primary();
  test_load_variants_data_rejects_empty_base_sendrisk();
  test_load_variants_data_rejects_empty_base_recvrisk();
  test_load_progression_set_rejects_row_not_summing_to_one();
  test_load_progression_set_rejects_wrong_row_length();
  test_load_vax_sched_rejects_mix_not_summing_to_one();
  test_load_ring_traits_happy_path();
  test_load_ring_traits_rejects_missing_rings_key();
  test_load_ring_traits_rejects_non_array_rings();
  test_load_ring_traits_rejects_empty_rings_array();
  test_load_ring_traits_rejects_duplicate_names();
  test_load_ring_traits_rejects_missing_pct_of_population();
  test_load_ring_traits_rejects_pct_out_of_range();
  test_load_ring_traits_rejects_missing_out_ring_prob();
  test_load_ring_traits_rejects_unknown_agegrp();
  test_load_ring_traits_rejects_prob_out_of_range();
  test_load_ring_traits_rejects_missing_agegrp_entry();
  test_load_ring_traits_rejects_pct_sum_not_one();
  test_load_vax_sched_set_rejects_missing_directory();
  test_load_vax_sched_set_rejects_non_directory_path();
  test_input_verify_accepts_template_inputs();
  test_check_config_rejects_missing_days();
  test_write_error_log_writes_complete_report();
  test_check_config_rejects_nonpositive_days();
  test_check_config_rejects_bad_calendar_start();
  test_check_config_rejects_age_dist_wrong_size();
  test_check_config_rejects_age_dist_bad_sum();
  test_check_config_requires_vax_keys_when_dovax();
  test_check_variants_rejects_non_base_first();
  test_check_variants_rejects_wrong_progression_row_length();
  test_check_variants_rejects_progression_row_bad_sum();
  test_check_vaccines_rejects_missing_int_key();
  test_check_vax_sched_rejects_mix_bad_sum();
  test_check_vax_sched_rejects_bad_dayrange_length();
  test_check_socialparams_rejects_missing_key();
  test_check_seed_rejects_entry_missing_key();
  test_check_soc_dist_rejects_bad_contact_delta_length();
  test_check_rings_rejects_pct_bad_sum();
  test_check_rings_rejects_missing_agegrp();
  if (options.write_artifacts) {
    fmt::println("parameter artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("parameter tests passed.");
}
