#include "test_support.h"

#include "../src/series.h"

namespace {

constexpr std::string_view GROUP = "series";

struct RuntimeNamesGuard {
  test_support::VariantNamesGuard variants;
  test_support::VaxNamesGuard vaccines;
  test_support::RingNamesGuard rings;
};

void install_names(size_t variant_count, size_t vax_count,
                   size_t ring_count) {
  Variant::names = {"none"};
  Vax::names = {"none"};
  Ring::names.clear();
  for (size_t i = 1; i <= variant_count; ++i) {
    Variant::names.push_back(fmt::format("variant_{}", i));
  }
  for (size_t i = 1; i <= vax_count; ++i) {
    Vax::names.push_back(fmt::format("vax_{}", i));
  }
  if (ring_count != 0) Ring::names.push_back("");
  for (size_t i = 1; i <= ring_count; ++i) {
    Ring::names.push_back(fmt::format("ring_{}", i));
  }
}

PopData make_pop(size_t ring_count = 0) {
  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
  if (ring_count != 0) {
    for (size_t person = 1; person <= pop.popn; ++person) {
      pop.ring[person] = static_cast<uint8_t>((person - 1) % ring_count + 1);
    }
  }
  return pop;
}

Histories make_histories(size_t day_count, size_t variant_count = 2,
                         size_t vax_count = 2, size_t ring_count = 0) {
  install_names(variant_count, vax_count, ring_count);
  auto pop = make_pop(ring_count);
  return Histories(day_count, pop, variant_count, vax_count, ring_count);
}

const TotalHistoryVector& only_history(
    const TotalHistorySet& resolved) {
  if (!resolved.invalid_selections.empty()) {
    fmt::println(stderr, "unexpected invalid selections: {}",
                 resolved.invalid_selections);
  }
  REQUIRE(resolved.invalid_selections.empty());
  REQUIRE(resolved.history_vectors.size() == 1);
  return resolved.history_vectors.front();
}

void test_atomic_layout_formula_and_introspection() {
  RuntimeNamesGuard guard;
  Histories histories = make_histories(2, 2, 2, 2);

  CHECK(HISTORY_AGE_COUNT == 5);
  CHECK(histories.ring_lane_count() == 2);
  CHECK(histories.phase_width(Trait::status) == 40);
  CHECK(histories.phase_width(Trait::vax) == 20);
  CHECK(histories.phase_width(Trait::variant) == 20);
  CHECK(histories.history_vector_count() == 160);

  CHECK(histories.history_vector_idx(
            Trait::status, Phase::now, uint8_t(UNEXPOSED), AGE0_19, 1) == 0);
  CHECK(histories.history_vector_idx(
            Trait::status, Phase::now, uint8_t(DEAD), AGE80_UP, 2) == 39);
  CHECK(histories.history_vector_idx(
            Trait::status, Phase::new_, uint8_t(UNEXPOSED), AGE0_19, 1) == 40);
  CHECK(histories.history_vector_idx(
            Trait::vax, Phase::now, 1, AGE0_19, 1) == 80);
  CHECK(histories.history_vector_idx(
            Trait::vax, Phase::new_, 1, AGE0_19, 1) == 100);
  CHECK(histories.history_vector_idx(
            Trait::variant, Phase::now, 1, AGE0_19, 1) == 120);
  CHECK(histories.history_vector_idx(
            Trait::variant, Phase::new_, 2, AGE80_UP, 2) == 159);

  for (size_t index = 0;
       index < histories.history_vector_count(); ++index) {
    const auto coordinates = histories.describe_history_vector(index);
    REQUIRE(coordinates.has_value());
    const auto parsed_trait = magic_enum::enum_cast<Trait>(coordinates->trait);
    const auto parsed_phase = magic_enum::enum_cast<Phase>(coordinates->phase);
    REQUIRE(parsed_trait.has_value());
    REQUIRE(parsed_phase.has_value());
    const auto trait = *parsed_trait;
    const auto phase = *parsed_phase;
    const auto value = trait == Trait::status
        ? uint8_t(*trait_from_string<Status>(coordinates->trait_value))
        : trait == Trait::vax
            ? uint8_t(*trait_from_string<Vax>(coordinates->trait_value))
            : uint8_t(*trait_from_string<Variant>(coordinates->trait_value));
    const auto age = age_history_idx_from_string(coordinates->age);
    const auto ring = ring_id_from_token(coordinates->ring);
    REQUIRE(age.has_value());
    REQUIRE(ring.has_value());
    CHECK(histories.history_vector_idx(
              trait, phase, value, Agegrp{*age}, *ring) == index);
  }
  CHECK(!histories.describe_history_vector(160).has_value());
  CHECK(histories.history_column_label(159) ==
        "trait=variant|phase=new_|value=variant_2|ring=ring_2|age=age80_up");
  CHECK(histories.explain_history_vector_idx(
            Trait::variant, Phase::new_, 2, AGE80_UP, 2)
        .contains("column=159"));
  CHECK(histories.explain_history_vector_idx(
            Trait::variant, Phase::now, 0, AGE0_19, 1)
        .starts_with("invalid history coordinates"));

  std::ostringstream dump;
  histories.dump_history_layout(dump);
  CHECK(test_support::split_trimmed_lines(dump.str()).size() == 160);
  CHECK(dump.str().contains("0: trait=status|phase=now|value=unexposed"));
}

void test_layout_all_zero_one_many_cardinalities() {
  RuntimeNamesGuard guard;
  for (size_t variant_count = 1; variant_count <= 3; ++variant_count) {
    for (size_t vax_count = 0; vax_count <= 3; ++vax_count) {
      for (size_t ring_count = 0; ring_count <= 3; ++ring_count) {
        Histories histories = make_histories(
            1, variant_count, vax_count, ring_count);
        const size_t ring_lanes = std::max<size_t>(ring_count, 1);
        const size_t expected = 2 * (4 + vax_count + variant_count)
                              * ring_lanes * HISTORY_AGE_COUNT;
        CHECK(histories.history_vector_count() == expected);
        CHECK(histories.real_variant_count() == variant_count);
        CHECK(histories.real_vax_count() == vax_count);
        CHECK(histories.real_ring_count() == ring_count);
        histories.validate_history_indexing();
      }
    }
  }
}

void test_case_1_atomic_column_count() {
  RuntimeNamesGuard guard;
  // case-1 has six variants, vaccination disabled, and rings disabled.
  Histories histories = make_histories(1, 6, 0, 0);
  CHECK(histories.history_vector_count() == 100);
  CHECK(histories.phase_width(Trait::status) == 20);
  CHECK(histories.phase_width(Trait::vax) == 0);
  CHECK(histories.phase_width(Trait::variant) == 30);
}

void test_day_one_seed_uses_atomic_age_and_ring_cells() {
  RuntimeNamesGuard guard;
  Histories histories = make_histories(2, 1, 0, 2);

  for (size_t person = 1; person <= 5; ++person) {
    const Agegrp age{static_cast<uint8_t>(person)};
    const uint8_t ring = static_cast<uint8_t>((person - 1) % 2 + 1);
    CHECK(histories.at(Trait::status, Phase::now, uint8_t(UNEXPOSED),
                       age, ring)[1] == 1);
  }
  CHECK(histories.aggregate_value(
            Trait::status, Phase::now, uint8_t(UNEXPOSED), 1) == 5);
  CHECK(histories.aggregate_value(
            Trait::status, Phase::new_, uint8_t(UNEXPOSED), 1) == 0);

  Histories no_rings = make_histories(1, 1, 0, 0);
  CHECK(no_rings.ring_lane_count() == 1);
  CHECK(no_rings.at(Trait::status, Phase::now, uint8_t(UNEXPOSED),
                    AGE40_59, RING_ALL)[1] == 1);
}

void test_atomic_update_and_stock_carry_forward() {
  RuntimeNamesGuard guard;
  Histories histories = make_histories(3, 1, 1, 0);

  histories.update(Trait::status, Phase::now, uint8_t(INFECTIOUS),
                   RING_ALL, AGE20_39, 1, 2);
  histories.update(Trait::status, Phase::new_, uint8_t(INFECTIOUS),
                   RING_ALL, AGE20_39, 1, 2);
  histories.update(Trait::vax, Phase::now, 1,
                   RING_ALL, AGE20_39, 1, 3);
  histories.update(Trait::vax, Phase::new_, 1,
                   RING_ALL, AGE20_39, 1, 3);

  CHECK(histories.at(Trait::status, Phase::now, uint8_t(INFECTIOUS),
                     AGE20_39)[1] == 2);
  CHECK(histories.at(Trait::status, Phase::now, uint8_t(INFECTIOUS),
                     AGE0_19)[1] == 0);
  histories.init_history_series(2);
  CHECK(histories.at(Trait::status, Phase::now, uint8_t(INFECTIOUS),
                     AGE20_39)[2] == 2);
  CHECK(histories.at(Trait::vax, Phase::now, 1, AGE20_39)[2] == 3);
  CHECK(histories.at(Trait::status, Phase::new_, uint8_t(INFECTIOUS),
                     AGE20_39)[2] == 0);
  CHECK(histories.at(Trait::vax, Phase::new_, 1, AGE20_39)[2] == 0);
}

void test_resolver_materializes_age_ring_and_vaccine_totals() {
  RuntimeNamesGuard guard;
  Histories histories = make_histories(2, 2, 2, 2);

  histories.at(Trait::status, Phase::now, uint8_t(INFECTIOUS), AGE0_19, 1)[1] = 2;
  histories.at(Trait::status, Phase::now, uint8_t(INFECTIOUS), AGE20_39, 2)[1] = 3;
  histories.at(Trait::vax, Phase::now, 1, AGE0_19, 1)[1] = 4;
  histories.at(Trait::vax, Phase::now, 2, AGE20_39, 2)[1] = 5;
  histories.at(Trait::variant, Phase::new_, 2, AGE60_79, 2)[1] = 7;

  auto total_status = create_history_set(
      HistorySelectorSet{{"now", "infectious", "total"}}, histories);
  CHECK(only_history(total_status).data[1] == 5);
  CHECK(only_history(total_status).source_history_idxs.size() == 10);

  auto age_total_rings = create_history_set(
      HistorySelectorSet{{"now", "infectious", "age0_19"}}, histories);
  CHECK(only_history(age_total_rings).data[1] == 2);
  CHECK(only_history(age_total_rings).source_history_idxs.size() == 2);

  auto ring_total_ages = create_history_set(
      HistorySelectorSet{{"now", "infectious", "total", "", "ring_2"}}, histories);
  CHECK(only_history(ring_total_ages).data[1] == 3);
  CHECK(only_history(ring_total_ages).source_history_idxs.size() == 5);

  auto vaccinated = create_history_set(
      HistorySelectorSet{{"now", "vaccinated", "total"}}, histories);
  CHECK(only_history(vaccinated).data[1] == 9);
  CHECK(only_history(vaccinated).source_history_idxs.size() == 20);

  auto brand = create_history_set(
      HistorySelectorSet{{"now", "vax:vax_1", "total"}}, histories);
  CHECK(only_history(brand).data[1] == 4);
  CHECK(only_history(brand).source_history_idxs.size() == 10);

  auto variant = create_history_set(
      HistorySelectorSet{{"new_", "variant:variant_2", "total"}}, histories);
  CHECK(only_history(variant).data[1] == 7);
  CHECK(only_history(variant).label == "new_variant:variant_2:total");
}

void test_vaccinated_aggregate_zero_one_many_and_invalid_placeholder() {
  RuntimeNamesGuard guard;

  Histories no_vax = make_histories(1, 1, 0, 0);
  auto none = create_history_set(
      HistorySelectorSet{{"now", "vaccinated", "total"}}, no_vax);
  CHECK(only_history(none).data[1] == 0);
  CHECK(only_history(none).source_history_idxs.empty());

  Histories one_vax = make_histories(1, 1, 1, 0);
  one_vax.at(Trait::vax, Phase::now, 1, AGE0_19)[1] = 4;
  auto one = create_history_set(
      HistorySelectorSet{{"now", "vaccinated", "total"}}, one_vax);
  CHECK(only_history(one).data[1] == 4);
  CHECK(only_history(one).source_history_idxs.size() == 5);

  Histories many_vax = make_histories(1, 1, 3, 0);
  many_vax.at(Trait::vax, Phase::now, 1, AGE0_19)[1] = 1;
  many_vax.at(Trait::vax, Phase::now, 2, AGE0_19)[1] = 2;
  many_vax.at(Trait::vax, Phase::now, 3, AGE0_19)[1] = 3;
  auto many = create_history_set(
      HistorySelectorSet{{"now", "vaccinated", "total"}}, many_vax);
  CHECK(only_history(many).data[1] == 6);
  CHECK(only_history(many).source_history_idxs.size() == 15);

  auto mixed = create_history_set(
      HistorySelectorSet{{"new_", "unexposed", "total"},
                           {"now", "unexposed", "total"}},
      many_vax);
  CHECK(mixed.invalid_selections ==
        std::vector<std::string>{"new_|unexposed|total"});
  REQUIRE(mixed.history_vectors.size() == 1);
  CHECK(mixed.history_vectors[0].data[1] == 5);

  HistorySelectorSet all_total("all", "total");
  CHECK(all_total.selections.size() == 17);
  CHECK(std::ranges::find(all_total.selections,
                          HistorySelector{"new_", "unexposed", "total"})
        == all_total.selections.end());
}

void test_print_and_serialization_use_materialized_totals() {
  RuntimeNamesGuard guard;
  Histories histories = make_histories(2, 1, 2, 2);
  histories.at(Trait::status, Phase::now, uint8_t(INFECTIOUS), AGE0_19, 1)[1] = 2;
  histories.at(Trait::status, Phase::now, uint8_t(INFECTIOUS), AGE20_39, 2)[1] = 3;
  histories.at(Trait::vax, Phase::now, 1, AGE0_19, 1)[1] = 4;
  histories.at(Trait::vax, Phase::now, 2, AGE20_39, 2)[1] = 5;

  const HistorySelectorSet selections(std::vector<HistorySelector>{
      {"now", "unknown_history", "total"},
      {"now", "infectious", "total"},
      {"now", "vaccinated", "total"},
  });

  std::ostringstream printed;
  print_selected_histories(selections, histories, 15, printed);
  CHECK(printed.str().contains("Skipping invalid history selections"));
  CHECK(printed.str().contains("now_infectious:total"));
  CHECK(printed.str().contains("       5"));
  CHECK(printed.str().contains("now_vaccinated:total"));
  CHECK(printed.str().contains("       9"));

  std::ostringstream total_status;
  print_total_status_histories(histories, 15, total_status);
  CHECK(total_status.str().contains("infected"));
  CHECK(total_status.str().contains("       5"));

  const auto temp_dir = test_support::fs::temp_directory_path()
                      / test_support::unique_name("epi_sim_series_");
  const auto csv_path = temp_dir / "totals.csv";
  serialize_selected_histories(selections, histories, csv_path);
  CHECK(test_support::read_file_text(csv_path) ==
        "now_infectious:total,now_vaccinated:total\n5,9\n0,0\n");
  test_support::fs::remove_all(temp_dir);
}

void test_variant_invariant_uses_materialized_totals() {
  RuntimeNamesGuard guard;
  Histories histories = make_histories(2, 2, 0, 2);
  histories.at(Trait::status, Phase::now, uint8_t(INFECTIOUS), AGE0_19, 1)[1] = 2;
  histories.at(Trait::variant, Phase::now, 1, AGE0_19, 1)[1] = 1;
  histories.at(Trait::variant, Phase::now, 2, AGE0_19, 1)[1] = 1;
  histories.validate_variant_invariant();

  histories.at(Trait::variant, Phase::now, 2, AGE0_19, 1)[1] = 0;
  bool threw = false;
  try {
    histories.validate_variant_invariant();
  } catch (const std::runtime_error&) {
    threw = true;
  }
  CHECK(threw);
}

void test_ring_selection_parsing() {
  RuntimeNamesGuard guard;
  install_names(1, 0, 2);

  CHECK(ring_id_from_token("") == RING_ALL);
  CHECK(ring_id_from_token("ring_2") == 2);
  CHECK(ring_id_from_token("1") == 1);
  CHECK(!ring_id_from_token("3").has_value());
  CHECK(!ring_id_from_token("999999999999999999999999999999").has_value());

  auto named = parse_ring_suffix("now_infectious@ring:ring_2");
  REQUIRE(named.has_value());
  CHECK(named->base_name == "now_infectious");
  CHECK(named->ring == 2);
  auto bare = parse_ring_suffix("now_infectious");
  REQUIRE(bare.has_value());
  CHECK(bare->ring == RING_ALL);
  CHECK(!parse_ring_suffix("now_infectious@ring:").has_value());
}

void test_fixed_selection_metadata_and_validation() {
  RuntimeNamesGuard guard;
  Histories histories = make_histories(1, 1, 1, 0);
  CHECK((trait_names == std::array<std::string_view, 3>{"status", "vax", "variant"}));
  CHECK((phase_names == std::array<std::string_view, 2>{"now", "new_"}));
  for (const auto trait : all_traits) {
    CHECK(magic_enum::enum_cast<Trait>(magic_enum::enum_name(trait)) == trait);
  }
  for (const auto phase : all_phases) {
    CHECK(magic_enum::enum_cast<Phase>(magic_enum::enum_name(phase)) == phase);
  }
  CHECK(!magic_enum::enum_cast<Trait>("COUNT").has_value());
  CHECK(!magic_enum::enum_cast<Phase>("COUNT").has_value());
  CHECK(!histories.valid_history_coordinates(
      static_cast<Trait>(3), Phase::now, 1, AGE0_19));
  CHECK(!histories.valid_history_coordinates(
      Trait::status, static_cast<Phase>(2), 1, AGE0_19));
  CHECK(!age_history_idx_from_string("unknown").has_value());
  CHECK(!age_history_idx_from_string("AGE0_19").has_value());
  CHECK(!age_history_idx_from_string("age80up").has_value());
  CHECK(age_history_idx_from_string("total") == HISTORY_AGE_TOTAL);

  const HistorySelectorSet invalid{
      {"now", "none", "total"},
      {"now", "INFECTIOUS", "total"},
      {"NOW", "infectious", "total"},
      {"new", "infectious", "total"},
      {"COUNT", "infectious", "total"},
      {"now", "infectious", "unknown"},
      {"new_", "unexposed", "total"},
      {"now", "vax:none", "total"},
      {"now", "variant:none", "total"}};
  const auto rejected = create_history_set(invalid, histories);
  CHECK(rejected.history_vectors.empty());
  CHECK(rejected.invalid_selections.size() == invalid.selections.size());

  const auto valid = create_history_set(
      HistorySelectorSet{{"new_", "dead", "total"}}, histories);
  CHECK(only_history(valid).label == "new_dead:total");
}

void write_series_artifacts(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;
  RuntimeNamesGuard guard;
  Histories histories = make_histories(2, 2, 2, 2);
  histories.at(Trait::status, Phase::now,
               uint8_t(INFECTIOUS), AGE0_19, 1)[1] = 2;
  histories.at(Trait::status, Phase::now,
               uint8_t(INFECTIOUS), AGE20_39, 2)[1] = 3;

  std::ostringstream summary;
  summary << "Atomic histories summary\n"
          << "========================\n\n"
          << "columns: " << histories.history_vector_count() << "\n"
          << "status phase width: " << histories.phase_width(Trait::status) << "\n"
          << "materialized now_infectious total: "
          << histories.aggregate_value(
                 Trait::status, Phase::now, uint8_t(INFECTIOUS), 1)
          << "\n\n";
  histories.dump_history_layout(summary);
  test_support::write_artifact_text(
      options, GROUP, "series_summary.txt", summary.str());
}

}  // namespace

void run_series_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running series tests...");
  test_atomic_layout_formula_and_introspection();
  test_layout_all_zero_one_many_cardinalities();
  test_case_1_atomic_column_count();
  test_day_one_seed_uses_atomic_age_and_ring_cells();
  test_atomic_update_and_stock_carry_forward();
  test_resolver_materializes_age_ring_and_vaccine_totals();
  test_vaccinated_aggregate_zero_one_many_and_invalid_placeholder();
  test_print_and_serialization_use_materialized_totals();
  test_variant_invariant_uses_materialized_totals();
  test_ring_selection_parsing();
  test_fixed_selection_metadata_and_validation();
  write_series_artifacts(options);
  if (options.write_artifacts) {
    fmt::println("series artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("series tests passed.");
}
