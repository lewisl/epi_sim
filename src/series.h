#pragma once
#include "parameters.h"
#include "population.h"
#include <cstdint>
#include <initializer_list>


enum class AgeBucket : uint8_t { total, age0_19, age20_39, age40_59, age60_79, age80_up, COUNT };

// Reserved ring slot holding the all-rings aggregate. Reuses the unused
// sentinel index so no real person (rings are 1-based) ever collides with it.
inline constexpr uint8_t RING_ALL = 0;

struct SeriesSelection {
    std::string name;        // this must be a string that matches -> presumably a subject FINISH YOUR DANM COMMENTS!
    std::string bucket;      // this must be a string that matches an AgeBucket enum
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

AgeBucket bucket_from_age(Agegrp agegrp);

// becomes TraitGroup
enum class SeriesBlock : uint8_t {
    now_status,
    new_status,
    now_vax,
    new_vax,
    now_variant,
    new_variant,
    COUNT
};

inline constexpr auto all_series_blocks = std::array{
    SeriesBlock::now_status, SeriesBlock::new_status,
    SeriesBlock::now_vax, SeriesBlock::new_vax,
    SeriesBlock::now_variant, SeriesBlock::new_variant};

static_assert(all_series_blocks.size() == size_t(SeriesBlock::COUNT));

using SeriesValue = std::int32_t;
using SeriesColumn = std::vector<SeriesValue>;
using SeriesColumnIndex = size_t;

struct SeriesBlockDescriptor {
    SeriesColumnIndex base_col{};
    size_t subject_count{};
    size_t subject_stride{};
    size_t ring_stride{};
    size_t column_count{};
    bool is_stock{};
};

class SeriesColumnMap {
public:
    SeriesColumnMap(size_t n_status, size_t n_vax, size_t n_variants,
                    size_t n_rings);

    // calculates the index of a series in AllSeries
    [[clang::always_inline]] SeriesColumnIndex column_index(
        SeriesBlock block, uint8_t subject_idx, AgeBucket bucket,
        uint8_t ring = RING_ALL) const {
        const auto& desc = descriptors_[size_t(block)];
        return desc.base_col
             + size_t(subject_idx) * desc.subject_stride
             + size_t(ring) * desc.ring_stride
             + size_t(bucket);
    }

    [[clang::always_inline]] const SeriesBlockDescriptor& descriptor(
        SeriesBlock block) const {
        return descriptors_[size_t(block)];
    }

    size_t n_rings() const { return n_rings_; }
    size_t total_columns() const { return total_columns_; }

private:
    std::array<SeriesBlockDescriptor, size_t(SeriesBlock::COUNT)> descriptors_{};
    size_t n_rings_{};
    size_t total_columns_{};
};


/*
AllSeries: top-level container for all time-series data in the simulation.

Nesting:
  AllSeries (one instance, passed by reference everywhere)
    vector of columns (indexed through SeriesColumnMap)
      vector<int32_t> (indexed by simulation day, 1-based; slot 0 unused)
*/
struct AllSeries {
    size_t day_cnt;

    AllSeries(size_t day_cnt, const PopData& pop,
              size_t n_variants, size_t n_vax, size_t n_rings);

    [[clang::always_inline]] SeriesColumnIndex column_index(
        SeriesBlock block, uint8_t subject_idx, AgeBucket bucket,
        uint8_t ring = RING_ALL) const {
        return column_map_.column_index(block, subject_idx, bucket, ring);
    }

    [[clang::always_inline]] SeriesColumn& column(SeriesColumnIndex col) {
        return cols_[col];
    }
    [[clang::always_inline]] const SeriesColumn& column(SeriesColumnIndex col) const {
        return cols_[col];
    }

    [[clang::always_inline]] SeriesColumn& at(
        SeriesBlock block, uint8_t subject_idx, AgeBucket bucket,
        uint8_t ring = RING_ALL) {
        return cols_[column_index(block, subject_idx, bucket, ring)];
    }
    [[clang::always_inline]] const SeriesColumn& at(
        SeriesBlock block, uint8_t subject_idx, AgeBucket bucket,
        uint8_t ring = RING_ALL) const {
        return cols_[column_index(block, subject_idx, bucket, ring)];
    }

    [[clang::always_inline]] void update(
        SeriesBlock block, uint8_t subject_idx, uint8_t ring, Agegrp agegrp,
        size_t day, SeriesValue change) {
        const auto& desc = column_map_.descriptor(block);
        const size_t bucket = size_t(bucket_from_age(agegrp));
        const size_t subject_base = desc.base_col + size_t(subject_idx) * desc.subject_stride;
        const size_t ring_base = subject_base + size_t(ring) * desc.ring_stride;

        cols_[ring_base + bucket][day] += change;
        cols_[ring_base + size_t(AgeBucket::total)][day] += change;
        if (ring != RING_ALL) {
            cols_[subject_base + bucket][day] += change;
            cols_[subject_base + size_t(AgeBucket::total)][day] += change;
        }
    }

    const SeriesBlockDescriptor& block_descriptor(SeriesBlock block) const {
        return column_map_.descriptor(block);
    }
    size_t n_rings() const { return column_map_.n_rings(); }
    size_t column_count() const { return column_map_.total_columns(); }

    void init_history_series(size_t day);
    void finalize_series();

    // Checks that sum(now_variant[v][total][day]) == now_status[INFECTIOUS][total][day]
    // for every simulated day. Throws on the first mismatch; prints OK if all pass.
    void validate_variant_invariant() const;

private:
    SeriesColumnMap column_map_;
    std::vector<SeriesColumn> cols_;
};

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

struct ResolvedSeriesCol {
    std::string label;
    SeriesColumn data;
};

struct ResolvedSeriesSelection {
    std::vector<ResolvedSeriesCol> cols;
    std::vector<std::string> invalid_selections;
};

ResolvedSeriesSelection resolve_selected_series(const SeriesColSpec& spec,
                                                const AllSeries& series);

void print_total_status_series(const AllSeries& series, size_t days_per_block = 15);
void print_selected_series(SeriesColSpec spec, const AllSeries& series,
                           size_t days_per_block = 15);
void serialize_selected_series(SeriesColSpec spec, const AllSeries& series,
                           std::filesystem::path output_path);
void serialize_selected_series(SeriesColSpec spec, const AllSeries& series,
                           string base_fname, vector<string> path_steps={});
