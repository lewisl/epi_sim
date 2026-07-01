#include "test_support.h"

#include "../src/setup.h"
#include "../src/sim.h"

namespace {

constexpr std::string_view GROUP = "pop_serialize";

// Mirrors test_setup's sample config but shrinks the run to 30 days so the
// end-of-run population shows meaningful state variation without the cost of a
// full production run. dovax/social-distancing/rings stay off: vaccination
// schedules start well after the 30-day window, so enabling them would add cost
// without populating vax columns.
Config simulation_config() {
  const auto paths = test_support::sample_paths();
  Config config;
  config.days = 30;
  config.locale = 38015;
  config.calendar_start = "2020-01-01";
  config.dovax = false;
  config.do_social_distancing = false;
  config.do_rings = false;
  config.age_dist = {0.251, 0.271, 0.255, 0.184, 0.039};
  config.seed = (test_support::project_dir() / "sample_parameters" / "seed_basic.json").string();
  config.geodata = paths.geodata;
  config.variants = paths.variants;
  config.social_params = paths.social;
  config.social_dist = (test_support::project_dir() / "sample_parameters" / "soc_dist.json").string();
  config.vaccines = paths.vaccines;
  config.vax_sched_dir = paths.vax_sched_dir;
  config.rings = (test_support::project_dir() / "sample_parameters" / "rings.json").string();
  return config;
}

void test_write_pop_data_pretty_compact_to_stream(const test_support::TestRunOptions& options) {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  PopData pop = test_support::make_popdata_fixture();
  const vector<size_t> rows = {1, 2, 3};

  std::ostringstream out;
  write_pop_data(pop, rows, {"status", "agegrp", "cond", "duration", "ring"},
                 OutSpec(out), Style::pretty, false, ",", false);
  test_support::write_artifact_text(options, GROUP, "pretty_compact.txt", out.str());

  const auto lines = test_support::split_trimmed_lines(out.str());
  const vector<string> expected = {
      "row     status      agegrp      cond        duration  ring",
      "----------------------------------------------------------",
      "     1  recovered   age20_39    uninfected  0         0",
      "     2  infectious  age40_59    mild        5         3",
      "     3  unexposed   age80_up    uninfected  0         0",
  };
  CHECK(lines == expected);
}

void test_pop_print_multi_value_output(const test_support::TestRunOptions& options) {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  PopData pop = test_support::make_popdata_fixture();
  const vector<size_t> rows = {1, 2, 3};

  std::ostringstream out;
  pop_print(pop, rows,
            {"variant", "variant_hist", "sickday", "sickday_hist",
             "recovday", "recovday_hist", "testday", "testday_hist"},
            OutSpec(out));
  test_support::write_artifact_text(options, GROUP, "pretty_multivalue.txt", out.str());

  const auto lines = test_support::split_trimmed_lines(out.str());
  REQUIRE(!lines.empty());
  CHECK(lines[0].find("variant") != string::npos);
  CHECK(lines[0].find("testday_hist") != string::npos);
  CHECK(std::find(lines.begin(), lines.end(), "     *              delta                  11                                             12") != lines.end());
}

void test_write_pop_data_serialized_to_stream(const test_support::TestRunOptions& options) {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  PopData pop = test_support::make_popdata_fixture();
  const vector<size_t> rows = {1, 2};

  std::ostringstream out;
  write_pop_data(pop, rows, {"status", "variant", "sickday"}, OutSpec(out),
                 Style::serialized, false, ",", true);
  test_support::write_artifact_text(options, GROUP, "serialized_stream.csv", out.str());

  const auto lines = test_support::split_trimmed_lines(out.str());
  const vector<string> expected = {
      "row,status,variant,sickday",
      "1,recovered,alpha,2",
      "2,infectious,delta,11",
  };
  CHECK(lines == expected);
}

void test_pop_to_csv_writes_file_and_escapes_cells(const test_support::TestRunOptions& options) {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none", "delta,epsilon"};

  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
  pop.status[1] = UNEXPOSED;
  pop.cond[1] = UNINFECTED;
  pop.variant[1] = Variant{1};

  test_support::fs::path out_dir;
  if (options.write_artifacts) {
    out_dir = test_support::artifact_group_dir(options, GROUP);
  } else {
    out_dir = test_support::fs::temp_directory_path() /
              fmt::format("epi_sim_pop_out_{}", std::random_device{}());
    test_support::fs::create_directories(out_dir);
  }
  const auto out_path = out_dir / "escaped_variant.csv";

  pop_to_csv(pop, vector<size_t>{1}, {"variant"}, OutSpec(out_path));

  CHECK(test_support::fs::exists(out_path));
  const string content = test_support::read_file_text(out_path);
  CHECK(content.find("row,variant") != string::npos);
  CHECK(content.find("\"delta,epsilon\"") != string::npos);

  if (!options.write_artifacts) test_support::fs::remove_all(out_dir);
}

void test_write_pop_data_rejects_bad_rows_and_columns() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  PopData pop = test_support::make_popdata_fixture();
  std::ostringstream out;

  bool bad_zero_row_threw = false;
  try {
    write_pop_data(pop, vector<size_t>{0, 1}, {"status"}, OutSpec(out),
                   Style::pretty, false, ",", false);
  } catch (const std::invalid_argument&) {
    bad_zero_row_threw = true;
  }
  CHECK(bad_zero_row_threw);

  bool bad_big_row_threw = false;
  try {
    write_pop_data(pop, vector<size_t>{1, pop.popn + 1}, {"status"}, OutSpec(out),
                   Style::pretty, false, ",", false);
  } catch (const std::invalid_argument&) {
    bad_big_row_threw = true;
  }
  CHECK(bad_big_row_threw);

  bool bad_column_threw = false;
  try {
    write_pop_data(pop, vector<size_t>{1}, {"does_not_exist"}, OutSpec(out),
                   Style::pretty, false, ",", false);
  } catch (const std::invalid_argument&) {
    bad_column_threw = true;
  }
  CHECK(bad_column_threw);
}

// Exercises the public single-cell renderer for every PopData column. Person 4
// is turned into a died agent so deadday/sdcase (left at defaults by the shared
// fixture) render meaningful values; person 2 already carries populated
// quarantine/vaccination/history state.
void test_render_pop_cell_covers_every_column() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::SDCaseNamesGuard sdcase_guard;
  test_support::RingNamesGuard ring_guard;
  PopData pop = test_support::make_popdata_fixture();

  pop.status[4] = DEAD;
  pop.deadday[4] = 22;
  pop.sdcase[4] = SDCase{1};

  auto cell = [&](std::string_view col, size_t row) {
    return render_pop_cell(col, pop.agent(row));
  };

  CHECK(cell("status", 1) == "recovered");
  CHECK(cell("status", 4) == "dead");
  CHECK(cell("agegrp", 1) == "age20_39");
  CHECK(cell("agegrp", 2) == "age40_59");
  CHECK(cell("cond", 1) == "uninfected");
  CHECK(cell("cond", 2) == "mild");
  CHECK(cell("duration", 1) == "0");
  CHECK(cell("duration", 2) == "5");
  CHECK(cell("variant", 1) == "alpha");
  CHECK(cell("variant", 2) == "delta");
  CHECK(cell("variant_hist", 1) == "alpha");
  CHECK(cell("variant_hist", 2) == "alpha|delta");
  CHECK(cell("variant_hist", 3) == "-");
  CHECK(cell("sickday", 2) == "11");
  CHECK(cell("sickday", 3) == "-");
  CHECK(cell("sickday_hist", 1) == "2");
  CHECK(cell("sickday_hist", 2) == "4|11");
  CHECK(cell("recovday", 1) == "9");
  CHECK(cell("recovday", 2) == "-");
  CHECK(cell("recovday_hist", 1) == "9");
  CHECK(cell("recovday_hist", 2) == "-");
  CHECK(cell("deadday", 1) == "0");
  CHECK(cell("deadday", 4) == "22");
  CHECK(cell("ring", 1) == "0");
  CHECK(cell("ring", 2) == "3");
  CHECK(cell("sdcase", 1) == "false");
  CHECK(cell("sdcase", 4) == "true");
  CHECK(cell("testday", 1) == "-");
  CHECK(cell("testday", 2) == "12");
  CHECK(cell("testday_hist", 1) == "-");
  CHECK(cell("testday_hist", 2) == "6|12");
  CHECK(cell("quar", 1) == "false");
  CHECK(cell("quar", 2) == "true");
  CHECK(cell("quarday", 1) == "0");
  CHECK(cell("quarday", 2) == "8");
  CHECK(cell("vaxstatus", 1) == "none");
  CHECK(cell("vaxstatus", 2) == "booster");
  CHECK(cell("vax", 1) == "-");
  CHECK(cell("vax", 2) == "moderna");
  CHECK(cell("vax_hist", 1) == "-");
  CHECK(cell("vax_hist", 2) == "pfizer|moderna");
  CHECK(cell("vaxday", 1) == "-");
  CHECK(cell("vaxday", 2) == "14");
  CHECK(cell("vaxday_hist", 1) == "-");
  CHECK(cell("vaxday_hist", 2) == "5|14");

  bool unknown_threw = false;
  try {
    (void)render_pop_cell("does_not_exist", pop.agent(1));
  } catch (const std::invalid_argument&) {
    unknown_threw = true;
  }
  CHECK(unknown_threw);
}

// set_output_file builds a timestamped CSV path and creates the file. Route it
// under a unique HOME subdirectory (as the scaffolding code does) so the test
// can clean up without touching the repo working tree.
void test_set_output_file_creates_timestamped_csv() {
  const std::string sub = test_support::unique_name("epi_sim_pop_setout_");
  const auto base_dir = test_support::home_dir() / sub;

  const auto out_path = set_output_file("popcheck", {sub});

  CHECK(test_support::fs::exists(out_path));
  CHECK(out_path.extension() == ".csv");
  CHECK(out_path.filename().string().rfind("popcheck_", 0) == 0);
  CHECK(out_path.parent_path() == base_dir);

  test_support::fs::remove_all(base_dir);
}

// Runs a short (30-day) headless simulation, then serializes representative
// agents with the "all" column sentinel so every column is rendered over real,
// varied end-of-run state. Deterministic: runsim seeds the RNG with a fixed value.
void test_serialize_all_columns_after_simulation(const test_support::TestRunOptions& options) {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Variant::names.clear();
  Vax::names.clear();

  Model model = setup_sim(simulation_config());
  model.headless = true;  // skip CSV writes and browser plots
  runsim(model);

  PopData& pop = model.pop;

  // The "all" sentinel must resolve to every defined column.
  const auto all_names = get_all_column_names();
  CHECK(all_names.size() == size_t(ColumnName::COUNT));

  // Pick one agent from each distinct end-state so the dump shows variation.
  // The infectious agent also carries populated variant/variant_hist ("base").
  std::optional<size_t> unexposed_row, infectious_row, recovered_row;
  for (size_t i = 1; i <= pop.popn; ++i) {
    if (!unexposed_row && pop.status[i] == UNEXPOSED) unexposed_row = i;
    if (!infectious_row && pop.status[i] == INFECTIOUS) infectious_row = i;
    if (!recovered_row && pop.status[i] == RECOVERED) recovered_row = i;
    if (unexposed_row && infectious_row && recovered_row) break;
  }
  REQUIRE(unexposed_row.has_value());
  REQUIRE(infectious_row.has_value());
  REQUIRE(recovered_row.has_value());

  const vector<size_t> rows = {*unexposed_row, *infectious_row, *recovered_row};

  // Serialized layout prints untruncated column keys, so use it to confirm every
  // column name is present in the header.
  std::ostringstream csv;
  write_pop_data(pop, rows, "all", OutSpec(csv), Style::serialized, false, ",", false);
  const auto csv_lines = test_support::split_trimmed_lines(csv.str());
  REQUIRE(!csv_lines.empty());
  for (const auto& name : all_names) {
    CHECK(csv_lines[0].find(name) != string::npos);
  }

  // The output reflects real, varied end-of-run state.
  const string body = csv.str();
  CHECK(body.find("unexposed") != string::npos);
  CHECK(body.find("infectious") != string::npos);
  CHECK(body.find("recovered") != string::npos);
  CHECK(body.find("base") != string::npos);  // base variant name from variants.json

  // Pretty "all" dump of the same agents, kept for human inspection.
  std::ostringstream pretty;
  write_pop_data(pop, rows, "all", OutSpec(pretty), Style::pretty, true, ",", false);
  test_support::write_artifact_text(options, GROUP, "all_columns_after_sim.txt", pretty.str());
}

}  // namespace

void run_pop_serialize_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running pop_serialize tests...");
  test_write_pop_data_pretty_compact_to_stream(options);
  test_pop_print_multi_value_output(options);
  test_write_pop_data_serialized_to_stream(options);
  test_pop_to_csv_writes_file_and_escapes_cells(options);
  test_write_pop_data_rejects_bad_rows_and_columns();
  test_render_pop_cell_covers_every_column();
  test_set_output_file_creates_timestamped_csv();
  test_serialize_all_columns_after_simulation(options);
  if (options.write_artifacts) {
    fmt::println("pop_serialize artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("pop_serialize tests passed.");
}
