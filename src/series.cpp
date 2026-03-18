#include "series.h"
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"
#include "sim.h"

namespace {
auto total_status_labels = std::array{"infected", "unexposed", "recovered", "dead"};
auto stat_cols = std::array{now_infected, now_unexposed, now_recovered, now_dead};

void diff_from_cumulative(std::span<const size_t> src, std::span<size_t> dest) {
  assert(src.size() == dest.size());
  if (src.size() <= 1) return;  // only the unused 0 slot exists

  // Day series are 1-indexed: day 1 copies through, later days subtract previous from current.
  dest[1] = src[1];
  for (size_t day = 2; day < src.size(); ++day) {
    dest[day] = src[day] - src[day - 1];
  }
}
}

SeriesBucket bucket_from_age(Agegrp agegrp) {
  switch (agegrp.v) {
    case Age::Age0_19.v:
      return SeriesBucket::age0_19;
    case Age::Age20_39.v:
      return SeriesBucket::age20_39;
    case Age::Age40_59.v:
      return SeriesBucket::age40_59;
    case Age::Age60_79.v:
      return SeriesBucket::age60_79;
    case Age::Age80_up.v:
      return SeriesBucket::age80_up;
    default:
      throw std::runtime_error("Unknown age group for series bucket mapping");
  }
}

void increment_series(DayData& series, SeriesName name, Agegrp agegrp, size_t day) {
  series.at(name, SeriesBucket::total)[day]++;
  series.at(name, bucket_from_age(agegrp))[day]++;
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
        increment_series(series, SeriesName::now_infected, agegrp, d);
        break;
      case Stat::Unexposed.v:
        increment_series(series, SeriesName::now_unexposed, agegrp, d);
        break;
      case Stat::Recovered.v:
        increment_series(series, SeriesName::now_recovered, agegrp, d);
        break;
      case Stat::Dead.v:
        increment_series(series, SeriesName::now_dead, agegrp, d);
        break;
      default:
        break;
    }
  }
}

void finalize_series(DayData& series) {
  for (auto bucket : all_series_buckets) {
    diff_from_cumulative(std::span<const size_t>(series.at(SeriesName::now_infected, bucket)),
                         std::span<size_t>(series.at(SeriesName::net_infected, bucket)));
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

  vector<string> invalid_names;
  vector<SeriesKey> cols;
  vector<string> labels;
  cols.reserve(col_names.size());
  labels.reserve(col_names.size());
  for (auto cname : col_names) {
    auto it = series_map.find(cname);
    if (it == series_map.end()) {
      invalid_names.push_back(cname);
      continue;
    }
    cols.push_back(it->second);
    labels.push_back(cname.size() < 12 ? cname : fmt::format("Series {}", labels.size()));
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
