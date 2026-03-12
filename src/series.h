#pragma once
#include "fmt/format.h"
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"


// clang-format off
// using enums to reference and access columns from an array of vectors...  we can have 127 columns
enum class SeriesColumns : uint8_t {now_infected, now_unexposed, now_recovered, now_dead,
    now_infected_0_19, now_unexposed_0_19, now_recovered_0_19, now_dead_0_19,
    now_infected_20_39, now_unexposed_20_39, now_recovered_20_39, now_dead_20_39,
    now_infected_40_59, now_unexposed_40_59, now_recovered_40_59, now_dead_40_59,
    now_infected_60_79, now_unexposed_60_79, now_recovered_60_79, now_dead_60_79,
    now_infected_80_up, now_unexposed_80_up, now_recovered_80_up, now_dead_80_up, 
COUNT };  // count, because it's last is the number of enum values
// clang-format on

using enum SeriesColumns; //sc = SeriesColumns; // shortcut

//clang-format off
// TODO this is very error prone to setup!
static inline MapEnum<SeriesColumns> series_map = {  // map literal: order doesn't matter
  {
    {"now_infected", now_infected}, {"now_unexposed", now_unexposed},
    {"now_recovered", now_recovered}, {"now_dead", now_dead},
    {"now_infected_0_19", now_infected_0_19}, {"now_unexposed_0_19", now_unexposed_0_19},
    {"now_recovered_0_19", now_recovered_0_19}, {"now_dead_0_19", now_dead_0_19},
    {"now_infected_20_39", now_infected_20_39}, {"now_unexposed_20_39", now_unexposed_20_39},
    {"now_recovered_20_39",now_recovered_20_39}, {"now_dead_20_39",now_dead_20_39},
    {"now_infected_40_59",now_infected_40_59}, {"now_unexposed_40_59",now_unexposed_40_59}, 
    {"now_recovered_40_59",now_recovered_40_59}, {"now_dead_40_59",now_dead_40_59},
    {"now_infected_60_79",now_infected_60_79}, {"now_unexposed_60_79",now_unexposed_60_79}, 
    {"now_recovered_60_79",now_recovered_60_79}, {"now_dead_60_79",now_dead_60_79},
    {"now_infected_80_up",now_infected_80_up}, {"now_unexposed_80_up",now_unexposed_80_up}, 
    {"now_recovered_80_up",now_recovered_80_up}, {"now_dead_80_up",now_dead_80_up}, 
  }
};
//clang-format on


/*
Use as follows:
create instance variable:         DayData series(day_count);   // day_count might be 180, 360, 720--add one to accommodate 1-indexing
update a specific vector and day: series[now_infected][12]++;
read a value:                     series[now_infected][12]

First index gets the vector from the array; second index gets the element from the vector.
*/
struct DayData {
    size_t day_cnt;  // will be actual days + 1
    std::array<std::vector<size_t>, size_t(SeriesColumns::COUNT)> cols;

    DayData(size_t day_cnt) : day_cnt(day_cnt) {
        for (auto& v : cols) v.assign(day_cnt + 1, 0uz);  // days are 1 indexed
    }

    // [] override enables indexing the container array with a SeriesColumns enum
    auto& operator[](SeriesColumns s) { return cols[size_t(s)]; }  
    auto const& operator[](SeriesColumns s) const { return cols[size_t(s)]; }
};

void update_series(const PopData & pop, DayData & series);
void print_total_status_series(const DayData& series, size_t days_per_block = 15);
void print_selected_series(std::vector<string> col_names, const DayData& series, size_t days_per_block=15); 