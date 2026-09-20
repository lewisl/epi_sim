#include "series.h"
#include "parameters.h"
#include "population.h"
#include <charconv>
#include <stdexcept>
#include <magic_enum/magic_enum.hpp>

namespace {
void ensure_parent_dir(const std::filesystem::path& output_path) {
  auto parent = output_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }
}
} // namespace

// ---------------------------------------------------------------
// Histories
// ---------------------------------------------------------------

// constructor for Histories collection of history vectors
Histories::Histories(size_t n_days, const PopData& pop,
                     size_t real_variant_count, size_t real_vax_count,
                     size_t real_ring_count)
    : day_cnt(n_days),
      real_variant_count_(real_variant_count),
      real_vax_count_(real_vax_count),
      real_ring_count_(real_ring_count),
      ring_lane_count_(std::max<size_t>(real_ring_count, 1)) {
    if (real_variant_count > 255 || real_vax_count > 255
        || real_ring_count > 255) {
        throw std::invalid_argument(
            "History trait and ring counts must fit in uint8_t raw values @ <= 255");
    }
    const std::array trait_value_counts{
        Status::names.size() - 1, real_vax_count_, real_variant_count_};
    for (const Trait trait : all_traits) {
        phase_widths_[size_t(trait)] =
            trait_value_counts[size_t(trait)]
          * ring_lane_count_ * HISTORY_AGE_COUNT;
    }

    // create and initialize outer vector container and inner vectors for histories accumulated during simulation
    // this instantiates only base history vectors updated during simulaton
    const size_t sim_history_count =
        magic_enum::enum_count<Phase>() * std::accumulate(phase_widths_.begin(), phase_widths_.end(), size_t(0));
    history_vectors_.assign(        
        sim_history_count,
        std::vector<HistoryValue>(n_days + 1, HistoryValue(0)));

    if (n_days == 0) return;

    // All people begin as now_unexposed. new_unexposed has physical columns
    // only to preserve the uniform phase width; simulation code never writes it.
    for (size_t person = 1; person <= pop.popn; ++person) {
        const Agegrp age = pop.agegrp[person];
        const uint8_t ring = real_ring_count_ == 0
                           ? RING_ALL
                           : uint8_t(pop.ring[person]);
        if (age.v == 0 || age.v > HISTORY_AGE_COUNT
            || (real_ring_count_ != 0
                && (ring == 0 || ring > real_ring_count_))) {
            throw std::runtime_error(fmt::format(
                "Cannot seed history for person {}: age={} ring={}",
                person, age.v, ring));
        }
        update(Trait::status, Phase::now, uint8_t(UNEXPOSED),
               ring, age, 1, 1);
    }
}

void Histories::init_history_series(size_t day) {
    if (day <= 1) return;
    for (const Trait trait : all_traits) {
        const auto begin = trait_phase_base(trait, Phase::now);
        const auto end = begin + phase_width(trait);
        for (size_t history_idx = begin; history_idx < end; ++history_idx) {
            history_vectors_[history_idx][day] = history_vectors_[history_idx][day - 1];
        }
    }
}

size_t Histories::trait_value_count(Trait trait) const {
    switch (trait) {
        case Trait::status: 
          return Status::names.size() - 1;
        case Trait::vax: 
          return real_vax_count_;
        case Trait::variant: 
          return real_variant_count_;
    }
    throw std::runtime_error("Invalid trait value for histories.");

}

bool Histories::valid_history_coordinates(
    Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
    uint8_t ring) const {
    if (size_t(trait) >= magic_enum::enum_count<Trait>()
        || size_t(phase) >= magic_enum::enum_count<Phase>()
        || trait_value == 0
        || size_t(trait_value) > trait_value_count(trait)
        || age.v == 0 || age.v > HISTORY_AGE_COUNT) {
        return false;
    }
    return real_ring_count_ == 0
         ? ring == RING_ALL
         : ring >= 1 && size_t(ring) <= real_ring_count_;
}

//
// introspection of Histories object and specific history vectors
//

// change output of this to HistorySelector
std::optional<HistorySelector> Histories::describe_history_vector(
    size_t history_idx) const {
    if (history_idx >= history_vectors_.size()) return std::nullopt;

    for (const Trait trait : all_traits) {
        const size_t value_stride = ring_lane_count_ * HISTORY_AGE_COUNT;
        for (const Phase phase : all_phases) {
            const auto base = trait_phase_base(trait, phase);
            const auto width = phase_width(trait);
            if (history_idx < base || history_idx >= base + width) continue;

            const size_t within_group = history_idx - base;
            const size_t value_ordinal = within_group / value_stride;
            const size_t within_value = within_group % value_stride;
            const size_t ring_ordinal = within_value / HISTORY_AGE_COUNT;
            const size_t age_ordinal = within_value % HISTORY_AGE_COUNT;
            return HistorySelector{
                .phase = std::string{magic_enum::enum_name(phase)},
                .trait_value = std::string{
                    trait == Trait::status ? Status::names[value_ordinal + 1] :
                    trait == Trait::vax ? Vax::names[value_ordinal + 1] :
                                         Variant::names[value_ordinal + 1]},
                .age = std::string{Agegrp::names[age_ordinal + 1]},
                .trait = std::string{magic_enum::enum_name(trait)},
                .ring = real_ring_count_ == 0
                      ? ""
                      : Ring::names[static_cast<size_t>(ring_ordinal+1)],
            };
        }
    }
    return std::nullopt;
}

std::string Histories::explain_history_vector_idx(
    Trait trait, Phase phase, uint8_t trait_value, Agegrp age,
    uint8_t ring) const {
    if (!valid_history_coordinates(trait, phase, trait_value, age, ring)) {
        return fmt::format(
            "invalid history coordinates: trait={} phase={} raw_value={} raw_ring={} raw_age={}",
            size_t(trait), size_t(phase), trait_value, ring, age.v);
    }
    const auto history_idx = history_vector_idx(
        trait, phase, trait_value, age, ring);
    return fmt::format(
        "{} -> column={} (value_ordinal={}, ring_ordinal={}, age_ordinal={})",
        history_column_label(history_idx), history_idx, size_t(trait_value - 1),
        real_ring_count_ == 0 ? 0 : size_t(ring - 1), size_t(age.v - 1));
}

void Histories::dump_history_layout(std::ostream& out) const {
    for (size_t history_idx = 0; history_idx < history_vectors_.size(); ++history_idx) {
        fmt::println(out, "{}: {}", history_idx, history_column_label(history_idx));
    }
}

void Histories::validate_history_indexing() const {
    std::vector<bool> seen(history_vectors_.size(), false);
    size_t visited = 0;
    for (const Trait trait : all_traits) {
        for (const Phase phase : all_phases) {
            for (size_t value = 1; value <= trait_value_count(trait); ++value) {
                for (size_t ring_ordinal = 0; ring_ordinal < ring_lane_count_; ++ring_ordinal) {
                    const uint8_t ring = real_ring_count_ == 0
                        ? RING_ALL
                        : static_cast<uint8_t>(ring_ordinal + 1);
                    for (size_t age = 1; age <= HISTORY_AGE_COUNT; ++age) {
                        const auto history_idx = history_vector_idx(
                            trait, phase, static_cast<uint8_t>(value),
                            Agegrp{static_cast<uint8_t>(age)}, ring);
                        if (history_idx >= seen.size() || seen[history_idx]) {
                            throw std::logic_error(fmt::format(
                                "Invalid history layout at column {}", history_idx));
                        }
                        seen[history_idx] = true;
                        ++visited;
                        const auto reverse = describe_history_vector(history_idx);
                        const HistorySelector expected{
                            std::string{magic_enum::enum_name(phase)},
                            std::string{trait == Trait::status ? Status::names[value] :
                                        trait == Trait::vax ? Vax::names[value] :
                                                             Variant::names[value]},
                            std::string{Agegrp::names[age]},
                            std::string{magic_enum::enum_name(trait)},
                            real_ring_count_ == 0 ? "" : Ring::names[static_cast<size_t>(ring)]};
                        if (!reverse || *reverse != expected) {
                            throw std::logic_error(fmt::format(
                                "History layout reverse lookup failed at column {}",
                                history_idx));
                        }
                    }
                }
            }
        }
    }
    if (visited != history_vectors_.size()
        || std::ranges::find(seen, false) != seen.end()) {
        throw std::logic_error("History layout does not cover every column");
    }
}


const std::vector<HistorySelector> enumerate_history_selections(const Histories& histories) {
    vector<HistorySelector> vh;
    for (auto i = 0; i < histories.sim_history_count(); ++i) {
      auto desc = histories.describe_history_vector(i);
      if (desc)
        vh.push_back(*desc);
      else 
        throw std::runtime_error(fmt::format("history description failed at {}", i));
    }
    return vh;
  }


std::string Histories::history_column_label(size_t history_idx) const {
    const auto selection = describe_history_vector(history_idx);
    if (!selection) return fmt::format("invalid_history_column={}", history_idx);
    const auto& c = *selection;
    return fmt::format(
        "trait={}|phase={}|value={}|ring={}|age={}",
        c.trait, c.phase, c.trait_value, c.ring=="" ? "population" : c.ring,
        c.age);
}



// calculate total at one history element position
//   as the total across agegrps and rings
HistoryValue Histories::aggregate_value(
    Trait trait, Phase phase, uint8_t trait_value, size_t day) const {
    HistoryValue result = 0;
    for (size_t ring_ordinal = 0;
         ring_ordinal < ring_lane_count_; ++ring_ordinal) {
        const uint8_t ring = real_ring_count_ == 0
            ? RING_ALL
            : static_cast<uint8_t>(ring_ordinal + 1);
        for (size_t age = 1; age <= HISTORY_AGE_COUNT; ++age) {
            result += at(trait, phase, trait_value,
                         Agegrp{static_cast<uint8_t>(age)}, ring)[day];
        }
    }
    return result;
}

void Histories::validate_variant_invariant() const {
    for (size_t day = 1; day <= day_cnt; ++day) {
        HistoryValue variant_total = 0;
        for (size_t v = 1; v <= real_variant_count_; ++v) {
            variant_total += aggregate_value(
                Trait::variant, Phase::now, static_cast<uint8_t>(v), day);
        }
        const HistoryValue infectious = aggregate_value(
            Trait::status, Phase::now, uint8_t(INFECTIOUS), day);
        if (variant_total != infectious)
            throw std::runtime_error(fmt::format(
                "Variant invariant failed on day {}: sum(now_variant)={} != now_infectious={}",
                day, variant_total, infectious));
    }
    fmt::println("Variant invariant OK: sum(now_variant) == now_infectious for all {} days.", day_cnt);
}

// ---------------------------------------------------------------
// HistorySelectorSet constructors and build_for_ages
// ---------------------------------------------------------------

void HistorySelectorSet::validate_sentinel(const char* s) {
    if (std::string_view(s) != "all")
        throw std::invalid_argument("only valid sentinel value is \"all\"");
}

HistorySelectorSet::HistorySelectorSet(const char* sentinel) {
    validate_sentinel(sentinel);
    std::vector<std::string> all_ages;
    all_ages.reserve(HISTORY_AGE_COUNT + 1);
    all_ages.emplace_back(age_vec_label(HISTORY_AGE_TOTAL));
    for (size_t age = 1; age <= HISTORY_AGE_COUNT; ++age) {
        all_ages.emplace_back(Agegrp::names[age]);
    }
    selections = build_for_ages(all_ages);
}

HistorySelectorSet::HistorySelectorSet(const char* sentinel, const char* age) {
    validate_sentinel(sentinel);
    selections = build_for_ages({std::string(age)});
}

HistorySelectorSet::HistorySelectorSet(
    const char* sentinel, std::vector<std::string> ages) {
    validate_sentinel(sentinel);
    selections = build_for_ages(std::move(ages));
}

std::vector<HistorySelector> HistorySelectorSet::build_for_ages(
        const std::vector<std::string>& ages) {
    std::vector<HistorySelector> out;
    for (const auto& age : ages) {
        // Status: skip index 0 ("none")
        for (size_t i = 1; i < Status::names.size(); ++i) {
            out.push_back({"now", std::string{Status::names[i]}, age});
            // new_unexposed is an allocated-but-never-written schema
            // placeholder, not a meaningful reporting selection.
            if (i != size_t(UNEXPOSED)) {
                out.push_back({"new_", std::string{Status::names[i]}, age});
            }
        }
        // Vaccinated aggregate (sums all brands)
        out.push_back({"now", "vaccinated", age});
        out.push_back({"new_", "vaccinated", age});
        // Per-brand vax: skip index 0 ("none")
        for (size_t i = 1; i < Vax::names.size(); ++i) {
            out.push_back({"now", "vax:" + Vax::names[i], age});
            out.push_back({"new_", "vax:" + Vax::names[i], age});
        }
        // Per-variant: skip index 0 ("none")
        for (size_t i = 1; i < Variant::names.size(); ++i) {
            out.push_back({"now", "variant:" + Variant::names[i], age});
            out.push_back({"new_", "variant:" + Variant::names[i], age});
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
    if (std::ranges::all_of(tok, [](unsigned char c) {
            return std::isdigit(c) != 0;
        })) {
        unsigned int value = 0;
        const auto [ptr, ec] = std::from_chars(
            tok.data(), tok.data() + tok.size(), value);
        if (ec == std::errc{} && ptr == tok.data() + tok.size()
            && value <= 255
            && (value == 0 || value < Ring::names.size())) {
            return static_cast<uint8_t>(value);
        }
        return std::nullopt;
    }
    auto it = std::find(Ring::names.begin(), Ring::names.end(), tok);
    if (it == Ring::names.end()) return std::nullopt;
    return static_cast<uint8_t>(std::distance(Ring::names.begin(), it));
}

namespace {

struct TraitSelectionView {
    std::string_view selector_prefix;
    Trait trait;
    std::span<const std::string> trait_value_names;
};

struct TotalHistorySource {
    std::string canonical_name;
    std::vector<size_t> source_history_idxs;
};

std::optional<TotalHistorySource> resolve_history_source(
    const Histories& histories, std::string_view phase_token,
    std::string_view trait_value_selector, uint8_t age, uint8_t ring) {
    const auto parsed_phase = magic_enum::enum_cast<Phase>(phase_token);
    if (!parsed_phase) return std::nullopt;
    const Phase phase = *parsed_phase;
    const auto display_phase = phase == Phase::now ? "now" : "new";

    if (age > HISTORY_AGE_COUNT) return std::nullopt;

    std::vector<uint8_t> rings;
    if (ring == RING_ALL) {
        rings.reserve(histories.ring_lane_count());
        if (histories.real_ring_count() == 0) {
            rings.push_back(RING_ALL);
        } else {
            for (size_t value = 1;
                 value <= histories.real_ring_count(); ++value) {
                rings.push_back(static_cast<uint8_t>(value));
            }
        }
    } else {
        if (histories.real_ring_count() == 0
            || ring > histories.real_ring_count()) {
            return std::nullopt;
        }
        rings.push_back(ring);
    }

    std::vector<Agegrp> ages;
    if (age == HISTORY_AGE_TOTAL) {
        ages.reserve(HISTORY_AGE_COUNT);
        for (size_t value = 1; value <= HISTORY_AGE_COUNT; ++value) {
            ages.emplace_back(static_cast<uint8_t>(value));
        }
    } else {
        ages.emplace_back(age);
    }

    const auto append_atomic_sources = [&histories, phase, &rings, &ages](
        TotalHistorySource& source, Trait trait,
        std::span<const uint8_t> trait_values) {
        source.source_history_idxs.reserve(
            trait_values.size() * rings.size() * ages.size());
        for (const uint8_t trait_value : trait_values) {
            for (const uint8_t atomic_ring : rings) {
                for (const Agegrp atomic_age : ages) {
                    source.source_history_idxs.push_back(
                        histories.history_vector_idx(
                            trait, phase, trait_value, atomic_age,
                            atomic_ring));
                }
            }
        }
    };

    if (trait_value_selector == "vaccinated") {
        TotalHistorySource source{
            fmt::format("{}_vaccinated", display_phase), {}};
        std::vector<uint8_t> trait_values;
        trait_values.reserve(histories.real_vax_count());
        for (size_t trait_value = 1;
             trait_value <= histories.real_vax_count(); ++trait_value) {
            trait_values.push_back(static_cast<uint8_t>(trait_value));
        }
        append_atomic_sources(source, Trait::vax, trait_values);
        return source;
    }

    const std::array traits{
        TraitSelectionView{
            "vax:", Trait::vax,
            std::span<const std::string>{Vax::names.data(), Vax::names.size()}},
        TraitSelectionView{
            "variant:", Trait::variant,
            std::span<const std::string>{Variant::names.data(), Variant::names.size()}},
    };

    for (const auto& trait : traits) {
        if (!trait_value_selector.starts_with(trait.selector_prefix)) continue;
        const auto trait_value_name =
            trait_value_selector.substr(trait.selector_prefix.size());
        const auto it = std::find(trait.trait_value_names.begin(),
                                  trait.trait_value_names.end(), trait_value_name);
        if (it == trait.trait_value_names.end()) return std::nullopt;

        const size_t trait_value = static_cast<size_t>(
            std::distance(trait.trait_value_names.begin(), it));
        if (trait_value == 0
            || trait_value > histories.trait_value_count(trait.trait)) {
            return std::nullopt;
        }

        TotalHistorySource source{
            fmt::format("{}_{}{}", display_phase, trait.selector_prefix,
                        trait.trait_value_names[trait_value]),
        };
        const std::array trait_values{static_cast<uint8_t>(trait_value)};
        append_atomic_sources(source, trait.trait, trait_values);
        return source;
    }

    const auto status = magic_enum::enum_cast<Status::Enum>(trait_value_selector);
    if (!status || *status == Status::Enum::none
        // Kept only so status now/new groups have the same fixed stride.
        || (phase == Phase::new_ && *status == Status::Enum::unexposed)) {
        return std::nullopt;
    }
    TotalHistorySource source{
        fmt::format("{}_{}", display_phase, magic_enum::enum_name(*status)), {}};
    const std::array trait_values{std::to_underlying(*status)};
    append_atomic_sources(source, Trait::status, trait_values);
    return source;
}

std::string raw_selection_label(const HistorySelector& selection) {
    return selection.ring.empty()
        ? fmt::format("{}|{}|{}", selection.phase, selection.trait_value, selection.age)
        : fmt::format("{}|{}|{}|{}", selection.phase, selection.trait_value,  selection.age, selection.ring);
}

std::string canonical_selection_label(const TotalHistorySource& source,
                                      uint8_t age, uint8_t ring) {
    if (ring == RING_ALL) {
        return fmt::format("{}:{}", source.canonical_name, age_vec_label(age));
    }
    const std::string ring_label = size_t(ring) < Ring::names.size()
        ? Ring::names[ring]
        : fmt::format("{}", ring);
    return fmt::format("{}:{}:{}", source.canonical_name, age_vec_label(age),
                       ring_label);
}

// total source vectors into a new history_vector
std::vector<HistoryValue> materialize_history_vector(
    const Histories& histories, const TotalHistorySource& source) {
    if (source.source_history_idxs.size() == 1) {
        return histories.history_vector(source.source_history_idxs.front());
    }

    std::vector<HistoryValue> result(histories.day_cnt + 1, 0);
    for (const size_t history_idx : source.source_history_idxs) {
        const auto& values = histories.history_vector(history_idx);
        for (size_t day = 0; day <= histories.day_cnt; ++day) {
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

TotalHistorySet create_history_set(
    const HistorySelectorSet & spec, const Histories& histories) {
  TotalHistorySet total_histories;
  auto& selections = spec.selections;
  total_histories.history_vectors.reserve(selections.size());

  for (const auto& sel : selections) {
    auto age = age_history_idx_from_string(sel.age);
    auto ring   = ring_id_from_token(sel.ring);
    const auto raw_label = raw_selection_label(sel);
    if (!age || !ring) {
      total_histories.invalid_selections.push_back(raw_label);
      continue;
    }
    auto source = resolve_history_source(histories, sel.phase, sel.trait_value, *age, *ring);
    if (!source) {
      total_histories.invalid_selections.push_back(raw_label);
      continue;
    }
    auto label = canonical_selection_label(*source, *age, *ring);
    auto data = materialize_history_vector(histories, *source);
    total_histories.history_vectors.push_back({
        std::move(label), std::move(data),
        std::move(source->source_history_idxs)});
  }

  return total_histories;
}

// ---------------------------------------------------------------
// Print functions
// ---------------------------------------------------------------

void print_total_status_histories(const Histories& histories,
                                  size_t days_per_group,
                                  std::ostream& out) {
  if (histories.day_cnt == 0) {
    fmt::println(out, "\nNo history to print.");
    return;
  }

  constexpr auto labels   = std::array{"infected", "unexposed", "recovered", "dead"};
  const HistorySelectorSet spec(std::vector<HistorySelector>{
      {"now", "infectious", "total"},
      {"now", "unexposed", "total"},
      {"now", "recovered", "total"},
      {"now", "dead", "total"},
  });
  const auto total_histories = create_history_set(spec, histories);
  if (!total_histories.invalid_selections.empty()
      || total_histories.history_vectors.size() != labels.size()) {
    fmt::println(out, "\nUnable to resolve total status histories: {}",
                 total_histories.invalid_selections);
    return;
  }

  fmt::println(out, "\nTotal status histories");
  for (size_t group_start = 1; group_start <= histories.day_cnt;
       group_start += days_per_group) {
    const size_t group_end =
        std::min(group_start + days_per_group - 1, histories.day_cnt);
    fmt::println(out, "\nDays {}-{}", group_start, group_end);

    fmt::print(out, "{:<12}", "day");
    for (size_t day = group_start; day <= group_end; ++day) {
      fmt::print(out, "{:>8}", day);
    }
    fmt::println(out, "");

    for (size_t status = 0; status < labels.size(); ++status) {
      fmt::print(out, "{:<12}", labels[status]);
      for (size_t day = group_start; day <= group_end; ++day) {
        fmt::print(out, "{:>8}",
                   total_histories.history_vectors[status].data[day]);
      }
      fmt::println(out, "");
    }
  }
}

void print_selected_histories(HistorySelectorSet spec,
                              const Histories& histories,
                              size_t days_per_group,
                              std::ostream& out) {
  auto& selections = spec.selections;
  if (histories.day_cnt == 0) {
    fmt::println(out, "\nNo history to print.");
    return;
  }
  if (selections.empty()) {
    fmt::println(out, "\nNo history selected.");
    return;
  }

  auto total_histories = create_history_set(spec, histories);
  const auto& history_vectors = total_histories.history_vectors;

  if (!total_histories.invalid_selections.empty()) {
    fmt::println(out, "\nSkipping invalid history selections: {}",
                 total_histories.invalid_selections);
  }
  if (history_vectors.empty()) {
    fmt::println(out, "\nNo valid history selected.");
    return;
  }

  fmt::println(out, "\nSelected histories:");
  for (size_t group_start = 1; group_start <= histories.day_cnt;
       group_start += days_per_group) {
    const size_t group_end =
        std::min(group_start + days_per_group - 1, histories.day_cnt);
    fmt::println(out, "\nDays {}-{}", group_start, group_end);

    fmt::print(out, "{:<12}", "day");
    for (size_t day = group_start; day <= group_end; ++day) {
      fmt::print(out, "{:>8}", day);
    }
    fmt::println(out, "");

    for (const auto& history : history_vectors) {
      fmt::print(out, "{:<12}", history.label);
      for (size_t day = group_start; day <= group_end; ++day) {
        fmt::print(out, "{:>8}", history.data[day]);
      }
      fmt::println(out, "");
    }
  }
}

// ---------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------

void serialize_selected_histories(HistorySelectorSet spec,
                                  const Histories& histories,
                                  std::filesystem::path output_path) {
  auto & selections = spec.selections;
  if (histories.day_cnt == 0) {
    fmt::println("\nNo history to output.");
    return;
  }
  if (selections.empty()) {
    fmt::println("\nNo history selected.");
    return;
  }

  auto total_histories = create_history_set(spec, histories);
  const auto& history_vectors = total_histories.history_vectors;

  if (!total_histories.invalid_selections.empty()) {
    fmt::println("\nSkipping unknown history selections: {}",
                 total_histories.invalid_selections);
  }
  if (history_vectors.empty()) {
    fmt::println("\nNo valid history selected.");
    return;
  }

  ensure_parent_dir(output_path);

  std::ofstream out(output_path);
  if (!out) {
    throw std::runtime_error(
        fmt::format("Could not write history CSV to '{}'", output_path.string()));
  }

  // Write header
  vector<string> headers;
  headers.reserve(history_vectors.size());
  for (const auto& history : history_vectors) headers.push_back(history.label);
  fmt::println(out, "{}", fmt::join(headers, ","));

  // Write rows (1-indexed days)
  vector<HistoryValue> row;
  row.reserve(history_vectors.size());
  for (size_t i = 1; i <= histories.day_cnt; ++i) {
    for (const auto& history : history_vectors) {
      row.push_back(history.data[i]);
    }
    fmt::println(out, "{}", fmt::join(row, ","));
    row.clear();
  }

  fmt::println("Wrote selected history CSV to '{}'", output_path.string());
}

void serialize_selected_histories(HistorySelectorSet spec,
                                  const Histories& histories,
                                  string base_fname,
                                  vector<string> path_steps) {
  const char* home = std::getenv("HOME");
  if (!home) throw std::runtime_error("HOME not set");
  std::filesystem::path fpath{home};
  if (path_steps.empty()) path_steps = {"code", "epi_sim", "series_output"};
  for (auto step : path_steps) fpath /= step;
  fpath /= make_timestamped_filename(base_fname) + ".csv";

  serialize_selected_histories(std::move(spec), histories, fpath);
}
