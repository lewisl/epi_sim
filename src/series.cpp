#include "series.h"
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"
#include "sim.h"

namespace {
auto total_status_labels = std::array{"infected", "unexposed", "recovered", "dead"};
auto total_status_names = std::array{
    SeriesName::now_infected, SeriesName::now_unexposed,
    SeriesName::now_recovered, SeriesName::now_dead};

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

AgeBucket bucket_from_age(Agegrp agegrp) {
  switch (agegrp.v) {
    case Age::Age0_19.v:
      return AgeBucket::age0_19;
    case Age::Age20_39.v:
      return AgeBucket::age20_39;
    case Age::Age40_59.v:
      return AgeBucket::age40_59;
    case Age::Age60_79.v:
      return AgeBucket::age60_79;
    case Age::Age80_up.v:
      return AgeBucket::age80_up;
    default:
      throw std::runtime_error("Unknown age group for series bucket mapping");
  }
}

void increment_series(DayData& series, SeriesName name, Agegrp agegrp, size_t day) {
  series.at(name, AgeBucket::total)[day]++;
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
  for (auto bucket : all_age_buckets) {
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

    for (auto [label, name] : std::views::zip(total_status_labels, total_status_names)) {
      fmt::print("{:<12}", label);
      for (size_t day = block_start; day <= block_end; ++day) {
        fmt::print("{:>8}", series.at(name, AgeBucket::total)[day]);
      }
      fmt::println("");
    }
  }
}

void print_selected_series(std::vector<SeriesSelection> selections, const DayData& series,
                           size_t days_per_block) {
  if (series.day_cnt == 0) {
    fmt::println("\nNo day series to print.");
    return;
  }

  if (selections.empty()) {
    fmt::println("\nNo series selected.");
    return;
  }

  vector<string> invalid_selections;
  vector<std::pair<SeriesName, AgeBucket>> cols;
  vector<string> labels;
  cols.reserve(selections.size());
  labels.reserve(selections.size());
  for (const auto& [name_text, bucket_text] : selections) {
    auto name = series_name_from_string(name_text);
    auto bucket = age_bucket_from_string(bucket_text);
    if (!name || !bucket) {
      invalid_selections.push_back(fmt::format("{}:{}", name_text, bucket_text));
      continue;
    }
    cols.emplace_back(*name, *bucket);
    const auto full_label = fmt::format("{}:{}", to_string(*name), to_string(*bucket));
    labels.push_back(full_label.size() < 12 ? full_label : fmt::format("Series {}", labels.size()));
  }

  if (!invalid_selections.empty()) {
    fmt::println("\nUnknown series selections: {}", invalid_selections);
    return;
  }

  vector<string> selected_labels;
  selected_labels.reserve(cols.size());
  for (const auto& [name, bucket] : cols) {
    selected_labels.push_back(fmt::format("{}:{}", to_string(name), to_string(bucket)));
  }

  fmt::println("\nSelected series: {}", selected_labels);
  for (size_t block_start = 1; block_start <= series.day_cnt; block_start += days_per_block) {
    const size_t block_end = std::min(block_start + days_per_block - 1, series.day_cnt);
    fmt::println("\nDays {}-{}", block_start, block_end);

    fmt::print("{:<12}", "day");
    for (size_t day = block_start; day <= block_end; ++day) {
      fmt::print("{:>8}", day);
    }
    fmt::println("");

    for (auto [label, col] : std::views::zip(labels, cols)) {
      const auto& [name, bucket] = col;
      fmt::print("{:<12}", label);
      for (size_t day = block_start; day <= block_end; ++day) {
        fmt::print("{:>8}", series.at(name, bucket)[day]);
      }
      fmt::println("");
    }
  }
}
