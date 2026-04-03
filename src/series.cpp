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

std::string format_civil_day(absl::CivilDay day) {
  return fmt::format("{:04d}-{:02d}-{:02d}", day.year(), unsigned(day.month()), day.day());
}

void ensure_parent_dir(const std::filesystem::path& output_path) {
  auto parent = output_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
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

void init_history_series(HistorySeries & series, size_t day) {
    if (day <= 1) return;
    for (auto bucket : all_age_buckets) {
        series.at(SeriesName::now_infected,  bucket)[day] = series.at(SeriesName::now_infected,  bucket)[day-1];
        series.at(SeriesName::now_unexposed, bucket)[day] = series.at(SeriesName::now_unexposed, bucket)[day-1];
        series.at(SeriesName::now_recovered, bucket)[day] = series.at(SeriesName::now_recovered, bucket)[day-1];
        series.at(SeriesName::now_dead,      bucket)[day] = series.at(SeriesName::now_dead,      bucket)[day-1];
        series.at(SeriesName::now_vaccinated,bucket)[day] = series.at(SeriesName::now_vaccinated,bucket)[day-1];
    }
}


void delta_series(HistorySeries& series, SeriesName name, Agegrp agegrp, size_t day, int change) {
  series.at(name, AgeBucket::total)[day] += change;
  series.at(name, bucket_from_age(agegrp))[day] += change;
}

/*
Adds one day of simulation outcomes to the history series.
Very hardwired to the columns in PopData and HistorySeries.
*/
// void old_update_series(const PopData & pop, HistorySeries & series) {
//   auto d = sim::ds.day;
//   for (size_t i = 1; i <= pop.popn; ++i) {
//     const auto status = pop.status[i];  // person i's status
//     const auto agegrp = pop.agegrp[i];  // person i's agegrp

//     switch (status.v) {
//       case Stat::Infectious.v:
//         increment_series(series, SeriesName::now_infected, agegrp, d);
//         break;
//       case Stat::Unexposed.v:
//         increment_series(series, SeriesName::now_unexposed, agegrp, d);
//         break;
//       case Stat::Recovered.v:
//         increment_series(series, SeriesName::now_recovered, agegrp, d);
//         break;
//       case Stat::Dead.v:
//         increment_series(series, SeriesName::now_dead, agegrp, d);
//         break;
//       default:
//         break;
//     }

//     if (pop.vaxstatus[i] != Vaxstat::none) {
//       increment_series(series, SeriesName::now_vaccinated, agegrp, d);
//     }
//   }
// }

// void update_series(const PopData & pop, HistorySeries & series, size_t day) {
//   for (auto agegrp : all_age_buckets){
//     if (day == 1) {

//     } else {
//       series.at(SeriesName::now_infected, agegrp)[day] = series.at(SeriesName::now_infected, agegrp)[day -1] +
//         series.at(SeriesName::new_infected, agegrp)[day] - series.at(SeriesName::new_dead, agegrp)[day] 
//         - series.at(SeriesName::new_recovered, agegrp)[day];
//       series.at(SeriesName::now_unexposed, agegrp)[day] = series.at(SeriesName::now_unexposed, agegrp)[day-1]
//         - series.at(SeriesName::new_infected, agegrp)[day];
//       series.at(SeriesName::now_dead, agegrp)[day] = series.at(SeriesName::now_dead, agegrp)[day-1]
//         + series.at(SeriesName::new_dead, agegrp)[day];
//       // this one is wrong!  we need to know who of yesterday's recovered became infected  
//       series.at(SeriesName::now_recovered, agegrp)[day] = series.at(SeriesName::now_recovered, agegrp)[day-1]
//         + series.at(SeriesName::new_recovered, agegrp)[day];
//     }

//   }
// }

void diff_from_cumulative(std::span<int> src, std::span<int> dest) {
  assert(src.size() == dest.size());
  if (src.size() <= 1) return;  // only the unused 0 slot exists

  // Day series are 1-indexed: day 1 copies through, later days subtract previous from current.
  dest[1] = src[1];
  for (size_t day = 2; day < src.size(); ++day) {
    dest[day] = src[day] - src[day - 1];
  }
}

void finalize_series(HistorySeries& series) {
  for (auto bucket : all_age_buckets) {
    diff_from_cumulative(std::span<int>(series.at(SeriesName::now_infected, bucket)),
                         std::span<int>(series.at(SeriesName::net_infected, bucket)));
  }
}

// debug code currently not called
void write_daily_trace_csv(const std::filesystem::path& output_path,
                           const std::vector<absl::CivilDay>& caldays,
                           const HistorySeries& series) {
  if (caldays.size() != series.day_cnt) {
    throw std::runtime_error("caldays length did not match series.day_cnt");
  }

  ensure_parent_dir(output_path);
  std::ofstream out(output_path);
  if (!out) {
    throw std::runtime_error(fmt::format("Could not open daily trace file '{}'", output_path.string()));
  }

  out << "day,calday,contacts,touched,new_infected,spread_new_infected,recovered,dead,"
         "new_infected_age60_79,recovered_age60_79,dead_age60_79,"
         "new_infected_age80_up,recovered_age80_up,dead_age80_up\n";

  for (size_t day = 1; day <= series.day_cnt; ++day) {
    out << day << ','
        << format_civil_day(caldays[day - 1]) << ','
        << series.at(SeriesName::new_infected, AgeBucket::total)[day] << ','
        << series.at(SeriesName::new_recovered, AgeBucket::total)[day] << ','
        << series.at(SeriesName::new_dead, AgeBucket::total)[day] << ','
        << series.at(SeriesName::new_infected, AgeBucket::age60_79)[day] << ','
        << series.at(SeriesName::new_recovered, AgeBucket::age60_79)[day] << ','
        << series.at(SeriesName::new_dead, AgeBucket::age60_79)[day] << ','
        << series.at(SeriesName::new_infected, AgeBucket::age80_up)[day] << ','
        << series.at(SeriesName::new_recovered, AgeBucket::age80_up)[day] << ','
        << series.at(SeriesName::new_dead, AgeBucket::age80_up)[day] << '\n';
  }
}

void print_total_status_series(const HistorySeries& series, size_t days_per_block) {
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

void print_selected_series(std::vector<SeriesSelection> selections, const HistorySeries& series,
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
