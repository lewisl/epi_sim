#include "series.h"
#include "parameters.h"
#include "population.h"
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
// SeriesColumnMap
// ---------------------------------------------------------------

SeriesColumnMap::SeriesColumnMap(size_t n_status, size_t n_vax,
                                 size_t n_variants, size_t n_rings)
    : n_rings_(n_rings) {
    const size_t ring_stride = size_t(AgeBucket::COUNT);
    const size_t subject_stride = n_rings * ring_stride;
    SeriesColumnIndex next_col = 0;

    auto add_block = [&](SeriesBlock block, size_t subject_count, bool is_stock) {
        const size_t column_count = subject_count * subject_stride;
        descriptors_[size_t(block)] = {
            .base_col = next_col,
            .subject_count = subject_count,
            .subject_stride = subject_stride,
            .ring_stride = ring_stride,
            .column_count = column_count,
            .is_stock = is_stock,
        };
        next_col += column_count;
    };

    add_block(SeriesBlock::now_status, n_status, true);
    add_block(SeriesBlock::new_status, n_status, false);
    add_block(SeriesBlock::now_vax, n_vax, true);
    add_block(SeriesBlock::new_vax, n_vax, false);
    add_block(SeriesBlock::now_variant, n_variants, true);
    add_block(SeriesBlock::new_variant, n_variants, false);
    total_columns_ = next_col;
}

// ---------------------------------------------------------------
// AllSeries
// ---------------------------------------------------------------

AllSeries::AllSeries(size_t day_cnt, const PopData& pop,
                     size_t n_variants, size_t n_vax, size_t n_rings)
    : day_cnt(day_cnt),
      column_map_(Status::names.size(), n_vax, n_variants, n_rings),
      cols_(column_map_.total_columns())
{
    for (auto& col : cols_) col.assign(day_cnt + 1, 0);  // days are 1-indexed
    if (day_cnt == 0) return;
    // Seed day-1 now_status for UNEXPOSED from PopData age bucket counts.
    // First-pass: seed only the RING_ALL aggregate. Per-ring day-1 UNEXPOSED
    // stocks are a follow-up (need ring membership threaded in).
    at(SeriesBlock::now_status, uint8_t(UNEXPOSED), AgeBucket::total)[1] =
        static_cast<SeriesValue>(pop.popn);
    for (size_t i = 0; i < pop.agegrp_parts.size(); ++i) {
        at(SeriesBlock::now_status, uint8_t(UNEXPOSED),
           static_cast<AgeBucket>(i + 1))[1] = pop.agegrp_parts[i];
    }
}

void AllSeries::init_history_series(size_t day) {
    if (day <= 1) return;
    for (SeriesBlock block : all_series_blocks) {
        const auto& desc = column_map_.descriptor(block);
        if (!desc.is_stock) continue;
        const auto end_col = desc.base_col + desc.column_count;
        for (SeriesColumnIndex col = desc.base_col; col < end_col; ++col) {
            cols_[col][day] = cols_[col][day - 1];
        }
    }
}

void AllSeries::finalize_series() {
    // net_infected removed; nothing to finalize
}

void AllSeries::validate_variant_invariant() const {
    const auto& variant_desc = block_descriptor(SeriesBlock::now_variant);
    for (size_t day = 1; day <= day_cnt; ++day) {
        SeriesValue variant_total = 0;
        for (size_t v = 1; v < variant_desc.subject_count; ++v) {
            variant_total += at(SeriesBlock::now_variant, static_cast<uint8_t>(v),
                                AgeBucket::total)[day];
        }
        const SeriesValue infectious =
            at(SeriesBlock::now_status, uint8_t(INFECTIOUS), AgeBucket::total)[day];
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
// Named selector resolution for serialization/printing
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

namespace {

struct SeriesFamilyView {
    std::string_view selector_prefix;
    SeriesBlock now_block;
    SeriesBlock new_block;
    std::span<const std::string> subject_names;
};

struct ResolvedSeriesSource {
    std::string canonical_name;
    std::vector<SeriesColumnIndex> source_columns;
};

std::optional<ResolvedSeriesSource> resolve_series_source(
    const AllSeries& series, std::string_view name, AgeBucket bucket,
    uint8_t ring) {
    bool is_now;
    std::string_view phase;
    std::string_view subject_selector;
    if (name.starts_with("now_")) {
        is_now = true;
        phase = "now";
        subject_selector = name.substr(4);
    } else if (name.starts_with("new_")) {
        is_now = false;
        phase = "new";
        subject_selector = name.substr(4);
    } else {
        return std::nullopt;
    }

    if (ring >= series.n_rings()) return std::nullopt;

    if (subject_selector == "vaccinated") {
        const SeriesBlock block = is_now ? SeriesBlock::now_vax : SeriesBlock::new_vax;
        const auto& desc = series.block_descriptor(block);
        ResolvedSeriesSource source{fmt::format("{}_vaccinated", phase), {}};
        source.source_columns.reserve(desc.subject_count > 0 ? desc.subject_count - 1 : 0);
        for (size_t subject = 1; subject < desc.subject_count; ++subject) {
            source.source_columns.push_back(series.column_index(
                block, static_cast<uint8_t>(subject), bucket, ring));
        }
        return source;
    }

    const std::array families{
        SeriesFamilyView{
            "vax:", SeriesBlock::now_vax, SeriesBlock::new_vax,
            std::span<const std::string>{Vax::names.data(), Vax::names.size()}},
        SeriesFamilyView{
            "variant:", SeriesBlock::now_variant, SeriesBlock::new_variant,
            std::span<const std::string>{Variant::names.data(), Variant::names.size()}},
        SeriesFamilyView{
            "", SeriesBlock::now_status, SeriesBlock::new_status,
            std::span<const std::string>{Status::names.data(), Status::names.size()}},
    };

    for (const auto& family : families) {
        if (!subject_selector.starts_with(family.selector_prefix)) continue;
        const auto subject_name = subject_selector.substr(family.selector_prefix.size());
        const auto it = std::find(family.subject_names.begin(), family.subject_names.end(),
                                  subject_name);
        if (it == family.subject_names.end()) return std::nullopt;

        const size_t subject = static_cast<size_t>(
            std::distance(family.subject_names.begin(), it));
        const SeriesBlock block = is_now ? family.now_block : family.new_block;
        const auto& desc = series.block_descriptor(block);
        if (subject == 0 || subject >= desc.subject_count) return std::nullopt;

        return ResolvedSeriesSource{
            fmt::format("{}_{}{}", phase, family.selector_prefix,
                        family.subject_names[subject]),
            {series.column_index(block, static_cast<uint8_t>(subject), bucket, ring)},
        };
    }

    return std::nullopt;
}

std::string raw_selection_label(const SeriesSelection& selection) {
    return selection.ring.empty()
        ? fmt::format("{}:{}", selection.name, selection.bucket)
        : fmt::format("{}:{}:{}", selection.name, selection.bucket, selection.ring);
}

std::string canonical_selection_label(const ResolvedSeriesSource& source,
                                      AgeBucket bucket, uint8_t ring) {
    if (ring == RING_ALL) {
        return fmt::format("{}:{}", source.canonical_name, to_string(bucket));
    }
    return fmt::format("{}:{}:{}", source.canonical_name, to_string(bucket),
                       Ring::names[ring]);
}

SeriesColumn materialize_series(const AllSeries& series,
                                const ResolvedSeriesSource& source) {
    if (source.source_columns.size() == 1) {
        return series.column(source.source_columns.front());
    }

    SeriesColumn result(series.day_cnt + 1, 0);
    for (const SeriesColumnIndex col : source.source_columns) {
        const auto& values = series.column(col);
        for (size_t day = 0; day <= series.day_cnt; ++day) {
            result[day] += values[day];
        }
    }
    return result;
}

} // namespace

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
    const auto raw_label = raw_selection_label(sel);
    if (!bucket || !ring) {
      resolved.invalid_selections.push_back(raw_label);
      continue;
    }
    auto source = resolve_series_source(series, sel.name, *bucket, *ring);
    if (!source) {
      resolved.invalid_selections.push_back(raw_label);
      continue;
    }
    auto label = canonical_selection_label(*source, *bucket, *ring);
    resolved.cols.push_back({std::move(label), materialize_series(series, *source)});
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
        fmt::print("{:>8}",
                   series.at(SeriesBlock::now_status, s, AgeBucket::total)[day]);
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
                           std::filesystem::path output_path) {
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

  ensure_parent_dir(output_path);

  std::ofstream out(output_path);
  if (!out) {
    throw std::runtime_error(
        fmt::format("Could not write series CSV to '{}'", output_path.string()));
  }

  // Write header
  vector<string> headers;
  headers.reserve(cols.size());
  for (const auto& col : cols) headers.push_back(col.label);
  fmt::println(out, "{}", fmt::join(headers, ","));

  // Write rows (1-indexed days)
  vector<SeriesValue> row;
  row.reserve(cols.size());
  for (size_t i = 1; i <= series.day_cnt; ++i) {
    for (const auto& col : cols) {
      row.push_back(col.data[i]);
    }
    fmt::println(out, "{}", fmt::join(row, ","));
    row.clear();
  }

  fmt::println("Wrote selected series CSV to '{}'", output_path.string());
}

void serialize_selected_series(SeriesColSpec spec, const AllSeries & series,
                           string base_fname, vector<string> path_steps) {
  const char* home = std::getenv("HOME");
  if (!home) throw std::runtime_error("HOME not set");
  std::filesystem::path fpath{home};
  if (path_steps.empty()) path_steps = {"code", "epi_sim", "series_output"};
  for (auto step : path_steps) fpath /= step;
  fpath /= make_timestamped_filename(base_fname) + ".csv";

  serialize_selected_series(std::move(spec), series, fpath);
}
