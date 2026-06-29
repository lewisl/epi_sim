#include "test_support.h"

#include "../src/series.h"

namespace {

constexpr std::string_view GROUP = "series";

AllSeries make_series(size_t day_cnt) {
  Variant::names = {"none", "base", "delta"};
  Vax::names = {"none", "pfizer", "moderna"};
  PopData pop(3, {0.0, 1.0, 0.0, 0.0, 0.0});
  size_t n_ring_slots = std::max<size_t>(Ring::names.size(), 1);
  return AllSeries(day_cnt, pop, Variant::names.size(), Vax::names.size(), n_ring_slots);
}

struct SeriesCsvOutput {
  test_support::fs::path csv_path;
  test_support::fs::path cleanup_dir;
};

SeriesCsvOutput make_series_csv_output(string_view stem) {
  const auto temp_dir = test_support::fs::temp_directory_path() /
                        fmt::format("epi_sim_series_{}_{}", stem, std::random_device{}());
  return {.csv_path = temp_dir / fmt::format("{}.csv", stem),
          .cleanup_dir = temp_dir};
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

  const auto output = make_series_csv_output("series_unit");

  serialize_selected_series({{"now_unexposed", "total"},
                             {"now_vaccinated", "total"},
                             {"now_variant:delta", "total"}},
                            series, output.csv_path);

  CHECK(test_support::fs::exists(output.csv_path));
  const auto lines = test_support::split_trimmed_lines(test_support::read_file_text(output.csv_path));
  REQUIRE(lines.size() >= 4);
  CHECK(lines[0] == "now_unexposed:total,now_vaccinated:total,now_variant:delta:total");
  CHECK(lines[1] == "3,0,0");
  CHECK(lines[2] == "1,3,2");
  CHECK(lines[3] == "0,0,0");

  test_support::fs::remove_all(output.cleanup_dir);
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

void test_no_rings_identity() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names.clear();
  AllSeries series = make_series(3);

  // With no rings defined, n_ring_slots == 1 and the only slot is RING_ALL.
  // The inner double-count guard in update() ensures writes land exactly once.
  series.now_status.update(uint8_t(INFECTIOUS), RING_ALL, AGE20_39, 1, 2);
  series.now_status.update(uint8_t(INFECTIOUS), RING_ALL, AGE40_59, 1, 3);

  CHECK(series.now_status.n_rings == 1);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::age20_39)[1] == 2);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::age40_59)[1] == 3);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[1] == 5);
}

void test_aggregate_equals_sum_of_rings() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(3);

  series.now_status.update(uint8_t(INFECTIOUS), 1, AGE20_39, 1, 4);
  series.now_status.update(uint8_t(INFECTIOUS), 2, AGE20_39, 1, 5);
  series.now_status.update(uint8_t(INFECTIOUS), 2, AGE40_59, 1, 7);

  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::age20_39, 1)[1] == 4);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::age20_39, 2)[1] == 5);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::age40_59, 2)[1] == 7);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::age20_39, RING_ALL)[1] == 9);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::age40_59, RING_ALL)[1] == 7);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total, RING_ALL)[1] == 16);
}

void test_no_double_count_in_aggregate() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(2);

  series.now_status.update(uint8_t(INFECTIOUS), 1, AGE20_39, 1, 1);

  // RING_ALL gets the mirror-write exactly once.
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::age20_39, RING_ALL)[1] == 1);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total, RING_ALL)[1] == 1);

  int per_ring_sum = 0;
  for (uint8_t r = 1; r < series.now_status.n_rings; ++r)
    per_ring_sum += series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total, r)[1];
  CHECK(per_ring_sum == series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total, RING_ALL)[1]);
}

void test_resolve_series_with_ring_arg() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(2);

  series.now_status.update(uint8_t(INFECTIOUS), 1, AGE20_39, 1, 3);
  series.now_status.update(uint8_t(INFECTIOUS), 2, AGE20_39, 1, 5);

  const auto r1  = resolve_series(series, "now_infectious", AgeBucket::total, 1);
  const auto r2  = resolve_series(series, "now_infectious", AgeBucket::total, 2);
  const auto all = resolve_series(series, "now_infectious", AgeBucket::total);  // default RING_ALL

  REQUIRE(r1.has_value());
  REQUIRE(r2.has_value());
  REQUIRE(all.has_value());
  CHECK((*r1)[1] == 3);
  CHECK((*r2)[1] == 5);
  CHECK((*all)[1] == 8);
}

void test_ring_qualified_selection_resolves_to_ring() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(2);

  series.now_status.update(uint8_t(INFECTIOUS), 1, AGE20_39, 1, 3);
  series.now_status.update(uint8_t(INFECTIOUS), 2, AGE20_39, 1, 5);

  // ring_id_from_token covers name, decimal, and empty paths.
  CHECK(ring_id_from_token("") == std::optional<uint8_t>{RING_ALL});
  CHECK(ring_id_from_token("ring_1") == std::optional<uint8_t>{1});
  CHECK(ring_id_from_token("2") == std::optional<uint8_t>{2});
  CHECK(!ring_id_from_token("nope").has_value());

  // End-to-end: a ring-qualified SeriesSelection drives a ring-specific CSV column.
  const auto output = make_series_csv_output("series_ring");

  serialize_selected_series({{"now_infectious", "total", "ring_1"},
                             {"now_infectious", "total", "ring_2"}},
                            series, output.csv_path);

  CHECK(test_support::fs::exists(output.csv_path));
  const auto lines = test_support::split_trimmed_lines(test_support::read_file_text(output.csv_path));
  REQUIRE(lines.size() >= 2);
  CHECK(lines[0] == "now_infectious:total:ring_1,now_infectious:total:ring_2");
  CHECK(lines[1] == "3,5");

  test_support::fs::remove_all(output.cleanup_dir);
}

void test_bare_selection_resolves_to_aggregate() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(2);

  series.now_status.update(uint8_t(INFECTIOUS), 1, AGE20_39, 1, 3);
  series.now_status.update(uint8_t(INFECTIOUS), 2, AGE20_39, 1, 5);

  // Bare selection (no ring) goes to the RING_ALL aggregate slot; header has no ring suffix.
  const auto output = make_series_csv_output("series_bare");

  serialize_selected_series({{"now_infectious", "total"}},
                            series, output.csv_path);

  CHECK(test_support::fs::exists(output.csv_path));
  const auto lines = test_support::split_trimmed_lines(test_support::read_file_text(output.csv_path));
  REQUIRE(lines.size() >= 2);
  CHECK(lines[0] == "now_infectious:total");
  CHECK(lines[1] == "8");

  test_support::fs::remove_all(output.cleanup_dir);
}

void test_mixed_valid_invalid_selection_drops_invalid_column() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(2);

  series.now_status.update(uint8_t(INFECTIOUS), 1, AGE20_39, 1, 3);
  series.now_status.update(uint8_t(INFECTIOUS), 2, AGE20_39, 1, 5);

  // A spec mixing a resolvable selection with an unknown one: the invalid
  // column is dropped (with a warning) and the valid column is still written.
  const auto output = make_series_csv_output("series_mixed");

  serialize_selected_series({{"now_infectious", "total"},
                             {"now_variant:missing", "total"}},
                            series, output.csv_path);

  CHECK(test_support::fs::exists(output.csv_path));
  const auto lines = test_support::split_trimmed_lines(test_support::read_file_text(output.csv_path));
  REQUIRE(lines.size() >= 2);
  CHECK(lines[0] == "now_infectious:total");
  CHECK(lines[1] == "8");

  test_support::fs::remove_all(output.cleanup_dir);
}

void test_parse_ring_suffix() {
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};

  const auto bare = parse_ring_suffix("now_infectious");
  REQUIRE(bare.has_value());
  CHECK(bare->base_name == "now_infectious");
  CHECK(bare->ring == RING_ALL);

  const auto named = parse_ring_suffix("now_infectious@ring:ring_1");
  REQUIRE(named.has_value());
  CHECK(named->base_name == "now_infectious");
  CHECK(named->ring == 1);

  const auto indexed = parse_ring_suffix("now_infectious@ring:2");
  REQUIRE(indexed.has_value());
  CHECK(indexed->base_name == "now_infectious");
  CHECK(indexed->ring == 2);

  CHECK(!parse_ring_suffix("now_infectious@ring:missing").has_value());
  CHECK(!parse_ring_suffix("now_infectious@ring:").has_value());
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
  summary << "first selection: " << all_total.selections.front().name << ":"
          << all_total.selections.front().bucket << "\n";
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
  test_no_rings_identity();
  test_aggregate_equals_sum_of_rings();
  test_no_double_count_in_aggregate();
  test_resolve_series_with_ring_arg();
  test_ring_qualified_selection_resolves_to_ring();
  test_bare_selection_resolves_to_aggregate();
  test_mixed_valid_invalid_selection_drops_invalid_column();
  test_parse_ring_suffix();
  write_series_artifacts(options);
  if (options.write_artifacts) {
    fmt::println("series artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("series tests passed.");
}
