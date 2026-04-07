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

void ensure_parent_dir(const std::filesystem::path& output_path) {
  auto parent = output_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }
}
}

AgeBucket bucket_from_age(Agegrp agegrp) {
  switch (agegrp.v) {
    case AGE0_19.v:
      return AgeBucket::age0_19;
    case AGE20_39.v:
      return AgeBucket::age20_39;
    case AGE40_59.v:
      return AgeBucket::age40_59;
    case AGE60_79.v:
      return AgeBucket::age60_79;
    case AGE80_UP.v:
      return AgeBucket::age80_up;
    default:
      throw std::runtime_error("Unknown age group for series bucket mapping");
  }
}

void HistorySeries::init_history_series(size_t day) {
    if (day <= 1) return;
    for (auto bucket : all_age_buckets) {
        at(SeriesName::now_infected,  bucket)[day] = at(SeriesName::now_infected,  bucket)[day-1];
        at(SeriesName::now_unexposed, bucket)[day] = at(SeriesName::now_unexposed, bucket)[day-1];
        at(SeriesName::now_recovered, bucket)[day] = at(SeriesName::now_recovered, bucket)[day-1];
        at(SeriesName::now_dead,      bucket)[day] = at(SeriesName::now_dead,      bucket)[day-1];
        at(SeriesName::now_vaccinated,bucket)[day] = at(SeriesName::now_vaccinated,bucket)[day-1];
    }
}


void HistorySeries::delta_series(SeriesName name, Agegrp agegrp, size_t day, int change) {
  at(name, AgeBucket::total)[day] += change;
  at(name, bucket_from_age(agegrp))[day] += change;
}


void diff_from_cumulative(std::span<int> src, std::span<int> dest) {
  assert(src.size() == dest.size());
  if (src.size() <= 1) return;  // only the unused 0 slot exists

  // Day series are 1-indexed: day 1 copies through, later days subtract previous from current.
  dest[1] = src[1];
  for (size_t day = 2; day < src.size(); ++day) {
    dest[day] = src[day] - src[day - 1];
  }
}

void HistorySeries::finalize_series() {
  for (auto bucket : all_age_buckets) {
    diff_from_cumulative(std::span<int>(at(SeriesName::now_infected, bucket)),
                         std::span<int>(at(SeriesName::net_infected, bucket)));
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


void serialize_selected_series(std::vector<SeriesSelection> selections, const HistorySeries& series,
                           string base_fname, vector<string> path_steps) {
  if (series.day_cnt == 0) {
    fmt::println("\nNo day series to output.");
    return;
  }

  if (selections.empty()) {
    fmt::println("\nNo series selected.");
    return;
  }

  // setup the columns to be written to file with labels for the header row
  vector<string> invalid_selections;
  vector<std::pair<SeriesName, AgeBucket>> cols;
  cols.reserve(selections.size());
  for (const auto& [name_text, bucket_text] : selections) {
    auto name = series_name_from_string(name_text);
    auto bucket = age_bucket_from_string(bucket_text);
    if (!name || !bucket) {
      invalid_selections.push_back(fmt::format("{}:{}", name_text, bucket_text));
      continue;
    }
    cols.emplace_back(*name, *bucket);
  }

  if (!invalid_selections.empty()) {
    fmt::println("\nSkipping unknown series selections: {}", invalid_selections);
  }

  if (cols.empty()) {
    fmt::println("\nNo valid series selected.");
    return;
  }

  vector<string> selected_labels;
  selected_labels.reserve(cols.size());
  for (const auto& [name, bucket] : cols) {
    selected_labels.push_back(fmt::format("{}:{}", to_string(name), to_string(bucket)));
  }

  // create and test file name
      const char* home = std::getenv("HOME");
      if (!home) throw std::runtime_error("HOME not set");
      std::filesystem::path fpath{home};  // fs::path accepts const char* directly
      if (path_steps.empty()) path_steps = {"code", "epi_sim", "series_output"};  // TODO: this doesn't make sense on anyone else's machine
      for (auto step : path_steps) fpath /= step;
      fpath /= make_timestamped_filename(base_fname) + ".csv";
      ensure_parent_dir(fpath);  // create dir if it doesn't exist

      std::ofstream out(fpath);
      if (!out) {
        throw std::runtime_error(
            fmt::format("Could not write rendered plot to '{}'", fpath.string()));
      }

  // write rows to csv file
  fmt::println(out, "{}", fmt::join(selected_labels, ","));

  std::vector<int> row;
  row.reserve(cols.size()); // allocate needed memory once
  for (auto i = 1; i <= series.day_cnt; ++i) {
    for (auto [name, bucket] : cols) {
      row.push_back(series.at(name, bucket)[i]);  // no allocations here
    }
    fmt::println(out, "{}", fmt::join(row, ","));
    row.clear();
  }

  fmt::println("Wrote selected series CSV to '{}'", fpath.string());
}
