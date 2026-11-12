// pop_serialize: rendering of PopData rows/columns, compared with golden files in
// test/fixtures/pop_serialize (reviewed by hand when created). On a mismatch the actual
// output is written to test_output/pop_serialize/actual_<file>.
#include "test_support.h"
#include "../src/pop_serialize.h"

namespace {

constexpr std::string_view GROUP = "pop_serialize";
using test_support::RuntimeNamesGuard;

void test_pretty_and_serialized_match_golden(const test_support::TestRunOptions& options) {
  RuntimeNamesGuard guard;
  PopData pop = test_support::make_popdata_fixture();
  const vector<size_t> rows = {1, 2, 3, 4};

  std::ostringstream compact;
  write_pop_data(pop, rows, {"status", "agegrp", "cond", "duration", "ring"}, OutSpec(compact), Style::pretty,
                 false, ",", false);
  test_support::compare_golden(compact.str(), "pop_serialize/pretty_compact.txt", options, GROUP);

  std::ostringstream multi;
  pop_print(pop, rows, {"variant", "variant_hist", "sickday", "sickday_hist", "vax_hist", "vaxday_hist"},
            OutSpec(multi));
  test_support::compare_golden(multi.str(), "pop_serialize/pretty_multivalue.txt", options, GROUP);

  std::ostringstream all;
  write_pop_data(pop, rows, "all", OutSpec(all), Style::serialized, false, ",", true);
  test_support::compare_golden(all.str(), "pop_serialize/serialized_all.csv", options, GROUP);
}

// selection: only the requested rows, in the requested order, with a header
void test_row_selection_and_order() {
  RuntimeNamesGuard guard;
  PopData pop = test_support::make_popdata_fixture();
  std::ostringstream out;
  write_pop_data(pop, vector<size_t>{3, 1}, {"status", "variant", "sickday"}, OutSpec(out), Style::serialized,
                 false, ",", true);
  const auto lines = test_support::split_trimmed_lines(out.str());
  CHECK_EQ(lines, (vector<string>{"row,status,variant,sickday", "3,unexposed,none,-", "1,recovered,alpha,2"}));
}

void test_pop_to_csv_writes_file_and_escapes_cells() {
  RuntimeNamesGuard guard;
  test_support::install_names({"delta,epsilon"});
  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
  pop.variant[1] = Variant{1};
  const auto dir = test_support::fs::temp_directory_path() / test_support::unique_name("epi_pop_out_");
  test_support::fs::create_directories(dir);
  const auto path = dir / "escaped.csv";
  pop_to_csv(pop, vector<size_t>{1}, {"variant"}, OutSpec(path));
  const string content = test_support::read_file_text(path);
  CHECK(content.contains("row,variant"));
  CHECK(content.contains("\"delta,epsilon\""));
  test_support::fs::remove_all(dir);
}

void test_rejects_bad_rows_and_columns() {
  RuntimeNamesGuard guard;
  PopData pop = test_support::make_popdata_fixture();
  std::ostringstream out;
  CHECK_THROWS(std::invalid_argument,
               write_pop_data(pop, vector<size_t>{0, 1}, {"status"}, OutSpec(out), Style::pretty, false, ",", false),
               "");
  CHECK_THROWS(std::invalid_argument,
               write_pop_data(pop, vector<size_t>{1, pop.popn + 1}, {"status"}, OutSpec(out), Style::pretty, false,
                              ",", false),
               "");
  CHECK_THROWS(std::invalid_argument,
               write_pop_data(pop, vector<size_t>{1}, {"does_not_exist"}, OutSpec(out), Style::pretty, false, ",",
                              false),
               "");
  CHECK_THROWS(std::invalid_argument, (void)render_pop_cell("does_not_exist", pop.agent(1)), "");
}

// every column renders the value stored in PopData (spot values)
void test_render_pop_cell_covers_every_column() {
  RuntimeNamesGuard guard;
  PopData pop = test_support::make_popdata_fixture();
  auto cell = [&](std::string_view col, size_t row) { return render_pop_cell(col, pop.agent(row)); };
  const vector<std::tuple<string, size_t, string>> expect{
      {"status", 1, "recovered"},   {"status", 4, "dead"},          {"agegrp", 2, "age40_59"},
      {"cond", 2, "mild"},          {"duration", 2, "5"},           {"variant", 2, "delta"},
      {"variant_hist", 2, "alpha|delta"}, {"variant_hist", 3, "-"}, {"sickday", 3, "-"},
      {"sickday_hist", 2, "4|11"},  {"recovday", 1, "9"},           {"recovday", 2, "-"},
      {"deadday", 4, "22"},         {"ring", 1, "population"},      {"ring", 2, "r3"},
      {"sdcase", 1, "false"},       {"sdcase", 4, "true"},          {"testday_hist", 2, "6|12"},
      {"quar", 2, "true"},          {"quarday", 2, "8"},            {"vaxstatus", 1, "none"},
      {"vaxstatus", 2, "booster"},  {"vax", 1, "-"},                {"vax", 2, "moderna"},
      {"vax_hist", 2, "pfizer|moderna"}, {"vaxday", 2, "14"},       {"vaxday_hist", 2, "5|14"}};
  for (const auto& [col, row, want] : expect)
    CHECK_MSG(cell(col, row) == want, "column {} row {}: '{}' want '{}'", col, row, cell(col, row), want);
  CHECK_EQ(get_all_column_names().size(), size_t(ColumnName::COUNT));
}

}  // namespace

void run_pop_serialize_tests(const test_support::TestRunOptions& options) {
  using tst::run_test;
  run_test("pretty_and_serialized_match_golden", [&] { test_pretty_and_serialized_match_golden(options); });
  run_test("row_selection_and_order", test_row_selection_and_order);
  run_test("pop_to_csv_escapes_cells", test_pop_to_csv_writes_file_and_escapes_cells);
  run_test("rejects_bad_rows_and_columns", test_rejects_bad_rows_and_columns);
  run_test("render_pop_cell_covers_every_column", test_render_pop_cell_covers_every_column);
}
