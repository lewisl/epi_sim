#pragma once
#include "fmt/format.h"
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"


enum class SeriesName : uint8_t { now_infected, now_unexposed, now_recovered, now_dead,
                                  new_infected, new_recovered, net_infected, new_dead,
                                  COUNT };

enum class SeriesBucket : uint8_t { total, age0_19, age20_39, age40_59, age60_79, age80_up,
                                    COUNT };

struct SeriesKey {
    SeriesName name;
    SeriesBucket bucket;

    bool operator==(const SeriesKey&) const = default;  // equality of SeriesKey requires "default" equality of all members:
                    // totally non-obvious way to specify it:  perfect c++ syntax where obvious doesn't reward the cargo cult
};

inline constexpr auto all_series_buckets = std::array{
    SeriesBucket::total, SeriesBucket::age0_19, SeriesBucket::age20_39,
    SeriesBucket::age40_59, SeriesBucket::age60_79, SeriesBucket::age80_up};

inline constexpr auto age_series_buckets = std::array{
    SeriesBucket::age0_19, SeriesBucket::age20_39, SeriesBucket::age40_59,
    SeriesBucket::age60_79, SeriesBucket::age80_up};

// Stock series: total plus age-specific aliases.
inline constexpr SeriesKey now_infected{SeriesName::now_infected, SeriesBucket::total};
inline constexpr SeriesKey now_unexposed{SeriesName::now_unexposed, SeriesBucket::total};
inline constexpr SeriesKey now_recovered{SeriesName::now_recovered, SeriesBucket::total};
inline constexpr SeriesKey now_dead{SeriesName::now_dead, SeriesBucket::total};
inline constexpr SeriesKey now_infected_0_19{SeriesName::now_infected, SeriesBucket::age0_19};
inline constexpr SeriesKey now_unexposed_0_19{SeriesName::now_unexposed, SeriesBucket::age0_19};
inline constexpr SeriesKey now_recovered_0_19{SeriesName::now_recovered, SeriesBucket::age0_19};
inline constexpr SeriesKey now_dead_0_19{SeriesName::now_dead, SeriesBucket::age0_19};
inline constexpr SeriesKey now_infected_20_39{SeriesName::now_infected, SeriesBucket::age20_39};
inline constexpr SeriesKey now_unexposed_20_39{SeriesName::now_unexposed, SeriesBucket::age20_39};
inline constexpr SeriesKey now_recovered_20_39{SeriesName::now_recovered, SeriesBucket::age20_39};
inline constexpr SeriesKey now_dead_20_39{SeriesName::now_dead, SeriesBucket::age20_39};
inline constexpr SeriesKey now_infected_40_59{SeriesName::now_infected, SeriesBucket::age40_59};
inline constexpr SeriesKey now_unexposed_40_59{SeriesName::now_unexposed, SeriesBucket::age40_59};
inline constexpr SeriesKey now_recovered_40_59{SeriesName::now_recovered, SeriesBucket::age40_59};
inline constexpr SeriesKey now_dead_40_59{SeriesName::now_dead, SeriesBucket::age40_59};
inline constexpr SeriesKey now_infected_60_79{SeriesName::now_infected, SeriesBucket::age60_79};
inline constexpr SeriesKey now_unexposed_60_79{SeriesName::now_unexposed, SeriesBucket::age60_79};
inline constexpr SeriesKey now_recovered_60_79{SeriesName::now_recovered, SeriesBucket::age60_79};
inline constexpr SeriesKey now_dead_60_79{SeriesName::now_dead, SeriesBucket::age60_79};
inline constexpr SeriesKey now_infected_80_up{SeriesName::now_infected, SeriesBucket::age80_up};
inline constexpr SeriesKey now_unexposed_80_up{SeriesName::now_unexposed, SeriesBucket::age80_up};
inline constexpr SeriesKey now_recovered_80_up{SeriesName::now_recovered, SeriesBucket::age80_up};
inline constexpr SeriesKey now_dead_80_up{SeriesName::now_dead, SeriesBucket::age80_up};

// Flow series: total plus age-specific aliases.
inline constexpr SeriesKey new_infected{SeriesName::new_infected, SeriesBucket::total};
inline constexpr SeriesKey new_infected_0_19{SeriesName::new_infected, SeriesBucket::age0_19};
inline constexpr SeriesKey new_infected_20_39{SeriesName::new_infected, SeriesBucket::age20_39};
inline constexpr SeriesKey new_infected_40_59{SeriesName::new_infected, SeriesBucket::age40_59};
inline constexpr SeriesKey new_infected_60_79{SeriesName::new_infected, SeriesBucket::age60_79};
inline constexpr SeriesKey new_infected_80_up{SeriesName::new_infected, SeriesBucket::age80_up};

inline constexpr SeriesKey new_recovered{SeriesName::new_recovered, SeriesBucket::total};
inline constexpr SeriesKey new_recovered_0_19{SeriesName::new_recovered, SeriesBucket::age0_19};
inline constexpr SeriesKey new_recovered_20_39{SeriesName::new_recovered, SeriesBucket::age20_39};
inline constexpr SeriesKey new_recovered_40_59{SeriesName::new_recovered, SeriesBucket::age40_59};
inline constexpr SeriesKey new_recovered_60_79{SeriesName::new_recovered, SeriesBucket::age60_79};
inline constexpr SeriesKey new_recovered_80_up{SeriesName::new_recovered, SeriesBucket::age80_up};

inline constexpr SeriesKey net_infected{SeriesName::net_infected, SeriesBucket::total};
inline constexpr SeriesKey net_infected_0_19{SeriesName::net_infected, SeriesBucket::age0_19};
inline constexpr SeriesKey net_infected_20_39{SeriesName::net_infected, SeriesBucket::age20_39};
inline constexpr SeriesKey net_infected_40_59{SeriesName::net_infected, SeriesBucket::age40_59};
inline constexpr SeriesKey net_infected_60_79{SeriesName::net_infected, SeriesBucket::age60_79};
inline constexpr SeriesKey net_infected_80_up{SeriesName::net_infected, SeriesBucket::age80_up};

inline constexpr SeriesKey new_dead{SeriesName::new_dead, SeriesBucket::total};
inline constexpr SeriesKey new_dead_0_19{SeriesName::new_dead, SeriesBucket::age0_19};
inline constexpr SeriesKey new_dead_20_39{SeriesName::new_dead, SeriesBucket::age20_39};
inline constexpr SeriesKey new_dead_40_59{SeriesName::new_dead, SeriesBucket::age40_59};
inline constexpr SeriesKey new_dead_60_79{SeriesName::new_dead, SeriesBucket::age60_79};
inline constexpr SeriesKey new_dead_80_up{SeriesName::new_dead, SeriesBucket::age80_up};

// TODO this is verbose, but keeps external column names stable for printing and lookup.
static inline absl::flat_hash_map<string, SeriesKey> series_map = {
    {"now_infected", now_infected}, {"now_unexposed", now_unexposed},
    {"now_recovered", now_recovered}, {"now_dead", now_dead},
    {"now_infected_0_19", now_infected_0_19}, {"now_unexposed_0_19", now_unexposed_0_19},
    {"now_recovered_0_19", now_recovered_0_19}, {"now_dead_0_19", now_dead_0_19},
    {"now_infected_20_39", now_infected_20_39}, {"now_unexposed_20_39", now_unexposed_20_39},
    {"now_recovered_20_39", now_recovered_20_39}, {"now_dead_20_39", now_dead_20_39},
    {"now_infected_40_59", now_infected_40_59}, {"now_unexposed_40_59", now_unexposed_40_59},
    {"now_recovered_40_59", now_recovered_40_59}, {"now_dead_40_59", now_dead_40_59},
    {"now_infected_60_79", now_infected_60_79}, {"now_unexposed_60_79", now_unexposed_60_79},
    {"now_recovered_60_79", now_recovered_60_79}, {"now_dead_60_79", now_dead_60_79},
    {"now_infected_80_up", now_infected_80_up}, {"now_unexposed_80_up", now_unexposed_80_up},
    {"now_recovered_80_up", now_recovered_80_up}, {"now_dead_80_up", now_dead_80_up},
    {"new_infected", new_infected}, {"new_infected_0_19", new_infected_0_19},
    {"new_infected_20_39", new_infected_20_39}, {"new_infected_40_59", new_infected_40_59},
    {"new_infected_60_79", new_infected_60_79}, {"new_infected_80_up", new_infected_80_up},
    {"new_recovered", new_recovered}, {"new_recovered_0_19", new_recovered_0_19},
    {"new_recovered_20_39", new_recovered_20_39}, {"new_recovered_40_59", new_recovered_40_59},
    {"new_recovered_60_79", new_recovered_60_79}, {"new_recovered_80_up", new_recovered_80_up},
    {"net_infected", net_infected}, {"net_infected_0_19", net_infected_0_19},
    {"net_infected_20_39", net_infected_20_39}, {"net_infected_40_59", net_infected_40_59},
    {"net_infected_60_79", net_infected_60_79}, {"net_infected_80_up", net_infected_80_up},
    {"new_dead", new_dead}, {"new_dead_0_19", new_dead_0_19},
    {"new_dead_20_39", new_dead_20_39}, {"new_dead_40_59", new_dead_40_59},
    {"new_dead_60_79", new_dead_60_79}, {"new_dead_80_up", new_dead_80_up},
};


/*
Use as follows:
create instance variable:         DayData series(day_count);   // day_count might be 180, 360, 720--add one to accommodate 1-indexing
update a specific vector and day: series[now_infected][12]++;
read a value:                     series[now_infected][12]

`new_*` series are transition-time flows, while `now_*` series are end-of-day stocks.
`DayData` stores them by semantic name first, then by bucket (total plus age groups).
*/
struct DayData {
    using BucketSeries = std::array<std::vector<size_t>, size_t(SeriesBucket::COUNT)>;  // shorthand for type

    size_t day_cnt;  // will be actual days + 1
    std::array<BucketSeries, size_t(SeriesName::COUNT)> cols;

    DayData(size_t day_cnt) : day_cnt(day_cnt) {
        for (auto& group : cols) {
            for (auto& v : group) v.assign(day_cnt + 1, 0uz);  // days are 1 indexed
        }
    }

    auto& operator[](SeriesKey key) { return cols[size_t(key.name)][size_t(key.bucket)]; }
    auto const& operator[](SeriesKey key) const { return cols[size_t(key.name)][size_t(key.bucket)]; }

    auto& at(SeriesName name, SeriesBucket bucket) { return cols[size_t(name)][size_t(bucket)]; }
    auto const& at(SeriesName name, SeriesBucket bucket) const {
        return cols[size_t(name)][size_t(bucket)];
    }

    auto& group(SeriesName name) { return cols[size_t(name)]; }
    auto const& group(SeriesName name) const { return cols[size_t(name)]; }
};

SeriesBucket bucket_from_age(Agegrp agegrp);
void increment_series(DayData& series, SeriesName name, Agegrp agegrp, size_t day);
void update_series(const PopData & pop, DayData & series);
void finalize_series(DayData& series);
void print_total_status_series(const DayData& series, size_t days_per_block = 15);
void print_selected_series(std::vector<string> col_names, const DayData& series, size_t days_per_block=15); 
