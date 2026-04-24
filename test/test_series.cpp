#include "test_support.h"

#include "../src/series.h"

namespace {

constexpr std::string_view GROUP = "series";

AllSeries make_series(size_t day_cnt) {
  Variant::names = {"none", "base", "delta"};
  Vax::names = {"none", "pfizer", "moderna"};
  PopData pop(3, {0.0, 1.0, 0.0, 0.0, 0.0});
  return AllSeries(day_cnt, pop, Variant::names.size(), Vax::names.size());
}

test_support::fs::path first_csv_in_dir(const test_support::fs::path& dir) {
  for (const auto& e : test_support::fs::directory_iterator(dir)) {
    if (e.is_regular_file() && e.path().extension() == ".csv") return e.path();
  }
  throw std::runtime_error("no csv in dir");
}

void test_init_history_series_carries_forward_stock_series() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  AllSeries series = make_series(3);

  series.now_status.at(uint8_t(RECOVERED), AgeBucket::total)[1] = 1;
  series.now_status.at(uint8_t(RECOVERED), AgeBucket::age20_39)[1] = 1;
  series.now_vax.at(uint8_t(Vax{1}), AgeBucket::total)[1] = 2;
  series.now_vax.at(uint8_t(Vax{1}), AgeBucket::age20_39)[1] = 2;
  series.now_variant.at(uint8_t(Variant{2}), AgeBucket::total)[1] = 1;
  series.now_variant.at(uint8_t(Variant{2}), AgeBucket::age20_39)[1] = 1;

  series.init_history_series(2);

  CHECK(series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[2] == 3);
  CHECK(series.now_status.at(uint8_t(RECOVERED), AgeBucket::total)[2] == 1);
  CHECK(series.now_vax.at(uint8_t(Vax{1}), AgeBucket::total)[2] == 2);
  CHECK(series.now_variant.at(uint8_t(Variant{2}), AgeBucket::total)[2] == 1);
}

void test_resolve_series_supports_status_vaccinated_and_variant_views() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  AllSeries series = make_series(3);

  series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[1] = 3;
  series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[2] = 1;
  series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[3] = 0;
  series.now_vax.at(uint8_t(Vax{1}), AgeBucket::total)[2] = 1;
  series.now_vax.at(uint8_t(Vax{2}), AgeBucket::total)[2] = 2;
  series.now_variant.at(uint8_t(Variant{2}), AgeBucket::total)[2] = 2;

  const auto unexposed = resolve_series(series, "now_unexposed", AgeBucket::total);
  const auto vaccinated = resolve_series(series, "now_vaccinated", AgeBucket::total);
  const auto delta = resolve_series(series, "now_variant:delta", AgeBucket::total);

  REQUIRE(unexposed.has_value());
  REQUIRE(vaccinated.has_value());
  REQUIRE(delta.has_value());
  CHECK((*unexposed)[1] == 3 && (*unexposed)[2] == 1 && (*unexposed)[3] == 0);
  CHECK((*vaccinated)[2] == 3);
  CHECK((*delta)[2] == 2);
  CHECK(!resolve_series(series, "now_variant:missing", AgeBucket::total).has_value());
}

void test_series_colspec_all_total_expands_current_runtime_names() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  AllSeries series = make_series(2);
  (void)series;

  SeriesColSpec spec("all", "total");
  const SeriesSelection expected_first{"now_unexposed", "total"};
  const SeriesSelection expected_vaccinated{"now_vaccinated", "total"};
  const SeriesSelection expected_pfizer{"new_vax:pfizer", "total"};
  const SeriesSelection expected_delta{"new_variant:delta", "total"};
  REQUIRE(spec.selections.size() == 18);
  CHECK(spec.selections.front() == expected_first);
  CHECK(std::find(spec.selections.begin(), spec.selections.end(), expected_vaccinated) != spec.selections.end());
  CHECK(std::find(spec.selections.begin(), spec.selections.end(), expected_pfizer) != spec.selections.end());
  CHECK(std::find(spec.selections.begin(), spec.selections.end(), expected_delta) != spec.selections.end());
}

void test_serialize_selected_series_writes_current_csv_layout() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  AllSeries series = make_series(3);

  series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[1] = 3;
  series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[2] = 1;
  series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[3] = 0;
  series.now_vax.at(uint8_t(Vax{1}), AgeBucket::total)[2] = 1;
  series.now_vax.at(uint8_t(Vax{2}), AgeBucket::total)[2] = 2;
  series.now_variant.at(uint8_t(Variant{2}), AgeBucket::total)[2] = 2;

  const string leaf = fmt::format("series_tmp_{}", std::random_device{}());
  const vector<string> path_steps = {"code", "epi_sim", "test_output", leaf};
  const auto out_dir = test_support::project_dir() / "test_output" / leaf;
  test_support::fs::create_directories(out_dir);

  serialize_selected_series({{"now_unexposed", "total"},
                             {"now_vaccinated", "total"},
                             {"now_variant:delta", "total"}},
                            series, "series_unit", path_steps);

  const auto csv_path = first_csv_in_dir(out_dir);
  const auto lines = test_support::split_trimmed_lines(test_support::read_file_text(csv_path));
  REQUIRE(lines.size() >= 4);
  CHECK(lines[0] == "now_unexposed:total,now_vaccinated:total,now_variant:delta:total");
  CHECK(lines[1] == "3,0,0");
  CHECK(lines[2] == "1,3,2");
  CHECK(lines[3] == "0,0,0");

  test_support::fs::remove_all(out_dir);
}

void test_validate_variant_invariant_checks_current_layout() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  AllSeries series = make_series(2);

  series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[1] = 2;
  series.now_variant.at(uint8_t(Variant{1}), AgeBucket::total)[1] = 1;
  series.now_variant.at(uint8_t(Variant{2}), AgeBucket::total)[1] = 1;
  series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[2] = 1;
  series.now_variant.at(uint8_t(Variant{2}), AgeBucket::total)[2] = 1;
  series.validate_variant_invariant();

  series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[2] = 2;
  bool threw = false;
  try {
    series.validate_variant_invariant();
  } catch (const std::runtime_error&) {
    threw = true;
  }
  CHECK(threw);
}

void write_series_artifacts(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;

  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  AllSeries series = make_series(3);

  series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[1] = 3;
  series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[2] = 1;
  series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[3] = 0;
  series.now_vax.at(uint8_t(Vax{1}), AgeBucket::total)[2] = 1;
  series.now_vax.at(uint8_t(Vax{2}), AgeBucket::total)[2] = 2;
  series.now_variant.at(uint8_t(Variant{2}), AgeBucket::total)[2] = 2;

  const std::vector<SeriesSelection> selections = {{"now_unexposed", "total"},
                                                   {"now_vaccinated", "total"},
                                                   {"now_variant:delta", "total"}};
  std::ostringstream summary;
  summary << "Series summary\n";
  summary << "==============\n\n";
  SeriesColSpec all_total("all", "total");
  summary << "all,total selection count: " << all_total.selections.size() << "\n";
  summary << "first selection: " << all_total.selections.front().first << ":"
          << all_total.selections.front().second << "\n";
  summary << "contains new_variant:delta:total: "
          << (std::find(all_total.selections.begin(), all_total.selections.end(),
                        SeriesSelection{"new_variant:delta", "total"}) != all_total.selections.end())
          << "\n\n";

  std::ostringstream csv;
  csv << "now_unexposed:total,now_vaccinated:total,now_variant:delta:total\n";
  const auto unexposed = resolve_series(series, "now_unexposed", AgeBucket::total).value();
  const auto vaccinated = resolve_series(series, "now_vaccinated", AgeBucket::total).value();
  const auto delta = resolve_series(series, "now_variant:delta", AgeBucket::total).value();
  for (size_t day = 1; day < unexposed.size(); ++day) {
    csv << unexposed[day] << "," << vaccinated[day] << "," << delta[day] << "\n";
  }

  series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[1] = 2;
  series.now_variant.at(uint8_t(Variant{1}), AgeBucket::total)[1] = 1;
  series.now_variant.at(uint8_t(Variant{2}), AgeBucket::total)[1] = 1;
  series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[2] = 1;
  series.now_variant.at(uint8_t(Variant{2}), AgeBucket::total)[2] = 1;
  series.validate_variant_invariant();
  summary << "variant invariant sample: OK for 3-day fixture\n";

  test_support::write_artifact_text(options, GROUP, "series_summary.txt", summary.str());
  test_support::write_artifact_text(options, GROUP, "selected_series.csv", csv.str());
}

}  // namespace

void run_series_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running series tests...");
  test_init_history_series_carries_forward_stock_series();
  test_resolve_series_supports_status_vaccinated_and_variant_views();
  test_series_colspec_all_total_expands_current_runtime_names();
  test_serialize_selected_series_writes_current_csv_layout();
  test_validate_variant_invariant_checks_current_layout();
  write_series_artifacts(options);
  if (options.write_artifacts) {
    fmt::println("series artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("series tests passed.");
}
