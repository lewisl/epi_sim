#include "test_support.h"

namespace {

constexpr std::string_view GROUP = "pop_serialize";

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

}  // namespace

void run_pop_serialize_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running pop_serialize tests...");
  test_write_pop_data_pretty_compact_to_stream(options);
  test_pop_print_multi_value_output(options);
  test_write_pop_data_serialized_to_stream(options);
  test_pop_to_csv_writes_file_and_escapes_cells(options);
  test_write_pop_data_rejects_bad_rows_and_columns();
  if (options.write_artifacts) {
    fmt::println("pop_serialize artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("pop_serialize tests passed.");
}
