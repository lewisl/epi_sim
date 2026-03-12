#include "series.h"
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"
#include "sim.h"

using enum SeriesColumns;



namespace {
auto total_status_labels = std::array{"infected", "unexposed", "recovered", "dead"};
auto ages = std::array{Age::Age0_19, Age::Age20_39, Age::Age40_59, Age::Age60_79,
                       Age::Age80_up};
auto stats = std::array{Stat::Infectious, Stat::Unexposed, Stat::Recovered, Stat::Dead};
auto stat_cols = std::array{now_infected, now_unexposed, now_recovered, now_dead};
auto recovered_byage =
    std::array{now_recovered_0_19, now_recovered_20_39, now_recovered_40_59,
               now_recovered_60_79, now_recovered_80_up};
auto infected_byage =
    std::array{now_infected_0_19, now_infected_20_39, now_infected_40_59,
               now_infected_60_79, now_infected_80_up};
auto dead_byage =
    std::array{now_dead_0_19, now_dead_20_39, now_dead_40_59,
               now_dead_60_79, now_dead_80_up};
auto unexposed_byage =
    std::array{now_unexposed_0_19, now_unexposed_20_39, now_unexposed_40_59,
               now_unexposed_60_79, now_unexposed_80_up};
}

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
Adds one day of simulation outcomes to the history series.
Very hardwired to the columns in PopData and DayData.
*/
void update_series(const PopData & pop, DayData & series) {
  auto d = sim::ds.day;
  for (size_t i = 1; i <= pop.popn; ++i) {
    const auto status = pop.status[i];  // person i's status
    const auto agegrp = pop.agegrp[i];  // person i's agegrp

    switch (status.v) {
      case Stat::Infectious.v:
        series[now_infected][d]++;
        update_age_column(series, d, agegrp, infected_byage);  // last arg is the array of series columns
        break;
      case Stat::Unexposed.v:
        series[now_unexposed][d]++;
        update_age_column(series, d, agegrp, unexposed_byage);
        break;
      case Stat::Recovered.v:
        series[now_recovered][d]++;
        update_age_column(series, d, agegrp, recovered_byage);
        break;
      case Stat::Dead.v:
        series[now_dead][d]++;
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

void print_selected_series(std::vector<string> col_names, const DayData& series, size_t days_per_block) {
  if (series.day_cnt == 0) {
    fmt::println("\nNo day series to print.");
    return;
  }

  if (col_names.empty()) {
    fmt::println("\nNo series selected.");
    return;
  }

  size_t numseries = col_names.size();

  vector<SeriesColumns> cols(numseries);
  vector<string> invalid_names;
  vector<string> labels(numseries);
  size_t idx {};  // zero initialized
  for (auto cname : col_names) {
    auto col = series_map.to_int(cname);
    if (!col.has_value()) {
      invalid_names.push_back(cname);
      continue;
    }
    cols[idx] = *col;
    labels[idx] = cname.size() < 12 ? cname : fmt::format("Series {}", idx);
    ++idx;
  }

  if (!invalid_names.empty()) {
    fmt::println("\nUnknown series columns: {}", invalid_names);
    return;
  }

  fmt::println("\nSelected series: {}", col_names);
  for (size_t block_start = 1; block_start <= series.day_cnt; block_start += days_per_block) {
    const size_t block_end = std::min(block_start + days_per_block - 1, series.day_cnt);
    fmt::println("\nDays {}-{}", block_start, block_end);

    fmt::print("{:<12}", "day");
    for (size_t day = block_start; day <= block_end; ++day) {
      fmt::print("{:>8}", day);
    }
    fmt::println("");

    for (auto [label, col] : std::views::zip(labels, cols)) {
      fmt::print("{:<12}", label);
      for (size_t day = block_start; day <= block_end; ++day) {
        fmt::print("{:>8}", series[col][day]);
      }
      fmt::println("");
    }
  }
}
