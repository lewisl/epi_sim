#pragma once
#include "fmt/format.h"
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"


enum class SeriesName : uint8_t { now_infected, now_unexposed, now_recovered, now_dead,
                                  now_vaccinated,
                                  new_infected, new_recovered, net_infected, new_dead,
                                  new_vaccinated,
                                  COUNT };

enum class AgeBucket : uint8_t { total, age0_19, age20_39, age40_59, age60_79, age80_up, COUNT };

using SeriesSelection = std::pair<string, string>;

inline constexpr auto all_series_names = std::array{
    SeriesName::now_infected, SeriesName::now_unexposed, SeriesName::now_recovered,
    SeriesName::now_dead, SeriesName::now_vaccinated,
    SeriesName::new_infected, SeriesName::new_recovered,
    SeriesName::net_infected, SeriesName::new_dead, SeriesName::new_vaccinated};

inline constexpr auto all_age_buckets = std::array{
    AgeBucket::total, AgeBucket::age0_19, AgeBucket::age20_39,
    AgeBucket::age40_59, AgeBucket::age60_79, AgeBucket::age80_up};

inline constexpr auto age_only_buckets = std::array{
    AgeBucket::age0_19, AgeBucket::age20_39, AgeBucket::age40_59,
    AgeBucket::age60_79, AgeBucket::age80_up};

inline constexpr auto series_name_labels = std::array{
    "now_infected", "now_unexposed", "now_recovered", "now_dead", "now_vaccinated",
    "new_infected", "new_recovered", "net_infected", "new_dead", "new_vaccinated"};

inline constexpr auto age_bucket_labels = std::array{
    "total", "age0_19", "age20_39", "age40_59", "age60_79", "age80_up"};

static_assert(all_series_names.size() == size_t(SeriesName::COUNT));
static_assert(all_age_buckets.size() == size_t(AgeBucket::COUNT));
static_assert(series_name_labels.size() == size_t(SeriesName::COUNT));
static_assert(age_bucket_labels.size() == size_t(AgeBucket::COUNT));

constexpr std::string_view to_string(SeriesName name) {
    return series_name_labels[size_t(name)];
}

constexpr std::string_view to_string(AgeBucket bucket) {
    return age_bucket_labels[size_t(bucket)];
}

inline std::optional<SeriesName> series_name_from_string(std::string_view text) {
    for (size_t i = 0; i < series_name_labels.size(); ++i) {
        if (series_name_labels[i] == text) return static_cast<SeriesName>(i);
    }
    return std::nullopt;
}

inline std::optional<AgeBucket> age_bucket_from_string(std::string_view text) {
    for (size_t i = 0; i < age_bucket_labels.size(); ++i) {
        if (age_bucket_labels[i] == text) return static_cast<AgeBucket>(i);
    }
    return std::nullopt;
}


/*
Use as follows:
create instance variable:         DayData series(day_count);   // pass actual simulated days; storage adds index 0 for 1-indexing
update a specific vector and day: series.at(SeriesName::now_infected, AgeBucket::total)[12]++;
read a value:                     series.at(SeriesName::now_infected, AgeBucket::total)[12]

`new_*` series are transition-time flows, while `now_*` series are end-of-day stocks.
`DayData` stores them by semantic name first, then by age bucket.
*/
struct DayData {
    using BucketSeries = std::array<std::vector<int>, size_t(AgeBucket::COUNT)>;

    size_t day_cnt;  // actual simulated day count; vectors allocate one extra slot for unused index 0
    std::array<BucketSeries, size_t(SeriesName::COUNT)> cols;

    DayData(size_t day_cnt) : day_cnt(day_cnt) {
        for (auto& group : cols) {
            for (auto& v : group) v.assign(day_cnt + 1, 0);  // days are 1 indexed
        }
    }

    auto& at(SeriesName name, AgeBucket bucket) { return cols[size_t(name)][size_t(bucket)]; }
    auto const& at(SeriesName name, AgeBucket bucket) const {
        return cols[size_t(name)][size_t(bucket)];
    }

    auto& group(SeriesName name) { return cols[size_t(name)]; }
    auto const& group(SeriesName name) const { return cols[size_t(name)]; }
};



AgeBucket bucket_from_age(Agegrp agegrp);
void increment_series(DayData& series, SeriesName name, Agegrp agegrp, size_t day);
void update_series(const PopData & pop, DayData & series);
void finalize_series(DayData& series);
void write_daily_trace_csv(const std::filesystem::path& output_path,
                           const std::vector<absl::CivilDay>& caldays,
                           const DayData& series);
void print_total_status_series(const DayData& series, size_t days_per_block = 15);
void print_selected_series(std::vector<SeriesSelection> selections, const DayData& series,
                           size_t days_per_block = 15);
