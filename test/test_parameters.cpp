#include "test_support.h"

#include "../src/input_verify.h"
#include "../src/template.h"

namespace {

namespace fs = std::filesystem;

constexpr std::string_view GROUP = "parameters";

bool has_error_containing(const input_verify_detail::Errors& e, std::string_view sub);  // defined below

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
  body["spread"]["sendrisk"] = empty_sendrisk ? json::array() : json(vector<float>(DURATIONLIM, 0.1f));
  body["spread"]["recvrisk"] = empty_recvrisk ? json::array() : json(vector<float>(5, 0.1f));
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

json base_progression_variant() {
  return json::parse(variants_json)["base"];
}

void test_load_progression_set_maps_reordered_ages_and_packs_breakdays() {
  json body = base_progression_variant();
  const json original_tree = body["progression_tree"];
  json reordered_tree = json::object();
  for (auto it = Agegrp::names.rbegin(); it != Agegrp::names.rend() - 1; ++it) {
    reordered_tree[*it] = original_tree[*it];
  }
  body["progression_tree"] = std::move(reordered_tree);

  json jdata;
  jdata["base"] = std::move(body);
  ProgressionSet progressionset = load_progression_set(jdata);
  const auto& tree = progressionset.progression[1].tree;

  CHECK(tree.entries.size() == 25);
  CHECK(tree.entry_index[zidx(AGE0_19)][6] == NO_PROGRESSION_ENTRY);

  const int16_t young_entry = tree.entry_index[zidx(AGE0_19)][5];
  REQUIRE(young_entry != NO_PROGRESSION_ENTRY);
  const auto& young_nil = tree.entries[static_cast<size_t>(young_entry)][NIL.v - 2]  /* rows are nil..severe = cond - 2 */;
  CHECK(approx_equal(young_nil[std::to_underlying(Progressmap::ToNil)], 0.4, 1e-6));
  CHECK(approx_equal(young_nil[std::to_underlying(Progressmap::ToMild)], 0.5, 1e-6));

  const int16_t old_entry = tree.entry_index[zidx(AGE80_UP)][5];
  REQUIRE(old_entry != NO_PROGRESSION_ENTRY);
  const auto& old_nil = tree.entries[static_cast<size_t>(old_entry)][NIL.v - 2]  /* rows are nil..severe = cond - 2 */;
  CHECK(approx_equal(old_nil[std::to_underlying(Progressmap::ToNil)], 0.1, 1e-6));
  CHECK(approx_equal(old_nil[std::to_underlying(Progressmap::ToSick)], 0.4, 1e-6));
}

void test_load_progression_set_requires_exact_real_age_names() {
  for (const auto invalid_age : {"unknown", "AGE0_19", "age019", "COUNT"}) {
    json body = base_progression_variant();
    auto& tree = body["progression_tree"];
    tree[invalid_age] = tree["age0_19"];
    tree.erase("age0_19");
    json jdata;
    jdata["base"] = std::move(body);
    expect_throws_containing([&] { (void)load_progression_set(jdata); },
                             "unknown age group");
  }
}

void test_load_progression_set_rejects_out_of_range_breakday() {
  json body = base_progression_variant();
  body["progression_tree"]["age0_19"]["26"] =
      body["progression_tree"]["age0_19"]["5"];
  body["progression_tree"]["age0_19"].erase("5");
  json jdata;
  jdata["base"] = std::move(body);

  expect_throws_containing([&] { (void)load_progression_set(jdata); },
                           "invalid breakday '26'");
}

void test_load_progression_set_requires_terminal_breakday() {
  json body = base_progression_variant();
  body["progression_tree"]["age0_19"].erase("25");
  json jdata;
  jdata["base"] = std::move(body);

  expect_throws_containing([&] { (void)load_progression_set(jdata); },
                           "must define terminal breakday 25");
}

void test_load_progression_set_requires_terminal_outcome() {
  json body = base_progression_variant();
  body["progression_tree"]["age0_19"]["25"]["nil"] = {0, 1, 0, 0, 0, 0};
  json jdata;
  jdata["base"] = std::move(body);

  expect_throws_containing([&] { (void)load_progression_set(jdata); },
                           "must transition only to recover or dead");
}

void test_load_progression_set_requires_every_age_group() {
  json body = base_progression_variant();
  body["progression_tree"].erase("age80_up");
  json jdata;
  jdata["base"] = std::move(body);

  expect_throws_containing([&] { (void)load_progression_set(jdata); },
                           "is missing age group 'age80_up'");
}

void test_load_progression_set_rejects_wrong_riskadjust_length() {
  json jdata;
  jdata["base"] = base_progression_variant();
  jdata["derived"] = base_progression_variant();
  jdata["derived"]["progression_tree"] = nullptr;
  jdata["derived"]["progression_factors"]["riskadjust"] = {1, 1};

  expect_throws_containing([&] { (void)load_progression_set(jdata); },
                           "riskadjust must be empty or contain exactly 6 values");
}

void test_load_seed_case_rejects_duration_above_limit() {
  test_support::VariantNamesGuard variant_guard;
  Variant::names = {"none", "base"};
  json seed = json::parse(seed_json)[0];
  for (auto& term : seed["change"]["terms"]) {
    if (term["trait"] == "duration") term["val"] = DURATIONLIM + 1;
  }
  ModelParams mp;

  expect_throws_containing([&] { (void)load_seed_case(seed, mp); },
                           "outside the supported range");
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


void test_vax_schedule_age_aliases_and_fallback() {
  const auto path = write_temp_json("sched_age_aliases", R"({
    "vaxesincluded": {},
    "dayrange": [1, 30],
    "targetpct": 1.0,
    "filtervec": ["AGE80UP", "Age_20_39"],
    "shotmode": "all",
    "pattern": [1.0, 1.0]
  })");
  const auto sched = load_vax_sched(path.string());
  REQUIRE(sched.filtervec.size() == 2);
  CHECK_EQ(sched.filtervec[0], AGE80_UP);  // underscore/case aliases are accepted
  CHECK_EQ(sched.filtervec[1], AGE20_39);
  fs::remove_all(path.parent_path());

  // unknown names (including the 'unknown' zero slot) would silently vaccinate nobody:
  // rejected by input_verify and by the loader (was G2)
  json js = json::parse(vaxsched::loc38015_old_json);
  for (const char* bad : {"bogus_age", "unknown"}) {
    js["filtervec"] = {"age80_up", bad};
    const auto bad_path = write_temp_json("sched_bad_age", js.dump());
    CHECK_THROWS(std::runtime_error, (void)load_vax_sched(bad_path.string()),
                 fmt::format("filtervec has unknown age group '{}'", bad));
    fs::remove_all(bad_path.parent_path());
    input_verify_detail::Errors e;
    input_verify_detail::check_vax_sched(js, "bad", e);
    CHECK_MSG(has_error_containing(e, "'filtervec' entry") && e.msgs.size() == 1,
              "input_verify for filtervec '{}': {}", bad, e.msgs);
  }
  js["filtervec"] = {"AGE80UP", "Age_20_39"};
  input_verify_detail::Errors ok;
  input_verify_detail::check_vax_sched(js, "aliases", ok);
  CHECK_MSG(!ok.any(), "input_verify rejects accepted aliases: {}", ok.msgs);
}



//
// load_vax_sched_set: directory-existence guards already exist -- pin down
// current behavior (no source changes here).
//




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

  auto [infectparams, progressionset, variant_names] = load_infect_params(paths.variants);
  CHECK(!variant_names.empty());
  CHECK(variant_names.size() == infectparams.size());
  CHECK(variant_names.size() == progressionset.progression.size());
  CHECK(variant_names[0] == "none");
  CHECK(variant_names[1] == "base");
  CHECK(variant_names[2] == "alpha");
  // expected sendrisk comes from the template itself: user days 1..DURATIONLIM at
  // json[0..DURATIONLIM-1]; loader prepends the unused [0]
  const auto tmpl_sendrisk =
      json::parse(variants_json)["base"]["spread"]["sendrisk"].get<vector<float>>();
  CHECK_EQ(tmpl_sendrisk.size(), size_t{DURATIONLIM});
  REQUIRE(infectparams[1].sendrisk.size() == size_t{DURATIONLIM} + 1);
  CHECK(approx_equal(infectparams[1].sendrisk[0], 0.0, 1e-6));   // loader-supplied, unused
  for (size_t d = 1; d <= DURATIONLIM; ++d)
    CHECK_MSG(approx_equal(infectparams[1].sendrisk[d], tmpl_sendrisk[d - 1], 1e-6),
              "sendrisk[{}] = {} want {}", d, infectparams[1].sendrisk[d], tmpl_sendrisk[d - 1]);
  CHECK(approx_equal(infectparams[1].recvrisk[0], 0.1, 1e-6));
  CHECK(approx_equal(infectparams[1].recvrisk[4], 0.56, 1e-6));
  const auto& base_tree = progressionset.progression[1].tree;
  CHECK(base_tree.entries.size() == 25);
  const int16_t base_age0_day5 = base_tree.entry_index[zidx(AGE0_19)][5];
  REQUIRE(base_age0_day5 != NO_PROGRESSION_ENTRY);
  const auto& base_age0_day5_nil =
      base_tree.entries[static_cast<size_t>(base_age0_day5)][NIL.v - 2]  /* rows are nil..severe = cond - 2 */;
  CHECK(base_age0_day5_nil.size() == 6);
  CHECK(approx_equal(base_age0_day5_nil[std::to_underlying(Progressmap::ToNil)], 0.4, 1e-6));
  CHECK(approx_equal(base_age0_day5_nil[std::to_underlying(Progressmap::ToMild)], 0.5, 1e-6));
  CHECK(progressionset.progression[2].factors.riskadjust.size() == 6);
  CHECK(approx_equal(progressionset.progression[2].factors.riskadjust[3], 1.1, 1e-6));
  const auto& alpha_tree = progressionset.progression[2].tree;
  const int16_t alpha_age0_day5 = alpha_tree.entry_index[zidx(AGE0_19)][5];
  REQUIRE(alpha_age0_day5 != NO_PROGRESSION_ENTRY);
  const auto& alpha_age0_day5_nil =
      alpha_tree.entries[static_cast<size_t>(alpha_age0_day5)][NIL.v - 2]  /* rows are nil..severe = cond - 2 */;
  CHECK(approx_equal(
      std::accumulate(alpha_age0_day5_nil.begin(), alpha_age0_day5_nil.end(), 0.0f),
      1.0, 1e-6));
  CHECK(alpha_age0_day5_nil[std::to_underlying(Progressmap::ToSick)] >
        base_age0_day5_nil[std::to_underlying(Progressmap::ToSick)]);

  SocialParams socialdata = load_social_params(paths.social);
  CHECK(approx_equal(socialdata.gammashape, 1.0, 1e-6));
  CHECK(approx_equal(socialdata.indoor_uplift, 1.1, 1e-6));
  CHECK(approx_equal(socialdata.contactfactors[0][1], 1.995, 1e-6));
  CHECK(approx_equal(socialdata.contactfactors[3][4], 0.475, 1e-6));
  CHECK(approx_equal(socialdata.touchfactors[0][0], 0.55, 1e-6));
  CHECK(approx_equal(socialdata.touchfactors[4][0], 0.28, 1e-6));


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
  const json variants = json::parse(variants_json);
  const json vaccines = json::parse(vaccines_json);
  check_config(parse_config(), e, dovax, do_rings, do_sd, locale);
  check_variants(variants, e);
  check_vaccines(vaccines, e);
  check_vaccine_variant_alignment(variants, vaccines, e);
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

void test_input_verify_reports_vaccine_variant_alignment_error() {
  const fs::path case_dir = fs::temp_directory_path() /
                            test_support::unique_name("epi_sim_bad_vax_variant_case_");
  const fs::path input_dir = case_dir / "input";
  fs::create_directories(case_dir);
  fs::copy(test_support::template_params_dir(), input_dir,
           fs::copy_options::recursive);

  json config = parse_config();
  config["dovax"] = true;
  {
    std::ofstream out(input_dir / "config.json");
    out << config.dump(2) << '\n';
  }

  json vaccines = json::parse(vaccines_json);
  vaccines["Pfizer"]["effectiveness"]["full"].erase("alpha");
  {
    std::ofstream out(input_dir / "vaccines.json");
    out << vaccines.dump(2) << '\n';
  }

  const fs::path original_cwd = fs::current_path();
  fs::current_path(case_dir);
  expect_throws_containing([&] { input_verify(input_dir); }, "Input validation failed");
  fs::current_path(original_cwd);

  std::ifstream error_log(case_dir / "input-error-log.txt");
  std::ostringstream report;
  report << error_log.rdbuf();
  CHECK(report.str().find("effectiveness('full'): missing factor for variant 'alpha'") !=
        std::string::npos);

  fs::remove_all(case_dir);
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

void test_check_config_rejects_negative_rt_sim_interval() {
  Errors e;
  bool dovax, dr, dsd;
  int locale;
  json cfg = parse_config();
  cfg["rt_sim_interval"] = 0;
  input_verify_detail::check_config(cfg, e, dovax, dr, dsd, locale);
  CHECK(!has_error_containing(e, "'rt_sim_interval'"));

  e.msgs.clear();
  cfg["rt_sim_interval"] = -1;
  input_verify_detail::check_config(cfg, e, dovax, dr, dsd, locale);
  CHECK(has_error_containing(e, "'rt_sim_interval' must be >= 0"));
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

void test_check_variants_rejects_empty_input() {
  Errors e;
  input_verify_detail::check_variants(json::object(), e);
  CHECK(has_error_containing(e, "expected a non-empty object of variants"));
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

void test_check_vaccine_variant_alignment_rejects_missing_factors() {
  Errors e;
  json variants = json::parse(variants_json);
  json vaccines = json::parse(vaccines_json);
  vaccines["Pfizer"]["infectfactor"].erase("alpha");
  vaccines["Pfizer"]["effectiveness"]["full"].erase("alpha");

  input_verify_detail::check_vaccine_variant_alignment(variants, vaccines, e);

  CHECK(has_error_containing(e, "infectfactor: missing factor for variant 'alpha'"));
  CHECK(has_error_containing(e, "effectiveness('full'): missing factor for variant 'alpha'"));
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

void test_check_variants_rejects_case_duplicate() {
  Errors e;
  json v = json::parse(variants_json);
  v["Base"] = v["base"];
  input_verify_detail::check_variants(v, e);
  CHECK(has_error_containing(e, "duplicate variant name 'Base' (matches 'base'"));
}

void test_check_variants_rejects_reserved_none() {
  Errors e;
  json v = json::parse(variants_json);
  v["None"] = v["base"];
  input_verify_detail::check_variants(v, e);
  CHECK(has_error_containing(e, "variant name 'None' is reserved"));
}

void test_check_vaccines_rejects_case_duplicate() {
  Errors e;
  json vac = json::parse(vaccines_json);
  vac["PFIZER"] = vac["Pfizer"];
  input_verify_detail::check_vaccines(vac, e);
  CHECK(has_error_containing(e, "duplicate vaccine name 'PFIZER' (matches 'Pfizer'"));
}

void test_check_rings_rejects_duplicate_name() {
  json r = json::parse(rings_json);
  const std::string first = r["rings"][0]["name"].get<std::string>();
  {
    Errors e;
    json exact = r;
    exact["rings"][1]["name"] = first;
    input_verify_detail::check_rings(exact, e);
    CHECK(has_error_containing(e, fmt::format("duplicate ring name '{}'", first)));
  }
  {
    Errors e;
    json cased = r;
    std::string upper = first;
    for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    cased["rings"][1]["name"] = upper;
    input_verify_detail::check_rings(cased, e);
    CHECK(has_error_containing(e, fmt::format("duplicate ring name '{}'", upper)));
  }
}

// Exact repeated object keys are collapsed by the JSON parser, so they can
// only be caught while loading the file inside input_verify.
void test_input_verify_reports_exact_duplicate_variant_key() {
  const fs::path case_dir = fs::temp_directory_path() /
                            test_support::unique_name("epi_sim_dup_variant_case_");
  const fs::path input_dir = case_dir / "input";
  fs::create_directories(case_dir);
  fs::copy(test_support::template_params_dir(), input_dir,
           fs::copy_options::recursive);
  {
    std::ofstream out(input_dir / "config.json");  // template_params_dir has no config.json
    out << parse_config().dump(2) << '\n';
  }

  // Re-emit "base" under its own name a second time: {"base": X, ..., "base": X}.
  const json variants = json::parse(variants_json);
  std::string text = variants.dump(2);
  text.pop_back();  // drop closing '}'
  text += fmt::format(",\n  \"base\": {}\n}}\n", variants["base"].dump());
  {
    std::ofstream out(input_dir / "variants.json");
    out << text;
  }

  const fs::path original_cwd = fs::current_path();
  fs::current_path(case_dir);
  expect_throws_containing([&] { input_verify(input_dir); }, "Input validation failed");
  fs::current_path(original_cwd);

  std::ifstream error_log(case_dir / "input-error-log.txt");
  std::ostringstream report;
  report << error_log.rdbuf();
  CHECK(report.str().find("variants.json: duplicate top-level key 'base'") !=
        std::string::npos);

  fs::remove_all(case_dir);
}


//
// Contract tests: the structures built from the template inputs must satisfy what
// spread / progression / vaccinate index into. These read the template JSON directly
// and compare, so they stay valid when parameter values are recalibrated.
//
void test_variants_contract_for_simulation() {
  test_support::RuntimeNamesGuard guard;
  const json j = json::parse(variants_json);
  auto [ip, ps, names] = load_infect_params(test_support::sample_paths().variants);
  REQUIRE(names.size() >= 2);
  CHECK_EQ(ip.size(), names.size());
  CHECK_EQ(ps.progression.size(), names.size());
  const float base_mult = j["base"]["spread"]["basemultiplier"].get<float>();
  CHECK_NEAR(base_mult, 1.0, 1e-6);

  for (size_t v = 1; v < names.size(); ++v) {
    const auto& name = names[v];
    const auto& body = j[name];
    const auto& p = ip[v];
    // sendrisk is indexed by duration 1..DURATIONLIM (index 0 unused)
    CHECK_MSG(p.sendrisk.size() == DURATIONLIM + 1, "{}: sendrisk size {}", name, p.sendrisk.size());
    // recvrisk is indexed by zidx(agegrp)
    CHECK_MSG(p.recvrisk.size() == Agegrp::count, "{}: recvrisk size {}", name, p.recvrisk.size());
    // recovery_immunity is indexed by spreader variant id
    CHECK_MSG(p.recovery_immunity.size() == names.size(), "{}: recovery_immunity size {}", name,
              p.recovery_immunity.size());
    CHECK_MSG(p.immunehalflife > 0, "{}: immunehalflife {}", name, p.immunehalflife);
    if (p.sendrisk.size() != DURATIONLIM + 1 || p.recvrisk.size() != Agegrp::count) continue;

    const auto js = body["spread"]["sendrisk"].get<vector<float>>();
    const float mult = body["spread"]["basemultiplier"].get<float>();
    int bad = 0;
    // JSON holds days 1..DURATIONLIM at js[0..DURATIONLIM-1]; loader prepends unused [0]
    for (size_t d = 1; d <= DURATIONLIM; ++d) {
      const float want = js.empty() ? ip[1].sendrisk[d] * mult : js[d - 1];
      bad += std::abs(p.sendrisk[d] - want) > 1e-5f || p.sendrisk[d] < 0.0f;
    }
    CHECK_MSG(bad == 0, "{}: {} sendrisk values differ from JSON (or base x basemultiplier)", name, bad);
    for (size_t w = 1; w < names.size(); ++w) {
      const auto& ri = body["immunity"]["recovery_immunity"];
      const float want = ri.contains(names[w]) ? ri[names[w]].get<float>() : 0.0f;
      CHECK_MSG(std::abs(p.recovery_immunity[w] - want) < 1e-6f,
                "{}: recovery_immunity[{}={}] = {} want {}", name, w, names[w], p.recovery_immunity[w], want);
    }

    // progression tree: terminal breakday for every age, valid packed indices, rows sum to 1
    const auto& tree = ps.progression[v].tree;
    int rows_bad = 0;
    for (size_t a = 0; a < Agegrp::count; ++a) {
      CHECK_MSG(tree.entry_index[a][0] == NO_PROGRESSION_ENTRY, "{}: day-0 entry for age {}", name, a);
      CHECK_MSG(tree.entry_index[a][DURATIONLIM] != NO_PROGRESSION_ENTRY, "{}: age {} lacks terminal day",
                name, Agegrp::names[a + 1]);
      for (size_t d = 1; d <= DURATIONLIM; ++d) {
        const int16_t e = tree.entry_index[a][d];
        if (e == NO_PROGRESSION_ENTRY) continue;
        if (e < 0 || static_cast<size_t>(e) >= tree.entries.size()) { ++rows_bad; continue; }
        for (const auto& row : tree.entries[e]) {
          float sum = 0;
          for (float x : row) { sum += x; rows_bad += x < 0.0f || x > 1.0f; }
          rows_bad += std::abs(sum - 1.0f) > 1e-5f;
        }
      }
    }
    CHECK_MSG(rows_bad == 0, "{}: {} invalid progression rows or indices", name, rows_bad);

    // a derived (null-tree) variant is base x riskadjust, renormalized
    if (body["progression_tree"].is_null() && !ps.progression[v].factors.riskadjust.empty()) {
      const auto& ra = ps.progression[v].factors.riskadjust;
      const auto& btree = ps.progression[1].tree;
      const int16_t e = btree.entry_index[zidx(AGE60_79)][5];
      REQUIRE(e != NO_PROGRESSION_ENTRY);
      auto row = btree.entries[e][NIL.v - 2];
      float sum = 0;
      for (size_t i = 0; i < row.size(); ++i) sum += (row[i] *= ra[i]);
      for (size_t i = 0; i < row.size(); ++i)
        CHECK_NEAR(tree.entries[e][NIL.v - 2][i], row[i] / sum, 1e-5);
    }
  }
}

void test_vaccines_contract_for_simulation() {
  test_support::RuntimeNamesGuard guard;
  const auto paths = test_support::sample_paths();
  (void)load_infect_params(paths.variants);  // vaccines are ordered by Variant::names
  const VaxSet vs = load_vax_data(paths.vaccines, true);
  const json jv = json::parse(vaccines_json);
  CHECK_EQ(vs.params.size(), vs.names.size());
  CHECK_EQ(vs.size(), size_t{Vax::count});
  CHECK_EQ(vs.size(), jv.size());
  CHECK_THROWS(std::runtime_error, (void)vs.at(Vax{0}), "Invalid vaccine lookup");

  for (size_t v = 1; v < vs.params.size(); ++v) {
    const auto& name = vs.names[v];
    const auto& p = vs.params[v];
    const auto& jb = jv[name];
    CHECK_EQ(p.reqdshots, jb["reqdshots"].get<int>());
    CHECK_EQ(p.delay2ndshot, jb["delay2ndshot"].get<int>());
    CHECK_EQ(p.halflife, jb["halflife"].get<int>());
    CHECK_EQ(p.full_effect_days, jb["full_effect_days"].get<int>());
    // vaxeffect reads infectfactor[variant]: indexed by Variant id, [0] unused
    REQUIRE(p.infectfactor.size() == Variant::names.size());
    for (size_t var = 1; var < Variant::names.size(); ++var) {
      CHECK_NEAR(p.infectfactor[var], jb["infectfactor"][Variant::names[var]].get<float>(), 1e-6);
    }
    // vaxeffect reads effectiveness[vaxstatus][variant]: indexed by trait ids, row/column 0 unused
    REQUIRE(p.effectiveness.size() == Vaxstatus::names.size());
    for (size_t shot = 1; shot < Vaxstatus::names.size(); ++shot) {
      REQUIRE(p.effectiveness[shot].size() == Variant::names.size());
      for (size_t var = 1; var < Variant::names.size(); ++var) {
        const float want = shot == Vaxstat::exhausted.v
                               ? 0.0f  // exhausted: 0 by definition, never supplied
                               : jb["effectiveness"][Vaxstatus::names[shot]][Variant::names[var]].get<float>();
        CHECK_NEAR(p.effectiveness[shot][var], want, 1e-6);
      }
    }
  }
}

void test_vax_schedules_contract_for_simulation() {
  test_support::RuntimeNamesGuard guard;
  const auto paths = test_support::sample_paths();
  (void)load_infect_params(paths.variants);
  const VaxSet vs = load_vax_data(paths.vaccines, true);
  VaxSchedSet ss = load_vax_sched_set(paths.vax_sched_dir, true);
  REQUIRE(ss.size() == 2);
  CHECK_EQ(ss.schedules[0].first, string("loc38015_old"));  // sorted by file stem
  CHECK_EQ(ss.schedules[1].first, string("loc38015_young"));

  for (auto& [sname, sched] : ss.schedules) {
    float mix = 0;
    for (const auto& spec : sched.vaxesincluded) {
      CHECK_MSG(spec.vax.v >= 1 && spec.vax.v <= Vax::count, "{}: invalid vax {}", sname, spec.vax.v);
      CHECK_EQ(spec.doses, spec.starting_doses);
      CHECK_MSG(spec.pct2ndshot >= 0 && spec.pct2ndshot <= 1 && spec.pctboost >= 0 && spec.pctboost <= 1,
                "{} {}: pct2ndshot {} pctboost {}", sname, spec.vax.show(), spec.pct2ndshot, spec.pctboost);
      for (const auto& alt : spec.alternate)
        CHECK_MSG(alt.v >= 1 && alt.v <= Vax::count && alt != spec.vax, "{}: bad alternate {}", sname, alt.v);
      mix += spec.mix;
    }
    CHECK_NEAR(mix, 1.0, 1e-5);
    CHECK_MSG(sched.dayrange.first >= 1 && sched.dayrange.first <= sched.dayrange.second, "{}: dayrange {}-{}",
              sname, sched.dayrange.first, sched.dayrange.second);
    CHECK_MSG(sched.targetpct > 0 && sched.targetpct <= 1, "{}: targetpct {}", sname, sched.targetpct);
    CHECK_MSG(!sched.filtervec.empty() && std::ranges::count(sched.filtervec, UNKNOWN) == 0,
              "{}: filtervec empty or has unknown ages", sname);
    // daily fractions over the window add up to targetpct (pattern is normalized), are
    // never negative, and are zero after the window (2nd-shot extension days)
    double total = 0;
    bool negative = false;
    for (int d = sched.dayrange.first; d <= sched.dayrange.second; ++d) {
      const float f = sched.spreadfunc(d);
      negative |= f < 0;
      total += f;
    }
    CHECK_MSG(!negative, "{}: negative daily fraction", sname);
    CHECK_MSG(std::abs(total / sched.targetpct - 1.0) < 0.02,
              "{}: daily fractions sum to {:.4f}, want targetpct {:.4f}", sname, total, sched.targetpct);
    CHECK_NEAR(sched.spreadfunc(sched.dayrange.second + 10), 0.0, 1e-9);
  }
}

void test_rings_contract_for_simulation() {
  test_support::RuntimeNamesGuard guard;
  const json jr = json::parse(rings_json)["rings"];
  const RingTraits rt = load_ring_traits(test_support::sample_paths().rings, true);
  REQUIRE(rt.ring_count() == jr.size());
  CHECK_EQ(size_t{Ring::count}, jr.size());
  CHECK_EQ(rt.pct_of_population.size(), jr.size() + 1);
  CHECK_EQ(rt.pct_of_population[0], 0.0f);
  for (size_t r = 1; r <= jr.size(); ++r) {
    const auto& e = jr[r - 1];
    CHECK_EQ(Ring::names[r], e["name"].get<string>());
    CHECK_NEAR(rt.pct_of_population[r], e["pct_of_population"].get<float>(), 1e-6);
    REQUIRE(rt.out_ring_prob[r].size() == Agegrp::names.size());  // indexed by Agegrp value
    CHECK_EQ(rt.out_ring_prob[r][0], 0.0f);
    for (size_t a = 1; a < Agegrp::names.size(); ++a)
      CHECK_NEAR(rt.out_ring_prob[r][a], e["out_ring_prob_by_agegrp"][Agegrp::names[a]].get<float>(), 1e-6);
  }
  const RingTraits off = load_ring_traits("", false);
  CHECK_EQ(off.ring_count(), size_t{0});
  CHECK_EQ(Ring::count, uint16_t{1});
}

// spread indexes contactfactors[cond - 2][agegrp - 1] and touchfactors[touch_map][agegrp - 1];
// the row labels must line up with those index formulas, and every cell must come from the
// matching JSON entry (the loader transposes age-major JSON to row-major arrays).
void test_social_params_contract_for_simulation() {
  const SocialParams sp = load_social_params(test_support::sample_paths().social);
  const json js = json::parse(socialparams_json);
  for (size_t i = 0; i < sp.contact_rows.size(); ++i) CHECK_EQ(sp.contact_rows[i], Condition::names[i + 2]);
  CHECK_EQ(sp.touch_rows[0], Status::names[UNEXPOSED.v]);
  CHECK_EQ(sp.touch_rows[1], Status::names[RECOVERED.v]);
  for (size_t i = 2; i < sp.touch_rows.size(); ++i) CHECK_EQ(sp.touch_rows[i], Condition::names[i]);
  for (size_t c = 0; c < sp.age_columns.size(); ++c) CHECK_EQ(sp.age_columns[c], Agegrp::names[c + 1]);
  int bad = 0;
  for (size_t r = 0; r < sp.contact_rows.size(); ++r)
    for (size_t c = 0; c < sp.age_columns.size(); ++c)
      bad += std::abs(sp.contactfactors[r][c] -
                      js["contactfactors"][sp.age_columns[c]][sp.contact_rows[r]].get<float>()) > 1e-6f;
  for (size_t r = 0; r < sp.touch_rows.size(); ++r)
    for (size_t c = 0; c < sp.age_columns.size(); ++c)
      bad += std::abs(sp.touchfactors[r][c] -
                      js["touchfactors"][sp.age_columns[c]][sp.touch_rows[r]].get<float>()) > 1e-6f;
  CHECK_MSG(bad == 0, "{} social factor cells differ from socialparams.json", bad);
  CHECK_NEAR(sp.gammashape, js["gammashape"].get<float>(), 1e-6);
  CHECK_NEAR(sp.indoor_uplift, js["indoor_uplift"].get<float>(), 1e-6);
}

// social distancing cases: factors rescaled into [delta0, delta1], order preserved
void test_social_distancing_cases_load() {
  test_support::RuntimeNamesGuard guard;
  const SocialParams sp = load_social_params(test_support::sample_paths().social);
  const json jd = json::parse(social_dist_json);
  const auto cases = load_sd_cases(jd, sp);
  REQUIRE(cases.size() == jd.size());
  CHECK_EQ(SDCase::names.size(), jd.size() + 1);
  for (size_t k = 0; k < cases.size(); ++k) {
    const auto& c = cases[k];
    CHECK_EQ(c.name, jd[k]["name"].get<string>());
    CHECK_EQ(SDCase::names[k + 1], c.name);
    CHECK_EQ(c.startday, jd[k]["startday"].get<int>());
    CHECK_EQ(c.endday, jd[k]["endday"].get<int>());
    CHECK_EQ(c.include_ages.size(), jd[k]["include_ages"].size());
    float cmin = 99, cmax = -99, tmin = 99, tmax = -99;
    int order_bad = 0;
    for (size_t r = 0; r < 4; ++r)
      for (size_t a = 0; a < 5; ++a) {
        cmin = std::min(cmin, c.contactfactors[r][a]);
        cmax = std::max(cmax, c.contactfactors[r][a]);
        if (a > 0) order_bad += (sp.contactfactors[r][a] > sp.contactfactors[r][a - 1]) !=
                                (c.contactfactors[r][a] > c.contactfactors[r][a - 1]);
      }
    for (const auto& row : c.touchfactors)
      for (float x : row) { tmin = std::min(tmin, x); tmax = std::max(tmax, x); }
    CHECK_NEAR(cmin, c.contact_delta[0], 1e-5);
    CHECK_NEAR(cmax, c.contact_delta[1], 1e-5);
    CHECK_NEAR(tmin, c.touch_delta[0], 1e-5);
    CHECK_NEAR(tmax, c.touch_delta[1], 1e-5);
    CHECK_MSG(order_bad == 0, "case {}: rescaling changed the order of {} contact factors", c.name, order_bad);
  }
  CHECK_EQ(cases[1].include_ages[0], AGE60_79);
  CHECK_EQ(cases[1].include_ages[1], AGE80_UP);

  json one = jd;
  one[0].erase("endday");
  const auto no_end = load_sd_cases(one, sp);
  CHECK_EQ(no_end[0].endday, INT_MAX);
  one = jd;
  one[0]["include_ages"] = {"age99"};
  CHECK_THROWS(std::invalid_argument, (void)load_sd_cases(one, sp), "Unknown Agegrp");
  one = jd;
  one[0]["contact_delta"] = {0.1, 0.2, 0.3};
  CHECK_THROWS(std::runtime_error, (void)load_sd_cases(one, sp), "must have 2 elements");
  // comply outside [0,1] (was G3) and day order (was G4; endday == startday also never
  // clears because tagging and clearing are if / else-if): input_verify and the loader
  auto both_reject = [&](std::string_view what, const std::function<void(json&)>& edit,
                         std::string_view loader_msg, std::string_view verify_msg) {
    json bad = jd;
    edit(bad[0]);
    std::string got;
    try { (void)load_sd_cases(bad, sp); } catch (const std::exception& e) { got = e.what(); }
    CHECK_MSG(got.contains(loader_msg), "{}: loader error '{}' lacks '{}'", what, got, loader_msg);
    input_verify_detail::Errors e;
    input_verify_detail::check_soc_dist(bad, e);
    CHECK_MSG(has_error_containing(e, verify_msg), "{}: input_verify errors {} lack '{}'", what, e.msgs, verify_msg);
  };
  both_reject("comply 1.5", [](json& c) { c["comply"] = 1.5; }, "comply must be in [0, 1]", "'comply'");
  both_reject("comply -0.1", [](json& c) { c["comply"] = -0.1; }, "comply must be in [0, 1]", "'comply'");
  both_reject("startday > endday", [](json& c) { c["startday"] = 120; c["endday"] = 60; }, "startday < endday",
              "'startday' < 'endday'");
  both_reject("startday == endday", [](json& c) { c["startday"] = 60; c["endday"] = 60; }, "startday < endday",
              "'startday' < 'endday'");
  both_reject("startday 0", [](json& c) { c["startday"] = 0; }, "startday < endday", "'startday' < 'endday'");
  input_verify_detail::Errors ok;
  input_verify_detail::check_soc_dist(jd, ok);
  CHECK_MSG(!ok.any(), "template soc_dist rejected: {}", ok.msgs);
}

// seed cases: template parses to the expected terms; every malformed seed is rejected
// with a message naming the trigger day
void test_seed_cases_load_and_reject_malformed() {
  test_support::RuntimeNamesGuard guard;
  test_support::install_names({"base", "delta"});
  ModelParams mp;
  const json js = json::parse(seed_json);
  const auto cases = load_seed_cases(js, mp);
  REQUIRE(cases.size() == 2);
  CHECK_EQ(cases[0].triggerday, 1);
  CHECK(cases[0].startofday);
  REQUIRE(cases[0].filter.terms.size() == 1);
  CHECK_EQ(cases[0].filter.terms[0].trait, string("agegrp"));
  CHECK_EQ(cases[0].filter.terms[0].val, int32_t(AGE20_39.v));
  CHECK_EQ(cases[0].change.count, 3);
  CHECK_EQ(cases[1].filter.terms[0].val, int32_t(AGE40_59.v));

  auto expect_reject = [&](std::string_view what, const std::function<void(json&)>& edit,
                           std::string_view msg) {
    json s = js[0];
    edit(s);
    std::string got;
    try { (void)load_seed_case(s, mp); } catch (const std::exception& e) { got = e.what(); }
    CHECK_MSG(!got.empty() && got.find(msg) != string::npos, "{}: want error containing '{}', got '{}'",
              what, msg, got.empty() ? "<accepted>" : got);
  };
  auto set_term = [](json& s, const string& trait, json val) {
    for (auto& t : s["change"]["terms"]) if (t["trait"] == trait) t["val"] = val;
  };
  expect_reject("status recovered", [&](json& s) { set_term(s, "status", "recovered"); }, "must be 'infectious'");
  expect_reject("cond uninfected", [&](json& s) { set_term(s, "cond", "uninfected"); }, "cond must be");
  expect_reject("duration 0", [&](json& s) { set_term(s, "duration", 0); }, "duration must be in 1..");
  expect_reject("variant none", [&](json& s) { set_term(s, "variant", "none"); }, "Variant 'none'");
  expect_reject("unknown variant", [&](json& s) { set_term(s, "variant", "omega"); }, "Unknown variant");
  expect_reject("no variant", [&](json& s) {
    json keep = json::array();
    for (auto& t : s["change"]["terms"]) if (t["trait"] != "variant") keep.push_back(t);
    s["change"]["terms"] = keep; }, "must include status 'infectious' and a variant");
  expect_reject("vaxstatus change", [&](json& s) {
    s["change"]["terms"].push_back({{"trait", "vaxstatus"}, {"val", "full"}}); }, "Disallowed change trait");
  expect_reject("duplicate cond", [&](json& s) {
    s["change"]["terms"].push_back({{"trait", "cond"}, {"val", "mild"}}); }, "appears more than once");
  expect_reject("count 0", [&](json& s) { s["change"]["count"] = 0; }, "count must be >= 1");
  expect_reject("triggerday 0", [&](json& s) { s["triggerday"] = 0; }, "triggerday must be >= 1");
  expect_reject("filter on duration", [&](json& s) {
    s["filter"].push_back({{"trait", "duration"}, {"val", 1}}); }, "Disallowed filter trait");

  // built directly in code, the constructor enforces the same rules
  CHECK_THROWS(std::runtime_error,
               (SeedCase(1, true, Filter{}, Change{{{"status", INFECTIOUS.v}, {"variant", 9}}, 1})),
               "variant must be a loaded variant");

  // end-of-day seeds load (runsim runs them after spread and progression; was G5)
  json late = js[0];
  late["startofday"] = false;
  CHECK(!load_seed_case(late, mp).startofday);
}

// malformed variant inputs must fail at load time
void test_variant_inputs_rejected_at_load() {
  test_support::RuntimeNamesGuard guard;
  json j = json::parse(variants_json);
  j["base"]["spread"]["sendrisk"].erase(j["base"]["spread"]["sendrisk"].size() - 1);
  CHECK_THROWS(std::runtime_error, (void)load_variants_data(j), "sendrisk must have exactly 25 values");
  input_verify_detail::Errors e;
  input_verify_detail::check_variants(j, e);
  CHECK(has_error_containing(e, "'sendrisk' must have exactly 25 values"));

  j = json::parse(variants_json);
  j["base"]["spread"]["recvrisk"] = {0.1, 0.2, 0.3, 0.4};  // 4 ages: infectrisk would read past the end (was G1)
  CHECK_THROWS(std::runtime_error, (void)load_variants_data(j), "recvrisk must have exactly 5 values");
  input_verify_detail::Errors e2;
  input_verify_detail::check_variants(j, e2);
  CHECK(has_error_containing(e2, "'recvrisk' must have exactly 5 values"));
}

}  // namespace

void run_parameter_tests(const test_support::TestRunOptions& options) {
  using tst::run_test;
  run_test("model_params_loading", [&] { test_model_params_loading(options); });
  run_test("load_variants_data_rejects_non_base_primary", test_load_variants_data_rejects_non_base_primary);
  run_test("load_variants_data_rejects_empty_base_sendrisk", test_load_variants_data_rejects_empty_base_sendrisk);
  run_test("load_variants_data_rejects_empty_base_recvrisk", test_load_variants_data_rejects_empty_base_recvrisk);
  run_test("load_progression_set_rejects_row_not_summing_to_one", test_load_progression_set_rejects_row_not_summing_to_one);
  run_test("load_progression_set_rejects_wrong_row_length", test_load_progression_set_rejects_wrong_row_length);
  run_test("load_progression_set_maps_reordered_ages_and_packs_breakdays", test_load_progression_set_maps_reordered_ages_and_packs_breakdays);
  run_test("load_progression_set_requires_exact_real_age_names", test_load_progression_set_requires_exact_real_age_names);
  run_test("load_progression_set_rejects_out_of_range_breakday", test_load_progression_set_rejects_out_of_range_breakday);
  run_test("load_progression_set_requires_terminal_breakday", test_load_progression_set_requires_terminal_breakday);
  run_test("load_progression_set_requires_terminal_outcome", test_load_progression_set_requires_terminal_outcome);
  run_test("load_progression_set_requires_every_age_group", test_load_progression_set_requires_every_age_group);
  run_test("load_progression_set_rejects_wrong_riskadjust_length", test_load_progression_set_rejects_wrong_riskadjust_length);
  run_test("load_seed_case_rejects_duration_above_limit", test_load_seed_case_rejects_duration_above_limit);
  run_test("load_vax_sched_rejects_mix_not_summing_to_one", test_load_vax_sched_rejects_mix_not_summing_to_one);
  run_test("vax_schedule_age_aliases_and_fallback", test_vax_schedule_age_aliases_and_fallback);
  run_test("input_verify_accepts_template_inputs", test_input_verify_accepts_template_inputs);
  run_test("input_verify_reports_vaccine_variant_alignment_error", test_input_verify_reports_vaccine_variant_alignment_error);
  run_test("check_config_rejects_missing_days", test_check_config_rejects_missing_days);
  run_test("write_error_log_writes_complete_report", test_write_error_log_writes_complete_report);
  run_test("check_config_rejects_nonpositive_days", test_check_config_rejects_nonpositive_days);
  run_test("check_config_rejects_negative_rt_sim_interval", test_check_config_rejects_negative_rt_sim_interval);
  run_test("check_config_rejects_bad_calendar_start", test_check_config_rejects_bad_calendar_start);
  run_test("check_config_rejects_age_dist_wrong_size", test_check_config_rejects_age_dist_wrong_size);
  run_test("check_config_rejects_age_dist_bad_sum", test_check_config_rejects_age_dist_bad_sum);
  run_test("check_config_requires_vax_keys_when_dovax", test_check_config_requires_vax_keys_when_dovax);
  run_test("check_variants_rejects_empty_input", test_check_variants_rejects_empty_input);
  run_test("check_variants_rejects_non_base_first", test_check_variants_rejects_non_base_first);
  run_test("check_variants_rejects_wrong_progression_row_length", test_check_variants_rejects_wrong_progression_row_length);
  run_test("check_variants_rejects_progression_row_bad_sum", test_check_variants_rejects_progression_row_bad_sum);
  run_test("check_vaccines_rejects_missing_int_key", test_check_vaccines_rejects_missing_int_key);
  run_test("check_vaccine_variant_alignment_rejects_missing_factors", test_check_vaccine_variant_alignment_rejects_missing_factors);
  run_test("check_vax_sched_rejects_mix_bad_sum", test_check_vax_sched_rejects_mix_bad_sum);
  run_test("check_vax_sched_rejects_bad_dayrange_length", test_check_vax_sched_rejects_bad_dayrange_length);
  run_test("check_socialparams_rejects_missing_key", test_check_socialparams_rejects_missing_key);
  run_test("check_seed_rejects_entry_missing_key", test_check_seed_rejects_entry_missing_key);
  run_test("check_soc_dist_rejects_bad_contact_delta_length", test_check_soc_dist_rejects_bad_contact_delta_length);
  run_test("check_rings_rejects_pct_bad_sum", test_check_rings_rejects_pct_bad_sum);
  run_test("check_rings_rejects_missing_agegrp", test_check_rings_rejects_missing_agegrp);
  run_test("check_variants_rejects_case_duplicate", test_check_variants_rejects_case_duplicate);
  run_test("check_variants_rejects_reserved_none", test_check_variants_rejects_reserved_none);
  run_test("check_vaccines_rejects_case_duplicate", test_check_vaccines_rejects_case_duplicate);
  run_test("check_rings_rejects_duplicate_name", test_check_rings_rejects_duplicate_name);
  run_test("input_verify_reports_exact_duplicate_variant_key", test_input_verify_reports_exact_duplicate_variant_key);
  run_test("variants_contract_for_simulation", test_variants_contract_for_simulation);
  run_test("vaccines_contract_for_simulation", test_vaccines_contract_for_simulation);
  run_test("vax_schedules_contract_for_simulation", test_vax_schedules_contract_for_simulation);
  run_test("rings_contract_for_simulation", test_rings_contract_for_simulation);
  run_test("social_params_contract_for_simulation", test_social_params_contract_for_simulation);
  run_test("social_distancing_cases_load", test_social_distancing_cases_load);
  run_test("seed_cases_load_and_reject_malformed", test_seed_cases_load_and_reject_malformed);
  run_test("variant_inputs_rejected_at_load", test_variant_inputs_rejected_at_load);
}
