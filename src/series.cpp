#include "series.h"
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"
#include "sim.h"

namespace {
void ensure_parent_dir(const std::filesystem::path& output_path) {
  auto parent = output_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }
}
} // namespace

AgeBucket bucket_from_age(Agegrp agegrp) {
  switch (agegrp.v) {
    case AGE0_19.v:  return AgeBucket::age0_19;
    case AGE20_39.v: return AgeBucket::age20_39;
    case AGE40_59.v: return AgeBucket::age40_59;
    case AGE60_79.v: return AgeBucket::age60_79;
    case AGE80_UP.v: return AgeBucket::age80_up;
    default:
      throw std::runtime_error("Unknown age group for series bucket mapping");
  }
}

// ---------------------------------------------------------------
// SeriesGroup
// ---------------------------------------------------------------

SeriesGroup::SeriesGroup(size_t n_subjects, size_t day_cnt)
    : subjects(n_subjects), day_cnt(day_cnt) {
    for (auto& bucket_arr : subjects) {
        for (auto& v : bucket_arr) v.assign(day_cnt + 1, 0);  // days are 1-indexed
    }
}

void SeriesGroup::update(uint8_t subject_idx, Agegrp agegrp, size_t day, int change) {
    auto bucket = size_t(bucket_from_age(agegrp));
    subjects[subject_idx][bucket][day] += change;
    subjects[subject_idx][size_t(AgeBucket::total)][day] += change;
}

// ---------------------------------------------------------------
// AllSeries
// ---------------------------------------------------------------

AllSeries::AllSeries(size_t day_cnt, const PopData& pop, size_t n_variants, size_t n_vax)
    : now_status(Status::names.size(), day_cnt),
      new_status(Status::names.size(), day_cnt),
      now_vax(n_vax, day_cnt),
      new_vax(n_vax, day_cnt),
      now_variant(n_variants, day_cnt),
      new_variant(n_variants, day_cnt),
      day_cnt(day_cnt)
{
    if (day_cnt == 0) return;
    // Seed day-1 now_status for UNEXPOSED from PopData age bucket counts
    now_status.subjects[uint8_t(UNEXPOSED)][size_t(AgeBucket::total)][1] = static_cast<int>(pop.popn);
    for (size_t i = 0; i < pop.agegrp_parts.size(); ++i) {
        now_status.subjects[uint8_t(UNEXPOSED)][i + 1][1] = pop.agegrp_parts[i];
    }
}

void AllSeries::init_history_series(size_t day) {
    if (day <= 1) return;
    // Carry now_status stocks forward from the previous day
    for (size_t s = 0; s < now_status.subjects.size(); ++s) {
        for (auto bucket : all_age_buckets) {
            now_status.at(static_cast<uint8_t>(s), bucket)[day] =
                now_status.at(static_cast<uint8_t>(s), bucket)[day-1];
        }
    }
    // Carry now_vax stocks forward
    for (size_t v = 0; v < now_vax.subjects.size(); ++v) {
        for (auto bucket : all_age_buckets) {
            now_vax.at(static_cast<uint8_t>(v), bucket)[day] =
                now_vax.at(static_cast<uint8_t>(v), bucket)[day-1];
        }
    }
    // Carry now_variant stocks forward
    for (size_t v = 0; v < now_variant.subjects.size(); ++v) {
        for (auto bucket : all_age_buckets) {
            now_variant.at(static_cast<uint8_t>(v), bucket)[day] =
                now_variant.at(static_cast<uint8_t>(v), bucket)[day-1];
        }
    }
}

void AllSeries::finalize_series() {
    // net_infected removed; nothing to finalize
}

void AllSeries::validate_variant_invariant() const {
    for (size_t day = 1; day <= day_cnt; ++day) {
        int variant_total = 0;
        for (size_t v = 1; v < now_variant.subjects.size(); ++v)
            variant_total += now_variant.at(static_cast<uint8_t>(v), AgeBucket::total)[day];
        int infectious = now_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[day];
        if (variant_total != infectious)
            throw std::runtime_error(fmt::format(
                "Variant invariant failed on day {}: sum(now_variant)={} != now_infectious={}",
                day, variant_total, infectious));
    }
    fmt::println("Variant invariant OK: sum(now_variant) == now_infectious for all {} days.", day_cnt);
}

// ---------------------------------------------------------------
// SeriesColSpec constructors and build_for_buckets
// ---------------------------------------------------------------

void SeriesColSpec::validate_sentinel(const char* s) {
    if (std::string_view(s) != "all")
        throw std::invalid_argument("only valid sentinel value is \"all\"");
}

SeriesColSpec::SeriesColSpec(const char* sentinel) {
    validate_sentinel(sentinel);
    std::vector<std::string> all_buckets;
    all_buckets.reserve(all_age_buckets.size());
    for (auto b : all_age_buckets) all_buckets.emplace_back(to_string(b));
    selections = build_for_buckets(all_buckets);
}

SeriesColSpec::SeriesColSpec(const char* sentinel, const char* bucket) {
    validate_sentinel(sentinel);
    selections = build_for_buckets({std::string(bucket)});
}

SeriesColSpec::SeriesColSpec(const char* sentinel, std::vector<std::string> buckets) {
    validate_sentinel(sentinel);
    selections = build_for_buckets(std::move(buckets));
}

std::vector<SeriesSelection> SeriesColSpec::build_for_buckets(
        const std::vector<std::string>& buckets) {
    std::vector<SeriesSelection> out;
    for (const auto& bkt : buckets) {
        // Status: skip index 0 ("none")
        for (size_t i = 1; i < Status::names.size(); ++i) {
            out.push_back({"now_" + Status::names[i], bkt});
            out.push_back({"new_" + Status::names[i], bkt});
        }
        // Vaccinated aggregate (sums all brands)
        out.push_back({"now_vaccinated", bkt});
        out.push_back({"new_vaccinated", bkt});
        // Per-brand vax: skip index 0 ("none")
        for (size_t i = 1; i < Vax::names.size(); ++i) {
            out.push_back({"now_vax:" + Vax::names[i], bkt});
            out.push_back({"new_vax:" + Vax::names[i], bkt});
        }
        // Per-variant: skip index 0 ("none")
        for (size_t i = 1; i < Variant::names.size(); ++i) {
            out.push_back({"now_variant:" + Variant::names[i], bkt});
            out.push_back({"new_variant:" + Variant::names[i], bkt});
        }
    }
    return out;
}

// ---------------------------------------------------------------
// resolve_series: named series lookup for serialization/printing
// ---------------------------------------------------------------

std::optional<vector<int>> resolve_series(const AllSeries& series,
                                          std::string_view name, AgeBucket bucket) {
    for (size_t i = 1; i < Status::names.size(); ++i) {
        if (name == "now_" + Status::names[i])
            return series.now_status.at(static_cast<uint8_t>(i), bucket);
        if (name == "new_" + Status::names[i])
            return series.new_status.at(static_cast<uint8_t>(i), bucket);
    }
    if (name == "now_vaccinated" || name == "new_vaccinated") {
        const SeriesGroup& grp = (name == "now_vaccinated") ? series.now_vax : series.new_vax;
        vector<int> result(series.day_cnt + 1, 0);
        for (size_t i = 1; i < grp.subjects.size(); ++i) {  // skip "none" at index 0
            const auto& v = grp.at(static_cast<uint8_t>(i), bucket);
            for (size_t d = 0; d <= series.day_cnt; ++d) result[d] += v[d];
        }
        return result;
    }
    // Named lookup: "now_variant:delta", "new_variant:alpha", "now_vax:Pfizer", etc.
    if (auto colon = name.find(':'); colon != std::string_view::npos) {
        auto group        = name.substr(0, colon);
        auto subject_name = std::string(name.substr(colon + 1));

        if (group == "now_variant" || group == "new_variant") {
            const SeriesGroup& grp = (group == "now_variant") ? series.now_variant : series.new_variant;
            auto it = std::find(Variant::names.begin(), Variant::names.end(), subject_name);
            if (it == Variant::names.end()) return std::nullopt;
            auto idx = static_cast<uint8_t>(std::distance(Variant::names.begin(), it));
            return grp.at(idx, bucket);
        }
        if (group == "now_vax" || group == "new_vax") {
            const SeriesGroup& grp = (group == "now_vax") ? series.now_vax : series.new_vax;
            auto it = std::find(Vax::names.begin(), Vax::names.end(), subject_name);
            if (it == Vax::names.end()) return std::nullopt;
            auto idx = static_cast<uint8_t>(std::distance(Vax::names.begin(), it));
            return grp.at(idx, bucket);
        }
    }

    return std::nullopt;
}

// ---------------------------------------------------------------
// Print functions
// ---------------------------------------------------------------

void print_total_status_series(const AllSeries& series, size_t days_per_block) {
  if (series.day_cnt == 0) {
    fmt::println("\nNo day series to print.");
    return;
  }

  constexpr auto labels   = std::array{"infected", "unexposed", "recovered", "dead"};
  constexpr auto statuses = std::array<uint8_t, 4>{
      uint8_t(INFECTIOUS), uint8_t(UNEXPOSED), uint8_t(RECOVERED), uint8_t(DEAD)};

  fmt::println("\nTotal status series");
  for (size_t block_start = 1; block_start <= series.day_cnt; block_start += days_per_block) {
    const size_t block_end = std::min(block_start + days_per_block - 1, series.day_cnt);
    fmt::println("\nDays {}-{}", block_start, block_end);

    fmt::print("{:<12}", "day");
    for (size_t day = block_start; day <= block_end; ++day) {
      fmt::print("{:>8}", day);
    }
    fmt::println("");

    for (auto [label, s] : std::views::zip(labels, statuses)) {
      fmt::print("{:<12}", label);
      for (size_t day = block_start; day <= block_end; ++day) {
        fmt::print("{:>8}", series.now_status.at(s, AgeBucket::total)[day]);
      }
      fmt::println("");
    }
  }
}

void print_selected_series(SeriesColSpec spec, const AllSeries& series,
                           size_t days_per_block) {
  auto& selections = spec.selections;
  if (series.day_cnt == 0) {
    fmt::println("\nNo day series to print.");
    return;
  }
  if (selections.empty()) {
    fmt::println("\nNo series selected.");
    return;
  }

  struct ResolvedCol { string label; vector<int> data; };
  vector<ResolvedCol> cols;
  cols.reserve(selections.size());
  vector<string> invalid_selections;

  for (const auto& [name_text, bucket_text] : selections) {
    auto bucket = age_bucket_from_string(bucket_text);
    if (!bucket) { invalid_selections.push_back(fmt::format("{}:{}", name_text, bucket_text)); continue; }
    auto data = resolve_series(series, name_text, *bucket);
    if (!data)   { invalid_selections.push_back(fmt::format("{}:{}", name_text, bucket_text)); continue; }
    cols.push_back({fmt::format("{}:{}", name_text, bucket_text), std::move(*data)});
  }

  if (!invalid_selections.empty()) {
    fmt::println("\nUnknown series selections: {}", invalid_selections);
    return;
  }
  if (cols.empty()) {
    fmt::println("\nNo valid series selected.");
    return;
  }

  fmt::println("\nSelected series:");
  for (size_t block_start = 1; block_start <= series.day_cnt; block_start += days_per_block) {
    const size_t block_end = std::min(block_start + days_per_block - 1, series.day_cnt);
    fmt::println("\nDays {}-{}", block_start, block_end);

    fmt::print("{:<12}", "day");
    for (size_t day = block_start; day <= block_end; ++day) {
      fmt::print("{:>8}", day);
    }
    fmt::println("");

    for (const auto& col : cols) {
      fmt::print("{:<12}", col.label);
      for (size_t day = block_start; day <= block_end; ++day) {
        fmt::print("{:>8}", col.data[day]);
      }
      fmt::println("");
    }
  }
}

// ---------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------

void serialize_selected_series(SeriesColSpec spec, const AllSeries& series,
                           string base_fname, vector<string> path_steps) {
  auto& selections = spec.selections;
  if (series.day_cnt == 0) {
    fmt::println("\nNo day series to output.");
    return;
  }
  if (selections.empty()) {
    fmt::println("\nNo series selected.");
    return;
  }

  struct ResolvedCol { string label; vector<int> data; };
  vector<ResolvedCol> cols;
  cols.reserve(selections.size());
  vector<string> invalid_selections;

  for (const auto& [name_text, bucket_text] : selections) {
    auto bucket = age_bucket_from_string(bucket_text);
    if (!bucket) { invalid_selections.push_back(fmt::format("{}:{}", name_text, bucket_text)); continue; }
    auto data = resolve_series(series, name_text, *bucket);
    if (!data)   { invalid_selections.push_back(fmt::format("{}:{}", name_text, bucket_text)); continue; }
    cols.push_back({fmt::format("{}:{}", name_text, bucket_text), std::move(*data)});
  }

  if (!invalid_selections.empty()) {
    fmt::println("\nSkipping unknown series selections: {}", invalid_selections);
  }
  if (cols.empty()) {
    fmt::println("\nNo valid series selected.");
    return;
  }

  // Build output path
  const char* home = std::getenv("HOME");
  if (!home) throw std::runtime_error("HOME not set");
  std::filesystem::path fpath{home};
  if (path_steps.empty()) path_steps = {"code", "epi_sim", "series_output"};
  for (auto step : path_steps) fpath /= step;
  fpath /= make_timestamped_filename(base_fname) + ".csv";
  ensure_parent_dir(fpath);

  std::ofstream out(fpath);
  if (!out) {
    throw std::runtime_error(
        fmt::format("Could not write series CSV to '{}'", fpath.string()));
  }

  // Write header
  vector<string> headers;
  headers.reserve(cols.size());
  for (const auto& col : cols) headers.push_back(col.label);
  fmt::println(out, "{}", fmt::join(headers, ","));

  // Write rows (1-indexed days)
  vector<int> row;
  row.reserve(cols.size());
  for (size_t i = 1; i <= series.day_cnt; ++i) {
    for (const auto& col : cols) {
      row.push_back(col.data[i]);
    }
    fmt::println(out, "{}", fmt::join(row, ","));
    row.clear();
  }

  fmt::println("Wrote selected series CSV to '{}'", fpath.string());
}
