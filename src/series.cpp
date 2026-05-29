#include "series.h"
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"
#include "sim.h"
#include <charconv>

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

SeriesGroup::SeriesGroup(size_t n_subjects, size_t n_rings, size_t day_cnt)
    : subjects(n_subjects), day_cnt(day_cnt), n_rings(n_rings) {
    for (auto& ring_arr : subjects) {
        ring_arr.resize(n_rings);
        for (auto& bucket_arr : ring_arr) {
            for (auto& v : bucket_arr) v.assign(day_cnt + 1, 0);  // days are 1-indexed
        }
    }
}

void SeriesGroup::update(uint8_t subject_idx, uint8_t ring, Agegrp agegrp,
                         size_t day, int change) {
    auto bucket = size_t(bucket_from_age(agegrp));
    auto& subj  = subjects[subject_idx];
    subj[ring][bucket][day]                       += change;
    subj[ring][size_t(AgeBucket::total)][day]     += change;
    if (ring != RING_ALL) {
        subj[RING_ALL][bucket][day]                   += change;
        subj[RING_ALL][size_t(AgeBucket::total)][day] += change;
    }
}

// ---------------------------------------------------------------
// AllSeries
// ---------------------------------------------------------------

AllSeries::AllSeries(size_t day_cnt, const PopData& pop,
                     size_t n_variants, size_t n_vax, size_t n_rings)
    : now_status(Status::names.size(), n_rings, day_cnt),
      new_status(Status::names.size(), n_rings, day_cnt),
      now_vax(n_vax, n_rings, day_cnt),
      new_vax(n_vax, n_rings, day_cnt),
      now_variant(n_variants, n_rings, day_cnt),
      new_variant(n_variants, n_rings, day_cnt),
      day_cnt(day_cnt)
{
    if (day_cnt == 0) return;
    // Seed day-1 now_status for UNEXPOSED from PopData age bucket counts.
    // First-pass: seed only the RING_ALL aggregate. Per-ring day-1 UNEXPOSED
    // stocks are a follow-up (need ring membership threaded in).
    now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[1] = static_cast<int>(pop.popn);
    for (size_t i = 0; i < pop.agegrp_parts.size(); ++i) {
        now_status.at(uint8_t(UNEXPOSED), static_cast<AgeBucket>(i + 1))[1] = pop.agegrp_parts[i];
    }
}

void AllSeries::init_history_series(size_t day) {
    if (day <= 1) return;
    auto carry = [day](SeriesGroup& grp) {
        for (size_t s = 0; s < grp.subjects.size(); ++s) {
            for (uint8_t r = 0; r < grp.n_rings; ++r) {
                for (auto bucket : all_age_buckets) {
                    grp.at(static_cast<uint8_t>(s), bucket, r)[day] =
                        grp.at(static_cast<uint8_t>(s), bucket, r)[day-1];
                }
            }
        }
    };
    carry(now_status);
    carry(now_vax);
    carry(now_variant);
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

// "" → RING_ALL. A decimal token is taken as a literal ring id; otherwise
// looked up by name in Ring::names. nullopt if unknown.
std::optional<uint8_t> ring_id_from_token(const std::string& tok) {
    if (tok.empty()) return RING_ALL;
    if (std::all_of(tok.begin(), tok.end(), ::isdigit)) {
        auto v = std::stoul(tok);
        if (v == 0 || v < Ring::names.size()) return static_cast<uint8_t>(v);
        return std::nullopt;
    }
    auto it = std::find(Ring::names.begin(), Ring::names.end(), tok);
    if (it == Ring::names.end()) return std::nullopt;
    return static_cast<uint8_t>(std::distance(Ring::names.begin(), it));
}

std::optional<vector<int>> resolve_series(const AllSeries& series,
                                          std::string_view name, AgeBucket bucket,
                                          uint8_t ring) {
    for (size_t i = 1; i < Status::names.size(); ++i) {
        if (name == "now_" + Status::names[i])
            return series.now_status.at(static_cast<uint8_t>(i), bucket, ring);
        if (name == "new_" + Status::names[i])
            return series.new_status.at(static_cast<uint8_t>(i), bucket, ring);
    }
    if (name == "now_vaccinated" || name == "new_vaccinated") {
        const SeriesGroup& grp = (name == "now_vaccinated") ? series.now_vax : series.new_vax;
        vector<int> result(series.day_cnt + 1, 0);
        for (size_t i = 1; i < grp.subjects.size(); ++i) {  // skip "none" at index 0
            const auto& v = grp.at(static_cast<uint8_t>(i), bucket, ring);
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
            return grp.at(idx, bucket, ring);
        }
        if (group == "now_vax" || group == "new_vax") {
            const SeriesGroup& grp = (group == "now_vax") ? series.now_vax : series.new_vax;
            auto it = std::find(Vax::names.begin(), Vax::names.end(), subject_name);
            if (it == Vax::names.end()) return std::nullopt;
            auto idx = static_cast<uint8_t>(std::distance(Vax::names.begin(), it));
            return grp.at(idx, bucket, ring);
        }
    }

    return std::nullopt;
}

std::optional<RingNameParse> parse_ring_suffix(std::string_view name) {
    constexpr std::string_view tag = "@ring:";
    auto pos = name.find(tag);
    if (pos == std::string_view::npos) {
        return RingNameParse{std::string(name), RING_ALL};
    }
    auto base   = name.substr(0, pos);
    auto suffix = name.substr(pos + tag.size());
    if (suffix.empty()) return std::nullopt;

    // try name lookup first
    auto it = std::find(Ring::names.begin(), Ring::names.end(), std::string(suffix));
    if (it != Ring::names.end()) {
        auto idx = static_cast<uint8_t>(std::distance(Ring::names.begin(), it));
        return RingNameParse{std::string(base), idx};
    }
    // fall back to decimal index
    int parsed = 0;
    auto first = suffix.data();
    auto last  = suffix.data() + suffix.size();
    auto [ptr, ec] = std::from_chars(first, last, parsed);
    if (ec != std::errc{} || ptr != last || parsed < 0 || parsed > 255) {
        return std::nullopt;
    }
    return RingNameParse{std::string(base), static_cast<uint8_t>(parsed)};
}

ResolvedSeriesSelection resolve_selected_series(const SeriesColSpec& spec,
                                                const AllSeries& series) {
  ResolvedSeriesSelection resolved;
  auto& selections = spec.selections;
  resolved.cols.reserve(selections.size());

  for (const auto& sel : selections) {
    auto bucket = age_bucket_from_string(sel.bucket);
    auto ring   = ring_id_from_token(sel.ring);
    auto label  = sel.ring.empty()
                      ? fmt::format("{}:{}", sel.name, sel.bucket)
                      : fmt::format("{}:{}:{}", sel.name, sel.bucket, sel.ring);
    if (!bucket || !ring) {
      resolved.invalid_selections.push_back(label);
      continue;
    }
    auto data = resolve_series(series, sel.name, *bucket, *ring);
    if (!data) {
      resolved.invalid_selections.push_back(label);
      continue;
    }
    resolved.cols.push_back({label, std::move(*data)});
  }

  return resolved;
}

// ---------------------------------------------------------------
// Print functions
// ---------------------------------------------------------------

void print_total_status_series(const AllSeries & series, size_t days_per_block) {
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

  auto resolved = resolve_selected_series(spec, series);
  const auto& cols = resolved.cols;

  if (!resolved.invalid_selections.empty()) {
    fmt::println("\nUnknown series selections: {}", resolved.invalid_selections);
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

void serialize_selected_series(SeriesColSpec spec, const AllSeries & series,
                           string base_fname, vector<string> path_steps) {
  auto & selections = spec.selections;
  if (series.day_cnt == 0) {
    fmt::println("\nNo day series to output.");
    return;
  }
  if (selections.empty()) {
    fmt::println("\nNo series selected.");
    return;
  }

  auto resolved = resolve_selected_series(spec, series);
  const auto& cols = resolved.cols;

  if (!resolved.invalid_selections.empty()) {
    fmt::println("\nSkipping unknown series selections: {}", resolved.invalid_selections);
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
