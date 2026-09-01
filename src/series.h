#pragma once
#include "parameters.h"
#include "population.h"
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <numeric>
#include <ostream>
#include <utility>


using AgeVecIndex = uint8_t;

inline constexpr AgeVecIndex HISTORY_AGE_TOTAL = 0;
// Histories store only the five concrete age groups. "total" is a query
// selector and is materialized by summing those atomic vectors.
inline constexpr size_t HISTORY_AGE_COUNT = Agegrp::names.size() - 1;

constexpr std::string_view age_vec_label(AgeVecIndex age) {
    if (age == HISTORY_AGE_TOTAL) return "total";
    return std::string_view{Agegrp::names[age]};
}

inline std::optional<AgeVecIndex> age_vec_index_from_string(std::string_view text) {
    if (text == "total") return HISTORY_AGE_TOTAL;
    for (size_t i = 1; i < Agegrp::names.size(); ++i) {
        if (Agegrp::names[i] == text) return static_cast<AgeVecIndex>(i);
    }
    return std::nullopt;
}

// Query selector for all rings. When rings are disabled, raw ring 0 also names
// the single implicit whole-population storage lane.
inline constexpr uint8_t RING_ALL = 0;

struct HistorySelection {
    std::string name;
    std::string age;         // "total" or one of the concrete Agegrp names
    std::string ring = "";   // "" -> RING_ALL (all-rings aggregate)

    bool operator==(const HistorySelection&) const = default;
};

enum class Trait : uint8_t {
    status,
    vax,
    variant,
    COUNT
};

enum class Phase : uint8_t {
    now,
    new_,
    COUNT
};

inline constexpr auto all_traits = std::array{
    Trait::status, Trait::vax, Trait::variant};

inline constexpr auto all_phases = std::array{
    Phase::now, Phase::new_};

static_assert(all_traits.size() == size_t(Trait::COUNT));
static_assert(all_phases.size() == size_t(Phase::COUNT));

using HistoryValue = std::int32_t;

struct HistoryColumnCoordinates {
    Trait trait;
    Phase phase;
    uint8_t trait_value; // raw enum value; concrete values begin at 1
    Agegrp age;
    uint8_t ring;        // 0 only for the implicit lane when rings are disabled

    bool operator==(const HistoryColumnCoordinates&) const = default;
};

/*
Histories: top-level container for all collected history in the simulation.

Nesting:
  Histories (one instance, passed by reference everywhere)
    vector of HistoryVectors (indexed directly by Histories)
      vector<int32_t> (indexed by simulation day, 1-based; slot 0 unused)
*/
struct Histories {
    size_t day_cnt;

    Histories(size_t n_days, const PopData& pop,
              size_t real_variant_count, size_t real_vax_count,
              size_t real_ring_count);

    [[clang::always_inline]] size_t history_vector_index(
        Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const {
        const size_t value_ordinal = size_t(trait_value - 1);
        const size_t ring_ordinal = real_ring_count_ == 0
                                  ? 0
                                  : size_t(ring - 1);
        const size_t age_ordinal = size_t(age.v - 1);
        return trait_phase_base(trait, phase)
             + value_ordinal * ring_lane_count_ * HISTORY_AGE_COUNT
             + ring_ordinal * HISTORY_AGE_COUNT
             + age_ordinal;
    }

    [[clang::always_inline]] std::vector<HistoryValue>& history_vector(
        size_t index) {
        return history_vectors_[index];
    }
    [[clang::always_inline]] const std::vector<HistoryValue>& history_vector(
        size_t index) const {
        return history_vectors_[index];
    }

    [[clang::always_inline]] std::vector<HistoryValue>& at(
        Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) {
        return history_vectors_[history_vector_index(
            trait, phase, trait_value, age, ring)];
    }
    [[clang::always_inline]] const std::vector<HistoryValue>& at(
        Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const {
        return history_vectors_[history_vector_index(
            trait, phase, trait_value, age, ring)];
    }

    [[clang::always_inline]] void update(
        Trait trait, Phase phase, uint8_t trait_value, uint8_t ring,
        Agegrp agegrp, size_t day, HistoryValue change) {
        history_vectors_[history_vector_index(
            trait, phase, trait_value, agegrp, ring)][day] += change;
    }

    size_t trait_value_count(Trait trait) const;
    size_t phase_width(Trait trait) const {
        return phase_widths_[size_t(trait)];
    }
    size_t real_variant_count() const { return real_variant_count_; }
    size_t real_vax_count() const { return real_vax_count_; }
    size_t real_ring_count() const { return real_ring_count_; }
    size_t ring_lane_count() const { return ring_lane_count_; }
    size_t history_vector_count() const { return history_vectors_.size(); }
    bool valid_history_coordinates(
        Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const;

    std::optional<HistoryColumnCoordinates> describe_history_vector(
        size_t index) const;
    std::string history_column_label(size_t index) const;
    std::string explain_history_vector_index(
        Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const;
    void dump_history_layout(std::ostream& out = std::cout) const;
    void validate_history_layout() const;

    HistoryValue aggregate_value(Trait trait, Phase phase,
                                 uint8_t trait_value, size_t day) const;

    void init_history_series(size_t day);

    // Checks that sum(now variant values, total age) == now infectious, total age
    // for every simulated day. Throws on the first mismatch; prints OK if all pass.
    void validate_variant_invariant() const;

private:
    [[clang::always_inline]] size_t trait_phase_base(
        Trait trait, Phase phase) const {
        const size_t status_width = phase_widths_[size_t(Trait::status)];
        const size_t vax_width = phase_widths_[size_t(Trait::vax)];
        switch (trait) {
            case Trait::status:
                return size_t(phase) * status_width;
            case Trait::vax:
                return 2 * status_width + size_t(phase) * vax_width;
            case Trait::variant:
                return 2 * (status_width + vax_width)
                     + size_t(phase) * phase_widths_[size_t(Trait::variant)];
            case Trait::COUNT:
                break;
        }
        std::unreachable();
    }

    std::array<size_t, size_t(Trait::COUNT)> phase_widths_{};
    size_t real_variant_count_{};
    size_t real_vax_count_{};
    size_t real_ring_count_{};
    size_t ring_lane_count_{};
    std::vector<std::vector<HistoryValue>> history_vectors_;
};

// Maps a ring token (as it appears in HistorySelection::ring) to a ring id.
// "" -> RING_ALL (all-rings aggregate). A decimal token is taken as a literal
// ring id; otherwise the token is looked up by name in Ring::names. Returns
// nullopt for an unknown name or out-of-range index.
std::optional<uint8_t> ring_id_from_token(const std::string& tok);

// Parses an optional "@ring:<name|idx>" suffix on a selection name.
// - "now_infectious"            -> {"now_infectious", RING_ALL}
// - "now_infectious@ring:Jail"  -> {"now_infectious", <idx of "Jail">}
// - "now_infectious@ring:3"     -> {"now_infectious", 3}
// Returns nullopt only when the suffix is present but does not resolve
// (unknown ring name or out-of-range index).
struct RingNameParse {
    std::string base_name;
    uint8_t ring;
};
std::optional<RingNameParse> parse_ring_suffix(std::string_view name);

/*
Input argument type for histories to be printed, serialized, or plotted.

Usage:
  HistorySelectionSpec("all")
  HistorySelectionSpec("all", "total")
  HistorySelectionSpec("all", {"total", "age20_39"})
  HistorySelectionSpec({{"now_infectious","total"}, {"now_recovered","total"}})
  HistorySelectionSpec{{"now_infectious","total"}, {"now_recovered","total"}}
  HistorySelectionSpec{{"now_infectious","total","Jail"}, {"now_dead","total"}}
*/
struct HistorySelectionSpec {
    std::vector<HistorySelection> selections;

    HistorySelectionSpec(std::vector<HistorySelection> v)
        : selections(std::move(v)) {}

    HistorySelectionSpec(std::initializer_list<HistorySelection> v)
        : selections(v) {}

    // "all" sentinel -> all trait values x all selectable ages
    HistorySelectionSpec(const char* sentinel);

    // "all" sentinel x single age
    HistorySelectionSpec(const char* sentinel, const char* age);

    // "all" sentinel x multiple ages
    HistorySelectionSpec(const char* sentinel, std::vector<std::string> ages);

private:
    static void validate_sentinel(const char* s);
    static std::vector<HistorySelection> build_for_ages(
        const std::vector<std::string>& ages);
};

struct ResolvedHistoryVector {
    std::string label;
    std::vector<HistoryValue> data;
    std::vector<size_t> source_history_vectors;
};

struct ResolvedHistorySelection {
    std::vector<ResolvedHistoryVector> history_vectors;
    std::vector<std::string> invalid_selections;
};

ResolvedHistorySelection resolve_history_selection(
    const HistorySelectionSpec& spec, const Histories& histories);

void print_total_status_histories(const Histories& histories,
                                  size_t days_per_group = 15,
                                  std::ostream& out = std::cout);
void print_selected_histories(HistorySelectionSpec spec,
                              const Histories& histories,
                              size_t days_per_group = 15,
                              std::ostream& out = std::cout);
void serialize_selected_histories(HistorySelectionSpec spec,
                                  const Histories& histories,
                                  std::filesystem::path output_path);
void serialize_selected_histories(HistorySelectionSpec spec,
                                  const Histories& histories,
                                  string base_fname,
                                  vector<string> path_steps = {});
