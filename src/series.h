#pragma once
#include "fmt/format.h"
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"
#include <initializer_list>


enum class AgeBucket : uint8_t { total, age0_19, age20_39, age40_59, age60_79, age80_up, COUNT };

// Reserved ring slot holding the all-rings aggregate. Reuses the unused
// sentinel index so no real person (rings are 1-based) ever collides with it.
inline constexpr uint8_t RING_ALL = 0;

struct SeriesSelection {
    std::string name;
    std::string bucket;
    std::string ring = "";   // "" → RING_ALL (all-rings aggregate)

    bool operator==(const SeriesSelection&) const = default;
};

inline constexpr auto all_age_buckets = std::array{
    AgeBucket::total, AgeBucket::age0_19, AgeBucket::age20_39,
    AgeBucket::age40_59, AgeBucket::age60_79, AgeBucket::age80_up};

inline constexpr auto age_bucket_labels = std::array{
    "total", "age0_19", "age20_39", "age40_59", "age60_79", "age80_up"};

static_assert(all_age_buckets.size() == size_t(AgeBucket::COUNT));
static_assert(age_bucket_labels.size() == size_t(AgeBucket::COUNT));

constexpr std::string_view to_string(AgeBucket bucket) {
    return age_bucket_labels[size_t(bucket)];
}

inline std::optional<AgeBucket> age_bucket_from_string(std::string_view text) {
    for (size_t i = 0; i < age_bucket_labels.size(); ++i) {
        if (age_bucket_labels[i] == text) return static_cast<AgeBucket>(i);
    }
    return std::nullopt;
}


/*
SeriesGroup: one group of series data, indexed first by subject (trait uint8_t value),
then by ring id (0 = RING_ALL aggregate), then by age bucket, then by simulation day
(1-based; slot 0 unused).

Usage:
  SeriesGroup sg(n_subjects, n_rings, day_cnt);
  sg.update(INFECTIOUS, ring, agegrp, today, 1);  // writes per-ring + RING_ALL aggregate
  int v = sg.at(INFECTIOUS, AgeBucket::total)[day];           // defaults to RING_ALL
  int v = sg.at(INFECTIOUS, AgeBucket::total, ring_id)[day];  // specific ring
*/
struct SeriesGroup {
    using BucketArray = std::array<std::vector<int>, size_t(AgeBucket::COUNT)>;  // one ring's age buckets
    using RingArray   = std::vector<BucketArray>;  // indexed by ring id (0 = RING_ALL)

    std::vector<RingArray> subjects;  // indexed by trait's uint8_t value
    size_t day_cnt{};
    size_t n_rings{};

    SeriesGroup() = default;
    SeriesGroup(size_t n_subjects, size_t n_rings, size_t day_cnt);

    // Writes four cells per call: (ring, bucket), (ring, total),
    // (RING_ALL, bucket), (RING_ALL, total). The RING_ALL mirror-writes
    // accumulate the all-rings aggregate inline; the inner guard avoids
    // double-counting when ring == RING_ALL (no-rings case).
    void update(uint8_t subject_idx, uint8_t ring, Agegrp agegrp, size_t day, int change);

    auto& at(uint8_t subject_idx, AgeBucket bucket, uint8_t ring = RING_ALL) {
        return subjects[subject_idx][ring][size_t(bucket)];
    }
    auto const& at(uint8_t subject_idx, AgeBucket bucket, uint8_t ring = RING_ALL) const {
        return subjects[subject_idx][ring][size_t(bucket)];
    }
};

/*
AllSeries: top-level container for all time-series data in the simulation.

Nesting:
  AllSeries (one instance, passed by reference everywhere)
    6 SeriesGroup members (compile-time named fields)
      vector of RingArray (indexed by trait uint8_t: Status, Vax, or Variant)
        vector of BucketArray (indexed by ring id; 0 = RING_ALL aggregate)
          array of 6 vector<int> (indexed by AgeBucket enum)
            vector<int> (indexed by simulation day, 1-based; slot 0 unused)
*/
struct AllSeries {
    SeriesGroup now_status, new_status;
    SeriesGroup now_vax,    new_vax;
    SeriesGroup now_variant, new_variant;
    size_t day_cnt;

    AllSeries(size_t day_cnt, const PopData& pop,
              size_t n_variants, size_t n_vax, size_t n_rings);

    void init_history_series(size_t day);
    void finalize_series();

    // Checks that sum(now_variant[v][total][day]) == now_status[INFECTIOUS][total][day]
    // for every simulated day. Throws on the first mismatch; prints OK if all pass.
    void validate_variant_invariant() const;
};


AgeBucket bucket_from_age(Agegrp agegrp);

// Resolves a (name, bucket, ring) triple to a day-indexed vector<int>.
// For vax series, sums across all non-none brands (index > 0).
// Ring defaults to RING_ALL (the all-rings aggregate). The name should
// be the base name only (no "@ring:..." suffix); see parse_ring_suffix.
// Returns nullopt if name is not recognized.
std::optional<vector<int>> resolve_series(const AllSeries& series,
                                          std::string_view name, AgeBucket bucket,
                                          uint8_t ring = RING_ALL);

// Maps a ring token (as it appears in SeriesSelection::ring) to a ring id.
// "" → RING_ALL (all-rings aggregate). A decimal token is taken as a literal
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
Input argument type for series columns to be printed/serialized/plotted.

Usage:
  SeriesColSpec("all")                         → all subjects × all age buckets
  SeriesColSpec("all", "total")                → all subjects × total bucket only
  SeriesColSpec("all", {"total", "age20_39"})  → all subjects × listed buckets
  SeriesColSpec({{"now_infectious","total"}, {"now_recovered","total"}})  → explicit list
  SeriesColSpec{{"now_infectious","total"}, {"now_recovered","total"}}    → initializer list
  SeriesColSpec{{"now_infectious","total","Jail"}, {"now_dead","total"}}  → mixed: ring-qualified + bare (aggregate)
*/
struct SeriesColSpec {
    std::vector<SeriesSelection> selections;

    // explicit vector of (name, bucket) pairs
    SeriesColSpec(std::vector<SeriesSelection> v) : selections(std::move(v)) {}

    // initializer list of (name, bucket) pairs
    SeriesColSpec(std::initializer_list<SeriesSelection> v) : selections(v) {}

    // "all" sentinel → all subjects × all age buckets
    SeriesColSpec(const char* sentinel);

    // "all" sentinel × single bucket
    SeriesColSpec(const char* sentinel, const char* bucket);

    // "all" sentinel × multiple buckets
    SeriesColSpec(const char* sentinel, std::vector<std::string> buckets);

private:
    static void validate_sentinel(const char* s);
    static std::vector<SeriesSelection> build_for_buckets(const std::vector<std::string>& buckets);
};

void print_total_status_series(const AllSeries& series, size_t days_per_block = 15);
void print_selected_series(SeriesColSpec spec, const AllSeries& series,
                           size_t days_per_block = 15);
void serialize_selected_series(SeriesColSpec spec, const AllSeries& series,
                           string base_fname, vector<string> path_steps={});
