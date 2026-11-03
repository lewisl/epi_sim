#include "series.h"
#include "parameters.h"
#include "population.h"
#include "traits.h"
// #include <charconv>
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

    if (n_days == 0) return;

    // create sim history vectors
    build_history_vectors(false);
    sim_history_count = history_vectors_.size();
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

    for (auto tr_enum : Tracked_Traits ) { 
      const auto & trait = trait_table[std::to_underlying(tr_enum)];
      HistoryMeta hm;
      hm.traitnum = tr_enum;
      hm.trait = magic_enum::enum_name(tr_enum);


      for (auto ph_num : all_phases) {
        hm.phase = magic_enum::enum_name(ph_num);
        hm.phasenum = ph_num;


        for (size_t trval_num = 1; trval_num <= trait.count ; ++trval_num) {
          hm.traitval  = trait.names[trval_num];
          hm.traitval_num = trval_num;

          // create and place detail simulation result vectors
          if (!for_totals) {     
            for (auto ring_i = 1; ring_i <= Ring::count; ++ring_i) { 
                hm.ring = Ring::names[ring_i];
                hm.ringnum = ring_i;
                // selsim.ring = ring_i;
              // }
              for (auto age_i = 1; age_i < Agegrp::names.size(); ++age_i) {
                hm.age = Agegrp::names[age_i];
                hm.agenum = age_i;
                
                // emplace_back a HistoryVector for simulation detail outcome
                hm.output_label = fmt::format("{}|{}|{}|{}|{}",
                  hm.trait, hm.phase, hm.traitval, hm.ring, hm.age); 
                const size_t col_idx = history_vectors_.size();
                history_vectors_.emplace_back(std::vector<HistoryValue>(day_cnt+1, HistoryValue(0)), hm);
                sim_history_map.emplace(Selsim(tr_enum, ph_num, trval_num, ring_i, age_i), col_idx);
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

            // emplace_back a HistoryVector for a total
            const size_t full_total_idx = history_vectors_.size();
            history_vectors_.emplace_back(std::vector<HistoryValue>(day_cnt+1, HistoryValue(0)), hm);
            sim_totals_map.emplace(Seltot(tr_enum, ph_num, trval_num, RING_ALL, HISTORY_AGE_TOTAL), full_total_idx);

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
                  // key to existing sim history vector
                  const Selsim tempkey(tr_enum, ph_num, trval_num, uint8_t{1}, age_i);
                  auto it = sim_history_map.find(tempkey);
                  if (it != sim_history_map.end()) 
                      sim_totals_map.emplace(Seltot(tr_enum, ph_num, trval_num, RING_ALL, age_i), it->second);
                  else throw std::range_error(fmt::format("Index to existing history column {} not found.", tempkey));
                }
              
                // totaling ages for the only ring: use existing total column--all rings (one) and totaled ages,
                //      which already exists as the full total column
                
                sim_totals_map.emplace(Seltot(tr_enum, ph_num, trval_num, uint8_t{1}, HISTORY_AGE_TOTAL), full_total_idx);
            }
            else {    // split totals with multple rings  
            // split total: sum of ages for each ring
            hm.age = "total";
            hm.agenum = HISTORY_AGE_TOTAL;  
            for (auto ring_i = 1; ring_i <= Ring::count; ++ring_i) {
              hm.ring = Ring::names[ring_i];
              hm.ringnum = ring_i;
              hm.output_label = fmt::format("{}|{}|{}|{}|{}", 
                hm.trait, hm.phase, hm.traitval, hm.ring, hm.age);
              const size_t col_idx = history_vectors_.size();
              history_vectors_.emplace_back(std::vector<HistoryValue>(day_cnt+1,HistoryValue(0)), hm);
              sim_totals_map.emplace(Seltot(tr_enum, ph_num, trval_num, ring_i, HISTORY_AGE_TOTAL), col_idx);
            }
            // split total sum of rings for each age
            hm.ring = "total";
            hm.ringnum = RING_ALL;
            for (auto age_i = 1; age_i <= Agegrp::count; ++age_i) {
              hm.age = Agegrp::names[age_i];
              hm.agenum = age_i;  
              hm.output_label = fmt::format("{}|{}|{}|{}|{}",
                hm.trait, hm.phase, hm.traitval, hm.ring, hm.age);
              const size_t col_idx = history_vectors_.size();
              history_vectors_.emplace_back(std::vector<HistoryValue>(day_cnt+1,HistoryValue(0)), hm);
              sim_totals_map.emplace(Seltot(tr_enum, ph_num, trval_num, RING_ALL, age_i), col_idx);
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
}



std::vector<size_t> Histories::select_indices(HistorySelectorSet sel, std::string grab_bag) {
  vector<size_t> idxs{};

  if (grab_bag == "all") idxs = std::views::iota(0uz, history_vectors_.size()) | 
    std::ranges::to<std::vector>();  // all the indices in sequence into the vector

  else if (grab_bag == "total") idxs = std::views::iota(sim_history_count, history_vectors_.size()) | 
    std::ranges::to<std::vector>();  // indices of all total history vectors

  else {
    for (auto & s : sel.selections) {
      auto traitnum = trait_lookup(s.trait);
      auto phasenum = s.phase == "now" ? uint8_t{0} : uint8_t{1};
      auto tr_val = tr_val_to_id(traitnum, s.trait_value);

      // use total map:  both are total. this stop in sieve must be first
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


void serialize_histories(Histories & histories, HistorySelectorSet sel, std::string grab_bag,
    std::filesystem::path output_path) {

  if (histories.day_cnt == 0) {
    fmt::println("\nNo history to output.");
    return;
  }

  vector<size_t> idxs = histories.select_indices(sel, grab_bag);


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

void serialize_histories(Histories& histories, 
                                 HistorySelectorSet sel, std::string grab_bag,
                                  string base_fname,
                                  vector<string> path_steps) {
  const char* home = std::getenv("HOME");
  if (!home) throw std::runtime_error("HOME not set");
  std::filesystem::path fpath{home};
  if (path_steps.empty()) path_steps = {"code", "epi_sim", "series_output"};
  for (auto step : path_steps) fpath /= step;
  fpath /= make_timestamped_filename(base_fname) + ".csv";

  serialize_histories(histories, sel, grab_bag, fpath);
}


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
                .trait = meta.trait,
                .phase = meta.phase,
                .trait_value = meta.traitval,
                .age = meta.age,
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
                              std::string_view title,
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
