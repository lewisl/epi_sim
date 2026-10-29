#pragma once
#include "parameters.h"
#include "traits.h"
#include "population.h"
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <ostream>
// #include <stdexcept>
#include <utility>
#include "absl/container/flat_hash_map.h"
#include <fmt/format.h>
// #include "absl/hash/.h"

using HistoryValue = std::int32_t;

/*
Tracked_Traits are recorded during the simulation for each agent who experiences any value of these traits.
*/
inline constexpr array<TraitID, 3> Tracked_Traits{TraitID::status, TraitID::vax, TraitID::variant};

inline constexpr uint8_t HISTORY_AGE_TOTAL = 0;
inline constexpr uint8_t RING_ALL = 0;
// Histories store only the five concrete age groups. "total" is a query
// selector and is materialized by summing those atomic vectors.

constexpr std::string_view age_vec_label(uint8_t age) {
    if (age == HISTORY_AGE_TOTAL) return "total";
    else return std::string_view{Agegrp::names[age]};
}

inline std::optional<uint8_t> age_history_idx_from_string(std::string_view text) {
    if (text == "total") return HISTORY_AGE_TOTAL;
    const auto age = magic_enum::enum_cast<Agegrp::Enum>(text);
    if (age && *age != Agegrp::Enum::unknown) return std::to_underlying(*age);
    return std::nullopt;
}

enum class Phase : uint8_t {
    now = 0,
    new_ = 1
};

/*
HistorySelector: semantic coordinates used to select a specific history (vector or column),
by phase (now or new_), trait_value (such as "Dead" or "Recovered" for trait Status),
age, trait, and ring.

Optionally, one can use the enum class values as selectors if
the target function can, too.
*/
struct HistorySelector {
    std::string phase;
    std::string trait_value;
    std::string age = "";         // "total" or one of the concrete Agegrp names
    std::string trait;  // needed for some uses...
    std::string ring = "";   // "" -> RING_ALL (all-rings aggregate)

    bool operator==(const HistorySelector&) const = default;
    const void print() {
      auto print_ring = ring == "" ? "\"population\"" : ring;
      fmt::println("phase: {:<6} trait: {:<9} trait value: {:18} age: {:<9} ring: {:<10}", phase, trait, trait_value, age, print_ring);
    }
};


// preferred key for histories simulation map
struct Selsim {
  std::uint8_t trait;
  std::uint8_t phase;
  std::uint8_t trval;
  std::uint8_t ring;
  std::uint8_t age;

  Selsim() = default;

  // use uint8_t inputs for all
  Selsim(uint8_t trait, uint8_t phase, uint8_t trval, uint8_t ring=RING_ALL, uint8_t age=HISTORY_AGE_TOTAL) :
      trait(trait), phase(phase), trval(trval), ring(ring), age(age) {}

  // use enum classes for TraitID and Phase
  Selsim(TraitID trait, Phase phase, std::uint8_t trval, std::uint8_t ring=RING_ALL, std::uint8_t age=HISTORY_AGE_TOTAL) :
    trait(std::to_underlying(trait)), phase(std::to_underlying(phase)), trval(trval), ring(ring), age(age) {}

  friend bool operator==(const Selsim& lhs, const Selsim& rhs) = default;

  template <typename H>
  friend H AbslHashValue(H h, const Selsim& sel);

  friend std::string format_as(const Selsim& s) {
    return fmt::format("Selsim{{trait={}, phase={}, trval={}, ring={}, age={}}}",
                       s.trait, s.phase, s.trval, s.ring, s.age);
  };
};

// use entire struct as hash key: 20% or so slower
// template <typename H>
// H AbslHashValue(H h, const Selsim& sel) {
//   return H::combine(std::move(h), sel.trait, sel.phase, sel.trval, sel.ring, sel.age);
// }

// use single uint64_t as hash key
template <typename H>
H AbslHashValue(H h, const Selsim& sel) {
    const std::uint64_t packed =
          std::uint64_t{sel.trait}
        | (std::uint64_t{sel.phase} << 8)
        | (std::uint64_t{sel.trval} << 16)
        | (std::uint64_t{sel.ring}  << 24)
        | (std::uint64_t{sel.age}   << 32);

    return H::combine(std::move(h), packed);
}

// prefererred key for histories total map
struct Seltot {
  std::uint8_t trait;
  std::uint8_t phase;
  std::uint8_t trval;
  std::uint8_t ring=RING_ALL; // not sure we need the default values here
  std::uint8_t age=HISTORY_AGE_TOTAL;

  Seltot() = default;

  // use uint8_t inputs for all
  Seltot(uint8_t trait, uint8_t phase, uint8_t trval, uint8_t ring=RING_ALL, uint8_t age=HISTORY_AGE_TOTAL) :
      trait(trait), phase(phase), trval(trval), ring(ring), age(age) {}

  // use enum classes for TraitID and Phase
  Seltot(TraitID trait, Phase phase, std::uint8_t trval, std::uint8_t ring=RING_ALL, std::uint8_t age=HISTORY_AGE_TOTAL) :
    trait(std::to_underlying(trait)), phase(std::to_underlying(phase)), trval(trval), ring(ring), age(age) {}

  friend bool operator==(const Seltot& lhs, const Seltot& rhs) = default;

  template <typename H>
  friend H AbslHashValue(H h, const Seltot& sel);

  friend std::string format_as(const Seltot& s) {
    return fmt::format("Seltot{{trait={}, phase={}, trval={}, ring={}, age={}}}",
                       s.trait, s.phase, s.trval, s.ring, s.age);
  };


};


// use struct as hash key: 20%+ slower
// template <typename H>
// H AbslHashValue(H h, const Seltot& sel) {
//   return H::combine(std::move(h), sel.trait, sel.phase, sel.trval);
// }


// use single uint64_t as hash key
template <typename H>
H AbslHashValue(H h, const Seltot& sel) {
    const std::uint64_t packed =
          std::uint64_t{sel.trait}
        | (std::uint64_t{sel.phase} << 8)
        | (std::uint64_t{sel.trval} << 16);

    return H::combine(std::move(h), packed);
}

/*
Each history_vector contains the data as a vector and the metadata that describes that data using struct HistoryMeta
*/
struct HistoryMeta {
  std::string phase;
  Phase phasenum;
  //
  std::string trait; 
  TraitID traitnum;
  //
  std::string traitval;  
  uint8_t traitval_num;  
  //
  std::string age;  
  Agegrp agenum; // ditto for Agegrp

  std::string ring;
  Ring ringnum;

  std::string output_label;

};

struct HistoryVector {
  vector<HistoryValue> data;
  HistoryMeta meta;
};


inline constexpr auto trait_names = magic_enum::enum_names<TraitID>();
inline constexpr auto phase_names = magic_enum::enum_names<Phase>();

inline constexpr auto all_traits = magic_enum::enum_values<TraitID>();
inline constexpr auto all_phases = magic_enum::enum_values<Phase>();

static_assert(magic_enum::enum_count<TraitID>() == 22);
static_assert(magic_enum::enum_count<Phase>() == 2);
static_assert(std::to_underlying(TraitID::status) == 1
              && std::to_underlying(TraitID::vax) == 2
              && std::to_underlying(TraitID::variant) == 3);
static_assert(std::to_underlying(Phase::now) == 0
              && std::to_underlying(Phase::new_) == 1);

// fwd declaration
struct HistorySelectorSet;  // it's after the Histories struct

/*
Histories: top-level container for all collected history in the simulation.

Nesting:
  Histories (one instance, passed by reference everywhere)
    vector of HistoryVectors (indexed directly by Histories)
      vector<int32_t> (indexed by simulation day, 1-based; slot 0 unused)
*/
struct Histories {
    size_t day_cnt;
    size_t sim_history_count; // number of history vectors updated during simulation
    size_t total_history_count;
    size_t all_history_count;

    std::vector<HistoryVector> history_vectors_;

    absl::flat_hash_map<Selsim, size_t> sim_history_map;
    absl::flat_hash_map<Seltot, size_t> sim_totals_map;
    std::vector<size_t> cumulative_columns{};
    std::vector<size_t> new_columns{};

    // constructor declaration, defined in series.cpp
    Histories(size_t n_days, const PopData& pop);
  
    void build_history_vectors(bool for_totals);

    std::vector<size_t> select_indices(HistorySelectorSet sel, std::string grab_bag="");


    // returns index to Histories.history_vectors_ method uses enum value inputs
    [[clang::always_inline]] size_t history_vector_idx(
            TraitID trait, Phase phase, uint8_t trait_value, Agegrp age,
            uint8_t ring = RING_ALL, bool total=false) const {
        if (total) {
          return sim_totals_map.at(Seltot{trait, phase, trait_value, age, ring});
        }
        else {
          // trait values are all 1-indexed; we need 0 indexed for this
          const size_t value_ordinal = size_t(trait_value - 1);
          const size_t ring_ordinal = size_t(ring - 1);
          const size_t age_ordinal = size_t(age.v - 1);
          return sim_history_map.at(Selsim{static_cast<uint8_t>(trait), static_cast<uint8_t>(phase), trait_value, ring, static_cast<uint8_t>(age)});
        }
    }


    // return a mutable vector reference using index input
    [[clang::always_inline]] std::vector<HistoryValue> & history_vector(size_t index) {
        return history_vectors_[index].data;
    }
    // return a const vector reference using index input
    [[clang::always_inline]] const std::vector<HistoryValue>& history_vector(
        size_t index) const {
        return history_vectors_[index].data;
    }
    
    // return a mutable vector 
    [[clang::always_inline]] std::vector<HistoryValue>& at(
        TraitID trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) {
        return history_vectors_[history_vector_idx(
            trait, phase, trait_value, age, ring)].data;
    }
    // return a mutable total vector
    [[clang::always_inline]] std::vector<HistoryValue>& at(
        TraitID trait, Phase phase, uint8_t trait_value, Agegrp age) {
        return history_vectors_[history_vector_idx(
            trait, phase, trait_value, age)].data;
    }    

    // return a const vector 
    [[clang::always_inline]] const std::vector<HistoryValue>& at(
        TraitID trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const {
        return history_vectors_[history_vector_idx(
            trait, phase, trait_value, age, ring)].data;
    }

    [[clang::always_inline]] void update(
        TraitID trait, Phase phase, uint8_t trait_value, uint8_t ring,
        Agegrp agegrp, size_t day, HistoryValue change) {
        history_vectors_[history_vector_idx(
            trait, phase, trait_value, agegrp, ring)].data[day] += change;
    }

    size_t trait_value_count(TraitID trait) const;
    bool valid_history_coordinates(
        TraitID trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const;

    // externally defined methods (in series.cpp)
    std::optional<HistorySelector> describe_history_vector(size_t index) const;
    std::string history_column_label(size_t index) const;
    std::string explain_history_vector_idx(
        TraitID trait, Phase phase, uint8_t trait_value, Agegrp age,
        uint8_t ring = RING_ALL) const;
    void dump_history_layout(std::ostream& out = std::cout) const;
    void validate_history_indexing() const;
    void insert_total_vecs();
    void calc_total_histories();
    void sum_total_histories();

    HistoryValue aggregate_value(TraitID trait, Phase phase,
                                 uint8_t trait_value, size_t day) const;

    void init_history_newday(size_t day);

    // Checks that sum(now variant values, total age) == now infectious, total age
    // for every simulated day. Throws on the first mismatch; prints OK if all pass.
    void validate_variant_invariant() const;

};

// Maps a ring token (as it appears in HistorySelector::ring) to a ring id.
// "" -> RING_ALL (all-rings aggregate). A decimal token is taken as a literal
// ring id; otherwise the token is looked up by name in Ring::names. Returns
// nullopt for an unknown name or out-of-range index.
std::optional<uint8_t> ring_id_from_token(const std::string& tok);


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
    std::vector<HistorySelector> selections;  // result to be consumed

    // constructors
    HistorySelectorSet(std::vector<HistorySelector> v)
        : selections(std::move(v)) {}

    HistorySelectorSet(std::initializer_list<HistorySelector> v)
        : selections(v) {}

    // "all" sentinel -> all trait values x all selectable ages
    // HistorySelectorSet(const char* sentinel);

    // "all" sentinel x single age -- we don't have the vectors
    // HistorySelectorSet(const char* sentinel, const char* age);

    // "all" sentinel x multiple ages -- we don't have the vectors
    // HistorySelectorSet(const char* sentinel, std::vector<std::string> ages);

private:
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
void serialize_histories_new_way(const Histories& histories, 
              std::vector<size_t> idxs,
              std::filesystem::path output_path);
void serialize_selected_histories(HistorySelectorSet spec,
                                  const Histories& histories,
                                  string base_fname,
                                  vector<string> path_steps = {});
const std::vector<HistorySelector>
   enumerate_history_selections(const Histories& histories);
