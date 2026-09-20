#pragma once
#include "parameters.h"
#include "population.h"
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <ostream>
#include <utility>


inline constexpr uint8_t HISTORY_AGE_TOTAL = 0;
inline constexpr uint8_t HISTORY_AGE_TOTAL = 0;
// Histories store only the five concrete age groups. "total" is a query
// selector and is materialized by summing those atomic vectors.
inline constexpr size_t HISTORY_AGE_COUNT = Agegrp::names.size() - 1;

constexpr std::string_view age_vec_label(uint8_t age) {
constexpr std::string_view age_vec_label(uint8_t age) {
    if (age == HISTORY_AGE_TOTAL) return "total";
    return std::string_view{Agegrp::names[age]};
}

inline std::optional<uint8_t> age_history_idx_from_string(std::string_view text) {
    if (text == "total") return HISTORY_AGE_TOTAL;
    const auto age = magic_enum::enum_cast<Agegrp::Enum>(text);
    if (age && *age != Agegrp::Enum::unknown) return std::to_underlying(*age);
    const auto age = magic_enum::enum_cast<Agegrp::Enum>(text);
    if (age && *age != Agegrp::Enum::unknown) return std::to_underlying(*age);
    return std::nullopt;
}

// Query selector for all rings. When rings are disabled, raw ring 0 also names
// the single implicit whole-population storage lane.
inline constexpr uint8_t RING_ALL = 0;

/*
HistorySelector: semantic coordinates used to select a specific history (vector or column),
by phase (now or new_), trait_value (such as "Dead" or "Recovered" for trait Status),
age, trait, and ring.
*/
struct HistorySelector {
    std::string phase;
    std::string trait_value;
    std::string age;         // "total" or one of the concrete Agegrp names
    std::string trait = "";  // needed for some uses...
    std::string ring = "";   // "" -> RING_ALL (all-rings aggregate)

    bool operator==(const HistorySelector&) const = default;
    const void print() {
      auto print_ring = ring == "" ? "\"population\"" : ring;
      fmt::println("phase: {:<6} trait: {:<9} trait value: {:18} age: {:<9} ring: {:<10}", phase, trait, trait_value, age, print_ring);
    }
};

enum class Trait : uint8_t {
    status = 0,
    vax = 1,
    variant = 2
};

enum class Phase : uint8_t {
    now = 0,
    new_ = 1
};

inline constexpr auto trait_names = magic_enum::enum_names<Trait>();
inline constexpr auto phase_names = magic_enum::enum_names<Phase>();

inline constexpr auto all_traits = magic_enum::enum_values<Trait>();
inline constexpr auto all_phases = magic_enum::enum_values<Phase>();

static_assert(magic_enum::enum_count<Trait>() == 3);
static_assert(magic_enum::enum_count<Phase>() == 2);
static_assert(std::to_underlying(Trait::status) == 0
              && std::to_underlying(Trait::vax) == 1
              && std::to_underlying(Trait::variant) == 2);
static_assert(std::to_underlying(Phase::now) == 0
              && std::to_underlying(Phase::new_) == 1);

using HistoryValue = std::int32_t;

/*
Histories: top-level container for all collected history in the simulation.

Nesting:
  Histories (one instance, passed by reference everywhere)
    vector of HistoryVectors (indexed directly by Histories)
      vector<int32_t> (indexed by simulation day, 1-based; slot 0 unused)
*/
struct Histories {
    size_t day_cnt;

    // constructor declaration, defined in series.cpp
    // constructor declaration, defined in series.cpp
    Histories(size_t n_days, const PopData& pop,
              size_t real_variant_count, size_t real_vax_count,
              size_t real_ring_count);

    // returns index to Histories.history_vectors_ method uses enum value inputs
    [[clang::always_inline]] size_t history_vector_idx(
        Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const {
        // trait values are all 1-indexed; we need 0 indexed for this
        // trait values are all 1-indexed; we need 0 indexed for this
        const size_t value_ordinal = size_t(trait_value - 1);
        const size_t ring_ordinal = real_ring_count_ == 0
                                  ? 0
                                  : size_t(ring - 1);
        const size_t age_ordinal = size_t(age.v - 1);
        return (trait_phase_base(trait, phase)     // this is a great way to do this
             + value_ordinal * ring_lane_count_ * HISTORY_AGE_COUNT
             + ring_ordinal * HISTORY_AGE_COUNT
             + age_ordinal);
    }

    // return a mutable vector reference using index input
    [[clang::always_inline]] std::vector<HistoryValue>& history_vector(
        size_t index) {
        return history_vectors_[index];
    }
    // return a const vector reference using index input
    [[clang::always_inline]] const std::vector<HistoryValue>& history_vector(
        size_t index) const {
        return history_vectors_[index];
    }
    // return a mutable vector 
    // return a mutable vector 
    [[clang::always_inline]] std::vector<HistoryValue>& at(
        Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) {
        return history_vectors_[history_vector_idx(
            trait, phase, trait_value, age, ring)];
    }
    // return a const vector 
    // return a const vector 
    [[clang::always_inline]] const std::vector<HistoryValue>& at(
        Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const {
        return history_vectors_[history_vector_idx(
            trait, phase, trait_value, age, ring)];
    }

    [[clang::always_inline]] void update(
        Trait trait, Phase phase, uint8_t trait_value, uint8_t ring,
        Agegrp agegrp, size_t day, HistoryValue change) {
        history_vectors_[history_vector_idx(
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
    size_t sim_history_count() const { return history_vectors_.size(); }
    bool valid_history_coordinates(
        Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const;

    // externally defined methods (in series.cpp)
    std::optional<HistorySelector> describe_history_vector(size_t index) const;
    std::string history_column_label(size_t index) const;
    std::string explain_history_vector_idx(
        Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const;
    void dump_history_layout(std::ostream& out = std::cout) const;
    void validate_history_indexing() const;
    void insert_total_vecs();
    void calc_total_histories();

    HistoryValue aggregate_value(Trait trait, Phase phase,
                                 uint8_t trait_value, size_t day) const;

    void init_history_series(size_t day);

    // Checks that sum(now variant values, total age) == now infectious, total age
    // for every simulated day. Throws on the first mismatch; prints OK if all pass.
    void validate_variant_invariant() const;

private:
      // returns index of the column group that will contain the desired column,
      // starting at the trait and the phase within that trait
      // returns index of the column group that will contain the desired column,
      // starting at the trait and the phase within that trait
    [[clang::always_inline]] size_t trait_phase_base(
        Trait trait, Phase phase) const {
        const size_t status_width = phase_widths_[size_t(Trait::status)];
        const size_t vax_width = phase_widths_[size_t(Trait::vax)];
        switch (trait) {   // this is a great way to do this
            case Trait::status:  // first trait within indices
        switch (trait) {   // this is a great way to do this
            case Trait::status:  // first trait within indices
                return size_t(phase) * status_width;
            case Trait::vax:  // both phases of status + 0 or 1 phase of vax
            case Trait::vax:  // both phases of status + 0 or 1 phase of vax
                return 2 * status_width + size_t(phase) * vax_width;
            case Trait::variant:  // didn't cache a variant_width variable because there is no re-use
            case Trait::variant:  // didn't cache a variant_width variable because there is no re-use
                return 2 * (status_width + vax_width)
                     + size_t(phase) * phase_widths_[size_t(Trait::variant)];
        }
        std::unreachable();
    }

    std::array<size_t, magic_enum::enum_count<Trait>()> phase_widths_{};
    std::array<size_t, magic_enum::enum_count<Trait>()> phase_widths_{};
    size_t real_variant_count_{};
    size_t real_vax_count_{};
    size_t real_ring_count_{};
    size_t ring_lane_count_{};
    std::vector<std::vector<HistoryValue>> history_vectors_;
};

// Maps a ring token (as it appears in HistorySelector::ring) to a ring id.
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
HistorySelectorSet: a vector (to hold the set) of HistorySelectors.

Usage:
  HistorySelectorSet("all")   // constructor with single sentinel value
  HistorySelectorSet("all", "total")     // constructor with sentinel value and agegrp value
  HistorySelectorSet("all", {"total", "age20_39"})
  HistorySelectorSet({{"now_infectious","total"}, {"now_recovered","total"}})
  HistorySelectorSet{{"now_infectious","total"}, {"now_recovered","total"}}
  HistorySelectorSet{{"now_infectious","total","Jail"}, {"now_dead","total"}}  // initializer list
*/
struct HistorySelectorSet {
    std::vector<HistorySelector> selections;

    // constructors
    HistorySelectorSet(std::vector<HistorySelector> v)
        : selections(std::move(v)) {}

    HistorySelectorSet(std::initializer_list<HistorySelector> v)
        : selections(v) {}

    // "all" sentinel -> all trait values x all selectable ages
    HistorySelectorSet(const char* sentinel);

    // "all" sentinel x single age
    HistorySelectorSet(const char* sentinel, const char* age);

    // "all" sentinel x multiple ages
    HistorySelectorSet(const char* sentinel, std::vector<std::string> ages);

private:
    static void validate_sentinel(const char* s);
    static std::vector<HistorySelector> build_for_ages(
        const std::vector<std::string>& ages);
};

struct TotalHistoryVector {
    std::string label;
    std::vector<HistoryValue> data;
    std::vector<size_t> source_history_idxs;
};

struct TotalHistorySet {
    std::vector<TotalHistoryVector> history_vectors;
    std::vector<std::string> invalid_selections;
};

TotalHistorySet create_history_set(
    const HistorySelectorSet& spec, const Histories& histories);

void print_total_status_histories(const Histories& histories,
                                  size_t days_per_group = 15,
                                  std::ostream& out = std::cout);
void print_selected_histories(HistorySelectorSet spec,
                              const Histories& histories,
                              size_t days_per_group = 15,
                              std::ostream& out = std::cout);
void serialize_selected_histories(HistorySelectorSet spec,
                                  const Histories& histories,
                                  std::filesystem::path output_path);
void serialize_selected_histories(HistorySelectorSet spec,
                                  const Histories& histories,
                                  string base_fname,
                                  vector<string> path_steps = {});
const std::vector<HistorySelector>
   enumerate_history_selections(const Histories& histories);
