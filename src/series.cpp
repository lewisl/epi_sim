#include "series.h"
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"
#include "sim.h"

namespace {
auto ages = std::array{Age::Age0_19, Age::Age20_39, Age::Age40_59, Age::Age60_79,
                       Age::Age80_up};
auto stats = std::array{Stat::Infectious, Stat::Unexposed, Stat::Recovered, Stat::Dead};
auto stat_cols = std::array{sc::now_infected, sc::now_unexposed, sc::now_recovered,
                            sc::now_dead};
auto recovered_byage =
    std::array{sc::now_recovered_0_19, sc::now_recovered_20_39, sc::now_recovered_40_59,
               sc::now_recovered_60_79, sc::now_recovered_80_up};
auto infected_byage =
    std::array{sc::now_infected_0_19, sc::now_infected_20_39, sc::now_infected_40_59,
               sc::now_infected_60_79, sc::now_infected_80_up};
auto dead_byage =
    std::array{sc::now_dead_0_19, sc::now_dead_20_39, sc::now_dead_40_59,
               sc::now_dead_60_79, sc::now_dead_80_up};
auto unexposed_byage =
    std::array{sc::now_unexposed_0_19, sc::now_unexposed_20_39, sc::now_unexposed_40_59,
               sc::now_unexposed_60_79, sc::now_unexposed_80_up};

// Order by_age columns by status: must match status order in `stats`.
auto stat_byage_group =
    std::array{infected_byage, unexposed_byage, recovered_byage, dead_byage};
auto total_status_labels = std::array{"infected", "unexposed", "recovered", "dead"};

template <size_t N>
void update_age_column(DayData& series, size_t day, Agegrp agegrp,
                       const std::array<SeriesColumns, N>& byage_cols) {
  switch (agegrp.v) {
    case Age::Age0_19.v:
      series[byage_cols[0]][day]++;
      break;
    case Age::Age20_39.v:
      series[byage_cols[1]][day]++;
      break;
    case Age::Age40_59.v:
      series[byage_cols[2]][day]++;
      break;
    case Age::Age60_79.v:
      series[byage_cols[3]][day]++;
      break;
    case Age::Age80_up.v:
      series[byage_cols[4]][day]++;
      break;
    default:
      break;
  }
}


/*
this was the first pass. It works. It was 18% slower so not bad. It is very compact code.  
But, it is a bear to maintain.
*/
void update_series_nested(const PopData & pop, DayData & series) {
  auto d = sim::ds.day;
  for (size_t i = 1; i <= pop.popn; ++i) {
    for (auto [stat, stat_col, stat_by_ages] : std::views::zip(stats, stat_cols, stat_byage_group)) {
      if (pop.status[i] == stat) {
        series[stat_col][d]++;
        for (auto [age, stat_byage] : std::views::zip(ages, stat_by_ages)) {
          if (pop.agegrp[i] == age) {
            series[stat_byage][d]++;
          }
        }
      }
    }
  }
}
} // namespace

void update_series(const PopData & pop, DayData & series) {
  auto d = sim::ds.day;
  for (size_t i = 1; i <= pop.popn; ++i) {
    const auto status = pop.status[i];  // person i's status
    const auto agegrp = pop.agegrp[i];  // person i's agegrp

    switch (status.v) {
      case Stat::Infectious.v:
        series[sc::now_infected][d]++;
        update_age_column(series, d, agegrp, infected_byage);  // last arg is the array of series columns
        break;
      case Stat::Unexposed.v:
        series[sc::now_unexposed][d]++;
        update_age_column(series, d, agegrp, unexposed_byage);
        break;
      case Stat::Recovered.v:
        series[sc::now_recovered][d]++;
        update_age_column(series, d, agegrp, recovered_byage);
        break;
      case Stat::Dead.v:
        series[sc::now_dead][d]++;
        update_age_column(series, d, agegrp, dead_byage);
        break;
      default:
        break;
    }
  }
}

void print_total_status_series(const DayData& series, size_t days_per_block) {
  if (series.day_cnt == 0) {
    fmt::println("\nNo day series to print.");
    return;
  }

  fmt::println("\nTotal status series");
  for (size_t block_start = 1; block_start <= series.day_cnt; block_start += days_per_block) {
    const size_t block_end = std::min(block_start + days_per_block - 1, series.day_cnt);
    fmt::println("\nDays {}-{}", block_start, block_end);

    fmt::print("{:<12}", "day");
    for (size_t day = block_start; day <= block_end; ++day) {
      fmt::print("{:>8}", day);
    }
    fmt::println("");

    for (auto [label, col] : std::views::zip(total_status_labels, stat_cols)) {
      fmt::print("{:<12}", label);
      for (size_t day = block_start; day <= block_end; ++day) {
        fmt::print("{:>8}", series[col][day]);
      }
      fmt::println("");
    }
  }
}
