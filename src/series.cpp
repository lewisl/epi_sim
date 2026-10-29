#include "series.h"
#include "parameters.h"
#include "population.h"
#include "traits.h"
#include <charconv>
#include <stdexcept>
#include <magic_enum/magic_enum.hpp>
#include "absl/container/flat_hash_map.h"
#include <fmt/format.h>
#include <fmt/base.h>


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
Histories::Histories(size_t n_days, const PopData& pop)
    : day_cnt(n_days)
  {
    const std::array trait_value_counts{
        Status::count, Vax::count, Variant::count};

    // create and initialize outer vector container and inner vectors for histories accumulated during simulation
    // this instantiates only base history vectors updated during simulaton
    sim_history_count = magic_enum::enum_count<Phase>() * 
          ((Status::count * Ring::count * Agegrp::count) +
          (Vax::count * Ring::count * Agegrp::count) +
          (Variant::count * Ring::count * Agegrp::count));


    if (n_days == 0) return;

    // create sim history vectors
    build_history_vectors(false);
    // create total history vectors after sim_history_vectors
    build_history_vectors(true);

    // update counts
    all_history_count = history_vectors_.size();
    total_history_count = all_history_count - sim_history_count;

    // All people begin as now_unexposed. new_unexposed has physical columns
    // only to preserve the uniform phase width; simulation code never writes it.
    for (size_t person = 1; person <= pop.popn; ++person) {
        const Agegrp age = pop.agegrp[person];
        const uint8_t ring = uint8_t(pop.ring[person]);
        if (age.v == 0 || age.v > Agegrp::count
            || ring == 0 || ring > Ring::count) {
            throw std::runtime_error(fmt::format(
                "Cannot seed history for person {}: age={} ring={}",
                person, age.v, ring));
        }
        update(TraitID::status, Phase::now, uint8_t(UNEXPOSED),
               ring, age, 1, 1);
    }

    // cumulative columns and new columns
    for (const auto & [key, col_idx] : sim_history_map) {
      if (key.phase == std::to_underlying(Phase::now)) cumulative_columns.push_back(col_idx);
      if (key.phase == std::to_underlying(Phase::new_)) new_columns.push_back(col_idx);
    }
}


void Histories::build_history_vectors(bool for_totals) {
    size_t max_names;
    Selsim selsim{};
    Seltot seltot{};
    size_t idx_sim{0};

    // initialize base index values for adding columns
    if (for_totals) idx_sim = sim_history_count;    // displacement of existing simulation tracked history vectors
    else idx_sim = 0;    // history vectors updated during simulation

    for (auto tr_enum : Tracked_Traits ) { 
      HistoryMeta hm;
      hm.traitnum = tr_enum;
      hm.trait = magic_enum::enum_name(tr_enum);
      selsim.trait = static_cast<uint8_t>(tr_enum);
      seltot.trait = static_cast<uint8_t>(tr_enum);

      max_names = tr_enum == TraitID::status ? Status::count :
                  tr_enum == TraitID::vax ? Vax::count : Variant::count;

      for (auto ph_num : all_phases) {
        hm.phase = magic_enum::enum_name(ph_num);
        hm.phasenum = ph_num;
        selsim.phase = static_cast<uint8_t>(ph_num);
        seltot.phase = static_cast<uint8_t>(ph_num);


        for (size_t trval_num = 1; trval_num <= max_names ; ++trval_num) {
          // trval for map key is uint8_t, not trait class value
          selsim.trval = trval_num;
          seltot.trval = trval_num;
          switch (tr_enum) {
            case TraitID::status:
            {
              hm.traitval = Status::names[trval_num]; // string
              hm.traitval_num = Status{uint8_t(trval_num)};
              break;
            }
            case TraitID::vax:
              hm.traitval = Vax::names[trval_num]; // string
              hm.traitval_num = trval_num;
              break;
            case TraitID::variant:
              hm.traitval = Variant::names[trval_num];
              hm.traitval_num = trval_num;
              break;
            default:
              throw std::range_error("Only status, vax, and variant allowed in history building switch.");
          } // switch on tr_enum

          // create and place detail simulation result vectors
          if (!for_totals) {     
            for (auto ring_i = 1; ring_i <= Ring::count; ++ring_i) { 
                hm.ring = Ring::names[ring_i];
                hm.ringnum = ring_i;
                selsim.ring = ring_i;
              // }
              for (auto age_i = 1; age_i < Agegrp::names.size(); ++age_i) {
                hm.age = Agegrp::names[age_i];
                hm.agenum = age_i;
                selsim.age = age_i;
                // emplace_back a HistoryVector for simulation detail outcome
                hm.output_label = fmt::format("{}|{}|{}|{}|{}",
                  hm.trait, hm.phase, hm.traitval, hm.ring, hm.age); 
                history_vectors_.emplace_back(std::vector<HistoryValue>(day_cnt+1, HistoryValue(0)), hm);
                sim_history_map.emplace(selsim, idx_sim);
                idx_sim++;  // only increment index count for new vector with new idx
              } // age loop

            }  // ring loop
          }
          
          else {    // for_totals true
            //  
            // full total:  both rings and ages to be summed
            //       one for trait-phase-trval
            hm.ring = "total";
            hm.ringnum = RING_ALL;
            hm.age = "total";
            hm.agenum = HISTORY_AGE_TOTAL;
            hm.output_label = fmt::format("{}|{}|{}|{}|{}",
                hm.trait, hm.phase, hm.traitval, hm.ring, hm.age); 
            seltot.age = HISTORY_AGE_TOTAL;
            seltot.ring = RING_ALL;
            // emplace_back a HistoryVector for a total
            history_vectors_.emplace_back(std::vector<HistoryValue>(day_cnt+1, HistoryValue(0)), hm);
            sim_totals_map.emplace(seltot, idx_sim);
            idx_sim++;   // only increment index count for new vector with new idx

            /*
            SPLIT TOTALS
            For split totals we always have multiple agegrps, but we might have only
            one ring: 
            - For totaling only one ring per age, it's just the simulation columns.
            - For totaling the agegrps for one ring, it's just the "full" totals.
            - But, we will create the keys so that the keys can always be used, but we map to indices for existing columns. We don't need a vector so we don't increment 
            multiple rings:
            */
            if (Ring::count == 1) {
                // "totaling" one ring per each age with a key for each age & ring = "total":
                //      use existing sim history column of age and ring, 
                //      but set a new key using ring = "total"
                //      don't create a vector; don't increment the index value idx_sim and we don't need
                //      hm for metadata because we use the existing column
                //      all we need is a new key that maps to the existing column.
                // Agegrp::count new keys in sim_totals_map; no new vectors (no new vector, no new idx)
                for (auto age_i = 1; age_i <= Agegrp::count; ++age_i) {
                  seltot.ring = uint8_t{0};
                  seltot.age = age_i;  // age_i is the agenum
                  // key to existing sim history vector
                  const Selsim tempkey{hm.traitnum, hm.phasenum, hm.traitval_num, Ring{1}, seltot.age};
                  auto it = sim_history_map.find(tempkey);
                  if (it != sim_history_map.end()) 
                      sim_totals_map.emplace(seltot, it->second);
                  else throw std::range_error(fmt::format("Index to existing history column {} not found.", tempkey));
                }
              
                // totaling ages for the only ring: use existing total column--all rings (one) and totaled ages,
                //      which already exists as the full total column
                
                seltot.ring = uint8_t{1}; // would be have string label of "population" or some input name
                seltot.age = HISTORY_AGE_TOTAL;
                // key to existing totals columns
                const Seltot tempkey{hm.traitnum, hm.phasenum, hm.traitval_num};
                auto it = sim_totals_map.find(tempkey);
                if (it != sim_totals_map.end()) {
                  const size_t col_idx = it->second;  // !! copy to avoid rehashing!
                  sim_totals_map.emplace(seltot, col_idx);}
                else throw std::range_error(fmt::format("Index to existing totals column {} not found.", tempkey));
            }
            else {    // split totals with multple rings  
            // split total: sum of ages for each ring
            for (auto ring_i = 1; ring_i <= Ring::count; ++ring_i) {
              hm.age = "total";
              hm.agenum = HISTORY_AGE_TOTAL;  
              hm.ring = Ring::names[ring_i];
              hm.ringnum = ring_i;
              hm.output_label = fmt::format("{}|{}|{}|{}|{}", 
                hm.trait, hm.phase, hm.traitval, hm.ring, hm.age);
              seltot.ring = ring_i;
              seltot.age = HISTORY_AGE_TOTAL;
              history_vectors_.emplace_back(std::vector<HistoryValue>(day_cnt+1,HistoryValue(0)), hm);
              sim_totals_map.emplace(seltot, idx_sim);
              idx_sim++;
            }
            // split total sum of rings for each age
            for (auto age_i = 1; age_i <= Agegrp::count; ++age_i) {
              hm.age = Agegrp::names[age_i];
              hm.agenum = age_i;  
              hm.ring = "total";
              hm.ringnum = RING_ALL;
              hm.output_label = fmt::format("{}|{}|{}|{}|{}",
                hm.trait, hm.phase, hm.traitval, hm.ring, hm.age);
              seltot.ring = RING_ALL;
              seltot.age = age_i;
              history_vectors_.emplace_back(std::vector<HistoryValue>(day_cnt+1,HistoryValue(0)), hm);
              sim_totals_map.emplace(seltot, idx_sim);
              idx_sim++;
            }
          }
        } // else for_totals == true
      }   // tr_val loop
    }     // phase loop
  }       // trait loop
}

// for simulation updated history vectors; not applicable to total history vectors
// performed 1 day at a time for the current day in the day loop
// for phase == 'now', each day's history begins with final count of preceding day
// not applicable to phase == 'new_'
void Histories::init_history_newday(size_t day) {
    if (day <= 1) return;
    for (const auto idx : cumulative_columns) {
      history_vectors_[idx].data[day] = history_vectors_[idx].data[day - 1];
    }
    // for (const TraitID trait : Tracked_Traits ) {
    //     const auto begin = trait_phase_base(trait, Phase::now);
    //     const auto end = begin + phase_width(trait);
    //     for (size_t history_idx = begin; history_idx < end; ++history_idx) {
    //         history_vectors_[history_idx].data[day] = history_vectors_[history_idx].data[day - 1];
    //     }
    // }
}

//=============================================================================
// do over on serialization, printing, plotting using histories
//=============================================================================

std::vector<size_t> Histories::select_indices(HistorySelectorSet sel, std::string grab_bag) {
  vector<size_t> idxs{};

  if (grab_bag == "all") idxs = std::views::iota(0uz, history_vectors_.size()) | 
    std::ranges::to<std::vector>();  // all the indices in sequence into the vector
  else if (grab_bag == "total") idxs = std::views::iota(sim_history_count + 1uz, history_vectors_.size()) | 
    std::ranges::to<std::vector>();  // indices of all total history vectors
  else {
    for (auto & s : sel.selections) {
      auto traitnum = trait_lookup(s.trait);
      auto phasenum = s.phase == "now" ? uint8_t{0} : uint8_t{1};
      auto tr_val = tr_val_to_id(traitnum, s.trait_value);

      // use total map:  both are total. this stop in sieve must be 1st
      if (s.age == "total" && s.ring == "total") {
        Seltot seltot{traitnum, phasenum, tr_val};
        auto it = sim_totals_map.find(seltot); // key exists?
        if (it != sim_totals_map.end()) idxs.push_back(it->second);
        else fmt::println("Key {} not found. skipped", seltot);
      }
      else if (s.age == "total" || s.ring == "total") {
        auto ringnum{HISTORY_AGE_TOTAL}; auto agenum{RING_ALL};
        if (s.age == "total") {agenum = HISTORY_AGE_TOTAL;
          ringnum = tr_val_to_id(TraitID::ring, s.ring);}
        if (s.ring == "total") {ringnum = RING_ALL;
            agenum = tr_val_to_id(TraitID::agegrp, s.age);}
        Seltot seltot{traitnum, phasenum, tr_val, ringnum, agenum };
        auto it = sim_totals_map.find(seltot); // key exists?
        if (it != sim_totals_map.end()) idxs.push_back(it->second);
        else fmt::println("Key {} not found. skipped", seltot);
      }
      // use history (simulation tracked) map
      else {
        auto ringnum = tr_val_to_id(TraitID::ring, s.ring);
        auto agenum = tr_val_to_id(TraitID::agegrp, s.age);
        Selsim selsim{traitnum, phasenum,tr_val,ringnum, agenum};
        auto it = sim_history_map.find(selsim); // key exists?
        if (it != sim_history_map.end()) idxs.push_back(it->second);
        else fmt::println("Key {} not found. skipped",selsim);
      }
    }
  }
  return idxs;
}


void serialize_histories_new_way(const Histories & histories, std::vector<size_t> idxs,
    std::filesystem::path output_path) {
  if (histories.day_cnt == 0) {
    fmt::println("\nNo history to output.");
    return;
  }
  if (idxs.empty()) {
    fmt::println("\nNo history selected.");
    return;
  }

  ensure_parent_dir(output_path);

  std::ofstream out(output_path);
  if (!out) {
    throw std::runtime_error(
        fmt::format("Could not write history CSV to '{}'", output_path.string()));
  }

  // Write header
  vector<std::string_view> headers;
  headers.reserve(idxs.size());
  for (auto idx : idxs) 
    headers.push_back(histories.history_vectors_[idx].meta.output_label);
  fmt::println(out, "{}", fmt::join(headers, ","));

  // write rows
  vector<HistoryValue> row;
  row.reserve(idxs.size());
  for (size_t i = 1; i <= histories.day_cnt; ++i) {
    for (auto idx : idxs) {
          row.push_back(histories.history_vectors_[idx].data[i]);}
    fmt::println(out, "{}", fmt::join(row, ","));
    row.clear();
  }
  fmt::println("Wrote selected history CSV to '{}'", output_path.string());
}

std::filesystem::path set_output_path(string base_fname, vector<string> path_steps) {
  const char* home = std::getenv("HOME");
  if (!home) throw std::runtime_error("HOME not set");
  std::filesystem::path fpath{home};
  if (path_steps.empty()) path_steps = {"code", "epi_sim", "series_output"};
  for (auto step : path_steps) fpath /= step;
  fpath /= make_timestamped_filename(base_fname) + ".csv";
  return fpath;
}

//=============================================================================
//=============================================================================


size_t Histories::trait_value_count(TraitID trait) const {
    switch (trait) {
        case TraitID::status: 
          return Status::names.size() - 1;
        case TraitID::vax: 
          return Vax::count;
        case TraitID::variant: 
          return Variant::count;
        default:
          throw std::runtime_error("Invalid trait value for histories.");
    }

}

bool Histories::valid_history_coordinates(
    TraitID trait, Phase phase, uint8_t trait_value, Agegrp age,
    uint8_t ring) const {
    if (size_t(trait) >= magic_enum::enum_count<TraitID>()
        || size_t(phase) >= magic_enum::enum_count<Phase>()
        || trait_value == 0
        || size_t(trait_value) > trait_value_count(trait)
        || age.v == 0 || age.v > Agegrp::count) {
        return false;
    }
    return ring >= 1 && size_t(ring) <= Ring::count;
}

//
// introspection of Histories object and specific history vectors
//

std::optional<HistorySelector> Histories::describe_history_vector(size_t history_idx) const {
    if (history_idx >= history_vectors_.size()) return std::nullopt;

    const HistoryMeta & meta(history_vectors_[history_idx].meta);

    return HistorySelector{
                .phase = meta.phase,
                .trait_value = meta.traitval,
                .age = meta.age,
                .trait = meta.trait,
                .ring = meta.ring
            };
}

void Histories::sum_total_histories() {
  Selsim selsim;  // selector for source history vectors
 
  for (auto [totals_key, totals_idx] : sim_totals_map) {   // iterate over destination vectors

    if (Ring::count == 1 && (totals_key.age != HISTORY_AGE_TOTAL || totals_key.ring != RING_ALL)) continue; //split total with 1 ring

    selsim.trait = totals_key.trait; selsim.phase = totals_key.phase; selsim.trval = totals_key.trval;

    for (size_t d_i = 1; d_i <= day_cnt; ++d_i) {
      HistoryValue result = 0;
      for (uint8_t age_i = 1; age_i <= Agegrp::count; ++age_i) {  // iterate over the source vectors
        if (totals_key.age != HISTORY_AGE_TOTAL && totals_key.age != age_i) continue;
        selsim.age = age_i;

        for (uint8_t ring_i = 1; ring_i <= Ring::count; ++ring_i) {
          if (totals_key.ring != RING_ALL && totals_key.ring != ring_i) continue;
          selsim.ring = ring_i;
          auto it = sim_history_map.find(selsim);
          if (it == sim_history_map.end()) 
            throw std::range_error(fmt::format("Source vector for history {} doesn't exist.", selsim));
          result += history_vectors_[it->second].data[d_i];
        } // source ring loop
          
      } // source age loop 
      history_vectors_[totals_idx].data[d_i] = result;
    } // day loop
  } // totals map keys loop
}


std::string Histories::explain_history_vector_idx(
    TraitID trait, Phase phase, uint8_t trait_value, Agegrp age,
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
        size_t(ring - 1), size_t(age.v - 1));
}

void Histories::dump_history_layout(std::ostream& out) const {
    for (size_t history_idx = 0; history_idx < history_vectors_.size(); ++history_idx) {
        fmt::println(out, "{}: {}", history_idx, history_column_label(history_idx));
    }
}


const std::vector<HistorySelector> enumerate_history_selections(const Histories& histories) {
    vector<HistorySelector> vh;
    // for (auto i = 0; i < histories.sim_history_count; ++i) {
    for (auto i = 0; i < histories.all_history_count; ++i) {

      auto desc = histories.describe_history_vector(i);
      if (desc)
        vh.push_back(*desc);
      else 
        throw std::runtime_error(fmt::format("history description failed at {}", i));
    }
    return vh;
  }

// we should see if we need this, but it works well now
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
    TraitID trait, Phase phase, uint8_t trait_value, size_t day) const {
    HistoryValue result = 0;
    for (size_t ring_i = 1; ring_i <= Ring::count; ++ring_i) {
        const uint8_t ring = static_cast<uint8_t>(ring_i);
        for (size_t age = 1; age <= Agegrp::count; ++age) {
            result += at(trait, phase, trait_value,
                         Agegrp{static_cast<uint8_t>(age)}, ring)[day];
        }
    }
    return result;
}


// ---------------------------------------------------------------
// HistorySelectorSet constructors and build_for_ages
// ---------------------------------------------------------------


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
        out.emplace_back("now", "vaccinated", age);  // any perf difference compared to push_back of literal?
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
    TraitID trait;
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

    if (age > Agegrp::count) return std::nullopt;

    std::vector<uint8_t> rings;
    if (ring == RING_ALL) {
        rings.reserve(Ring::count);
        for (size_t value = 1;
             value <= Ring::count; ++value) {
            rings.push_back(static_cast<uint8_t>(value));
        }
    } else {
        if (ring > Ring::count) {
            return std::nullopt;
        }
        rings.push_back(ring);
    }

    std::vector<Agegrp> ages;
    if (age == HISTORY_AGE_TOTAL) {
        ages.reserve(Agegrp::count);
        for (size_t value = 1; value <= Agegrp::count; ++value) {
            ages.emplace_back(static_cast<uint8_t>(value));
        }
    } else {
        ages.emplace_back(age);
    }

    const auto append_atomic_sources = [&histories, phase, &rings, &ages](
        TotalHistorySource& source, TraitID trait,
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
        trait_values.reserve(Vax::count);
        for (size_t trait_value = 1;
             trait_value <= Vax::count; ++trait_value) {
            trait_values.push_back(static_cast<uint8_t>(trait_value));
        }
        append_atomic_sources(source, TraitID::vax, trait_values);
        return source;
    }

    const std::array traits{
        TraitSelectionView{
            "vax:", TraitID::vax,
            std::span<const std::string>{Vax::names.data(), Vax::names.size()}},
        TraitSelectionView{
            "variant:", TraitID::variant,
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
    append_atomic_sources(source, TraitID::status, trait_values);
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

    std::vector<HistoryValue> result(histories.day_cnt + 1, 0);  // virtual 1-indexing
    for (const size_t history_idx : source.source_history_idxs) {
        const auto& values = histories.history_vector(history_idx);
        for (size_t day = 0; day <= histories.day_cnt; ++day) {
            result[day] += values[day];
        }
    }
    return result;
}

} // namespace


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
        std::move(label), std::move(data),        // move consumes vector, so have to create again for another use
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

    for (size_t status = 1; status < labels.size(); ++status) {
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

  auto total_histories = create_history_set(spec, histories);  // don't create: check existence
  const auto& history_vectors = total_histories.history_vectors;

  if (!total_histories.invalid_selections.empty()) {
    fmt::println("\nSkipping unknown history selections: {}",
                 total_histories.invalid_selections);
  }
  if (history_vectors.empty()) {      // as reference to existing total history vectors
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
