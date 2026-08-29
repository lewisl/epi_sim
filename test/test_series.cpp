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

void test_column_map_uses_block_subject_ring_bucket_order() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(3);

  const auto& now_status = series.block_descriptor(SeriesBlock::now_status);
  const auto& new_status = series.block_descriptor(SeriesBlock::new_status);
  const auto& now_vax = series.block_descriptor(SeriesBlock::now_vax);

  CHECK(now_status.base_col == 0);
  CHECK(now_status.subject_count == Status::names.size());   // based on trait Status
  CHECK(now_status.ring_stride == size_t(AgeBucket::COUNT));
  CHECK(now_status.subject_stride == series.n_rings() * size_t(AgeBucket::COUNT));
  CHECK(now_status.column_count == Status::names.size() * now_status.subject_stride);
  CHECK(new_status.base_col == now_status.base_col + now_status.column_count);
  CHECK(now_vax.base_col == new_status.base_col + new_status.column_count);

  SeriesColumnIndex next_base = 0;
  for (const SeriesBlock block : all_series_blocks) {
    const auto& desc = series.block_descriptor(block);
    CHECK(desc.base_col == next_base);
    CHECK(desc.ring_stride == size_t(AgeBucket::COUNT));
    CHECK(desc.subject_stride == series.n_rings() * desc.ring_stride);
    CHECK(desc.column_count == desc.subject_count * desc.subject_stride);
    next_base += desc.column_count;
  }
  CHECK(series.column_count() == next_base);
  CHECK(series.block_descriptor(SeriesBlock::now_status).is_stock);
  CHECK(!series.block_descriptor(SeriesBlock::new_status).is_stock);
  CHECK(series.block_descriptor(SeriesBlock::now_vax).is_stock);
  CHECK(!series.block_descriptor(SeriesBlock::new_vax).is_stock);
  CHECK(series.block_descriptor(SeriesBlock::now_variant).is_stock);
  CHECK(!series.block_descriptor(SeriesBlock::new_variant).is_stock);

  const auto base = series.column_index(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                                        AgeBucket::total, RING_ALL);
  CHECK(series.column_index(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                            AgeBucket::age0_19, RING_ALL) == base + 1);
  CHECK(series.column_index(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                            AgeBucket::total, 1) == base + now_status.ring_stride);
  CHECK(series.column_index(SeriesBlock::now_status, uint8_t(RECOVERED),
                            AgeBucket::total, RING_ALL) == base + now_status.subject_stride);

  series.column(base)[1] = 11;
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::total)[1] == 11);
}

void test_day_one_seed_is_aggregate_only() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1"};
  AllSeries series = make_series(2);

  CHECK(series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED),
                  AgeBucket::total, RING_ALL)[1] == 3);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED),
                  AgeBucket::age20_39, RING_ALL)[1] == 3);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED),
                  AgeBucket::total, 1)[1] == 0);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED),
                  AgeBucket::age20_39, 1)[1] == 0);
}

void test_init_history_series_carries_forward_stock_series() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  AllSeries series = make_series(3);

  series.at(SeriesBlock::now_status, uint8_t(RECOVERED), AgeBucket::total)[1] = 1;
  series.at(SeriesBlock::now_status, uint8_t(RECOVERED), AgeBucket::age20_39)[1] = 1;
  series.at(SeriesBlock::now_vax, uint8_t(Vax{1}), AgeBucket::total)[1] = 2;
  series.at(SeriesBlock::now_vax, uint8_t(Vax{1}), AgeBucket::age20_39)[1] = 2;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{2}), AgeBucket::total)[1] = 1;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{2}), AgeBucket::age20_39)[1] = 1;
  series.at(SeriesBlock::new_status, uint8_t(RECOVERED), AgeBucket::total)[1] = 1;

  series.init_history_series(2);

  CHECK(series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[2] == 3);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(RECOVERED), AgeBucket::total)[2] == 1);
  CHECK(series.at(SeriesBlock::now_vax, uint8_t(Vax{1}), AgeBucket::total)[2] == 2);
  CHECK(series.at(SeriesBlock::now_variant, uint8_t(Variant{2}), AgeBucket::total)[2] == 1);
  CHECK(series.at(SeriesBlock::new_status, uint8_t(RECOVERED), AgeBucket::total)[2] == 0);
}

void test_resolve_series_supports_status_vaccinated_and_variant_views() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  AllSeries series = make_series(3);

  series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[1] = 3;
  series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[2] = 1;
  series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[3] = 0;
  series.at(SeriesBlock::now_vax, uint8_t(Vax{1}), AgeBucket::total)[2] = 1;
  series.at(SeriesBlock::now_vax, uint8_t(Vax{2}), AgeBucket::total)[2] = 2;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{2}), AgeBucket::total)[2] = 2;

  const auto resolved = resolve_selected_series(
      {{"now_unexposed", "total"},
        {"now_vaccinated", "total"},
        {"now_variant:delta", "total"},
        {"now_variant:missing", "total"},
        {"now_vax:none", "total"}},
      series);

  REQUIRE(resolved.cols.size() == 3);
  REQUIRE(resolved.invalid_selections.size() == 2);
  CHECK(resolved.cols[0].data[1] == 3 && resolved.cols[0].data[2] == 1 &&
        resolved.cols[0].data[3] == 0);
  CHECK(resolved.cols[1].data[2] == 3);
  CHECK(resolved.cols[2].data[2] == 2);
  CHECK(resolved.invalid_selections[0] == "now_variant:missing:total");
  CHECK(resolved.invalid_selections[1] == "now_vax:none:total");
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

  series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[1] = 3;
  series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[2] = 1;
  series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[3] = 0;
  series.at(SeriesBlock::now_vax, uint8_t(Vax{1}), AgeBucket::total)[2] = 1;
  series.at(SeriesBlock::now_vax, uint8_t(Vax{2}), AgeBucket::total)[2] = 2;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{2}), AgeBucket::total)[2] = 2;

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

  series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS), AgeBucket::total)[1] = 2;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{1}), AgeBucket::total)[1] = 1;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{2}), AgeBucket::total)[1] = 1;
  series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS), AgeBucket::total)[2] = 1;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{2}), AgeBucket::total)[2] = 1;
  series.validate_variant_invariant();

  series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS), AgeBucket::total)[2] = 2;
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
  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), RING_ALL,
                AGE20_39, 1, 2);
  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), RING_ALL,
                AGE40_59, 1, 3);

  CHECK(series.n_rings() == 1);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::age20_39)[1] == 2);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::age40_59)[1] == 3);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::total)[1] == 5);
}

void test_aggregate_equals_sum_of_rings() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(3);

  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 1, AGE20_39, 1, 4);
  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 2, AGE20_39, 1, 5);
  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 2, AGE40_59, 1, 7);

  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::age20_39, 1)[1] == 4);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::age20_39, 2)[1] == 5);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::age40_59, 2)[1] == 7);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::age20_39, RING_ALL)[1] == 9);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::age40_59, RING_ALL)[1] == 7);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::total, RING_ALL)[1] == 16);
}

void test_no_double_count_in_aggregate() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(2);

  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 1, AGE20_39, 1, 1);

  // RING_ALL gets the mirror-write exactly once.
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::age20_39, RING_ALL)[1] == 1);
  CHECK(series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                  AgeBucket::total, RING_ALL)[1] == 1);

  int per_ring_sum = 0;
  for (uint8_t r = 1; r < series.n_rings(); ++r) {
    per_ring_sum += series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                              AgeBucket::total, r)[1];
  }
  CHECK(per_ring_sum == series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS),
                                   AgeBucket::total, RING_ALL)[1]);
}

void test_resolve_series_with_ring_arg() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(2);

  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 1,
                AGE20_39, 1, 3);
  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 2,
                AGE20_39, 1, 5);

  const auto resolved = resolve_selected_series(
      {{"now_infectious", "total", "ring_1"},
       {"now_infectious", "total", "ring_2"},
       {"now_infectious", "total"}},
      series);

  REQUIRE(resolved.cols.size() == 3);
  CHECK(resolved.invalid_selections.empty());
  CHECK(resolved.cols[0].data[1] == 3);
  CHECK(resolved.cols[1].data[1] == 5);
  CHECK(resolved.cols[2].data[1] == 8);
}

void test_ring_qualified_selection_resolves_to_ring() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Ring::names = {"", "ring_1", "ring_2"};
  AllSeries series = make_series(2);

  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 1,
                AGE20_39, 1, 3);
  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 2,
                AGE20_39, 1, 5);

  // ring_id_from_token covers name, decimal, and empty paths.
  CHECK(ring_id_from_token("") == std::optional<uint8_t>{RING_ALL});
  CHECK(ring_id_from_token("ring_1") == std::optional<uint8_t>{1});
  CHECK(ring_id_from_token("2") == std::optional<uint8_t>{2});
  CHECK(!ring_id_from_token("nope").has_value());

  // End-to-end: a ring-qualified SeriesSelection drives a ring-specific CSV column.
  const auto output = make_series_csv_output("series_ring");

  serialize_selected_series({{"now_infectious", "total", "ring_1"},
                             {"now_infectious", "total", "2"}},
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

  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 1,
                AGE20_39, 1, 3);
  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 2,
                AGE20_39, 1, 5);

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

  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 1,
                AGE20_39, 1, 3);
  series.update(SeriesBlock::now_status, uint8_t(INFECTIOUS), 2,
                AGE20_39, 1, 5);

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

  series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[1] = 3;
  series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[2] = 1;
  series.at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[3] = 0;
  series.at(SeriesBlock::now_vax, uint8_t(Vax{1}), AgeBucket::total)[2] = 1;
  series.at(SeriesBlock::now_vax, uint8_t(Vax{2}), AgeBucket::total)[2] = 2;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{2}), AgeBucket::total)[2] = 2;

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
  const auto resolved = resolve_selected_series(SeriesColSpec(selections), series);
  REQUIRE(resolved.cols.size() == 3);
  for (size_t day = 1; day <= series.day_cnt; ++day) {
    csv << resolved.cols[0].data[day] << "," << resolved.cols[1].data[day]
        << "," << resolved.cols[2].data[day] << "\n";
  }

  series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS), AgeBucket::total)[1] = 2;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{1}), AgeBucket::total)[1] = 1;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{2}), AgeBucket::total)[1] = 1;
  series.at(SeriesBlock::now_status, uint8_t(INFECTIOUS), AgeBucket::total)[2] = 1;
  series.at(SeriesBlock::now_variant, uint8_t(Variant{2}), AgeBucket::total)[2] = 1;
  series.validate_variant_invariant();
  summary << "variant invariant sample: OK for 3-day fixture\n";

  test_support::write_artifact_text(options, GROUP, "series_summary.txt", summary.str());
  test_support::write_artifact_text(options, GROUP, "selected_series.csv", csv.str());
}

}  // namespace

void run_series_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running series tests...");
  test_column_map_uses_block_subject_ring_bucket_order();
  test_day_one_seed_is_aggregate_only();
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
