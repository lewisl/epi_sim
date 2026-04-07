
#include "lib_includes.h"

#include "parameters.h"
#include "population.h"
#include "random.h"
#include "sim.h"
#include "setup.h"
#include "disease_modeling.h"
#include "spread.h"
#include "progression.h"
#include "timing.h"
#include "series.h"
#include "plot.h"
#include "agent_pop_print.h"
#include "vaccination.h"

// forward declarations
SummaryData print_summary(PopData & pop);


namespace {

// Allowed trait names for Filter and Change terms, validated at load time.
const std::unordered_set<string> FILTER_TRAITS = {
    "status", "agegrp", "cond", "variant", "vaxstatus", "quar", "tested"};
const std::unordered_set<string> CHANGE_TRAITS = {
    "status", "cond", "duration", "variant", "vaxstatus", "quar"};

// Parse a single Term value from JSON given its trait name.
// Throws std::runtime_error on unknown or disallowed values.
int32_t parse_term_val(const string& trait, const json& jval, const ModelParams& mp) {
  if (trait == "status") {
    auto opt = trait_from_string<Status>(jval.get<string>());
    if (!opt) throw std::runtime_error("Unknown status value: " + jval.get<string>());
    return int32_t(uint8_t(*opt));
  }
  if (trait == "agegrp") {
    auto opt = trait_from_string<Agegrp>(jval.get<string>());
    if (!opt) throw std::runtime_error("Unknown agegrp value: " + jval.get<string>());
    return int32_t(uint8_t(*opt));
  }
  if (trait == "cond") {
    auto opt = trait_from_string<Condition>(jval.get<string>());
    if (!opt) throw std::runtime_error("Unknown cond value: " + jval.get<string>());
    return int32_t(uint8_t(*opt));
  }
  if (trait == "vaxstatus") {
    auto opt = trait_from_string<Vaxstatus>(jval.get<string>());
    if (!opt) throw std::runtime_error("Unknown vaxstatus value: " + jval.get<string>());
    return int32_t(uint8_t(*opt));
  }
  if (trait == "variant") {
    const string vname = jval.get<string>();
    auto it = std::find(Variant::names.begin(), Variant::names.end(), vname);
    if (it == Variant::names.end())
      throw std::runtime_error("Unknown variant: " + vname);
    uint8_t vidx = static_cast<uint8_t>(std::distance(Variant::names.begin(), it));
    if (vidx == 0)
      throw std::runtime_error("Variant 'none' (index 0) is not valid in seed terms");
    return int32_t(vidx);
  }
  // Numeric traits: duration, quar
  return jval.get<int32_t>();
}

// Returns true if all filter terms match the person (AND semantics).
bool matches_filter(AgentView person, const Filter& filt) {
  for (const auto& f : filt.terms) {
    int32_t pval{};
    if      (f.trait == "status")    pval = int32_t(uint8_t(person.status()));
    else if (f.trait == "agegrp")    pval = int32_t(uint8_t(person.agegrp()));
    else if (f.trait == "cond")      pval = int32_t(uint8_t(person.cond()));
    else if (f.trait == "variant")   pval = int32_t(uint8_t(person.variant()));
    else if (f.trait == "vaxstatus") pval = int32_t(uint8_t(person.vaxstatus()));
    else if (f.trait == "quar")      pval = int32_t(person.quar());
    else if (f.trait == "tested")    pval = int32_t(person.tested());
    if (pval != f.val) return false;
  }
  return true;
}

// Apply change terms to a person via AgentView, routing status changes through
// make_sick / make_well / make_dead to preserve all invariants.
// Guard: make_sick is only applied to unexposed or recovered persons.
void apply_change(AgentView person, const Change& chg, HistorySeries& series) {
  // Find a status term if present
  auto status_it = std::find_if(chg.terms.begin(), chg.terms.end(),
                                [](const Term& t) { return t.trait == "status"; });

  if (status_it != chg.terms.end()) {
    Status new_status{static_cast<uint8_t>(status_it->val)};

    if (new_status == INFECTIOUS) {
      if (person.status() != UNEXPOSED && person.status() != RECOVERED) {
        fmt::println("WARNING: skipping make_sick for person {}: status is '{}', must be unexposed or recovered",
                     person.id, person.status().show());
        return;
      }
      Variant  var{1};        // default: base (index 1)
      Condition cond = NIL;
      Duration  dur{1};
      for (const auto& t : chg.terms) {
        if      (t.trait == "variant")  var  = Variant{static_cast<uint8_t>(t.val)};
        else if (t.trait == "cond")     cond = Condition{static_cast<uint8_t>(t.val)};
        else if (t.trait == "duration") dur  = t.val;
      }
      person.make_sick(var, series, cond, dur);
      // Fall through to apply any remaining terms not consumed by make_sick
      for (const auto& t : chg.terms) {
        if (t.trait == "status" || t.trait == "variant" ||
            t.trait == "cond"   || t.trait == "duration") continue;
        if      (t.trait == "vaxstatus") person.vaxstatus() = Vaxstatus{static_cast<uint8_t>(t.val)};
        else if (t.trait == "quar")      person.quar()      = static_cast<uint8_t>(t.val);
      }
      return;
    }

    if (new_status == RECOVERED) { person.make_well(series);  return; }
    if (new_status == DEAD)      { person.make_dead(series);   return; }
  }

  // No status routing: apply all terms directly
  for (const auto& t : chg.terms) {
    if      (t.trait == "cond")      person.cond()      = Condition{static_cast<uint8_t>(t.val)};
    else if (t.trait == "duration")  person.duration()  = t.val;
    else if (t.trait == "vaxstatus") person.vaxstatus() = Vaxstatus{static_cast<uint8_t>(t.val)};
    else if (t.trait == "quar")      person.quar()      = static_cast<uint8_t>(t.val);
  }
}

} // anonymous namespace


// SeedCase::operator(): find candidates via filter, apply change to up to change.count of them.
vector<size_t> SeedCase::operator()(HistorySeries& series) {
  vector<size_t> seeded;
  int matched = 0;
  for (size_t i = 1; i <= pop.popn && matched < change.count; ++i) {
    auto person = pop.agent(i);
    if (matches_filter(person, filter)) {
      apply_change(person, change, series);
      seeded.push_back(i);
      ++matched;
    }
  }
  if (seeded.empty())
    fmt::println("WARNING: no agents matched filter criteria for seedcase on triggerday {}", triggerday);
  return seeded;
}


vector<SeedCase> load_seed_cases(const json& jdata, PopData& pop, const ModelParams& mp) {
  vector<SeedCase> seedcases;
  for (const auto& sc : jdata) {
    int  triggerday  = sc["triggerday"];
    bool startofday  = sc.value("startofday", true);

    // --- parse filter terms ---
    Filter filt;
    for (const auto& t : sc["filter"]) {
      string trait = t["trait"].get<string>();
      if (!FILTER_TRAITS.count(trait))
        throw std::runtime_error("Disallowed filter trait: '" + trait + "'");
      filt.terms.push_back({trait, parse_term_val(trait, t["val"], mp)});
    }

    // --- parse change terms + count ---
    Change chg;
    chg.count = sc["change"]["count"].get<int>();
    for (const auto& t : sc["change"]["terms"]) {
      string trait = t["trait"].get<string>();
      if (!CHANGE_TRAITS.count(trait))
        throw std::runtime_error("Disallowed change trait: '" + trait + "'");
      chg.terms.push_back({trait, parse_term_val(trait, t["val"], mp)});
    }

    seedcases.emplace_back(triggerday, startofday, std::move(filt), std::move(chg), pop);
  }
  return seedcases;
}

void runsim(Model& model, vector<SeedCase> seedcases) {
  ModelParams& mp = model.mp;  // all disease, vaccine, social parameters
  PopData &pop = model.pop;    // all person data

  // seed the random number generator
  xo::seed(99999);  // have used 12345

  // create vector set for series statistics
  HistorySeries series(model.ndays, pop);

  // reset day counter to zero
  sim::reset_day();
  sim::debug = model.debug;

  // setup timers for performance metering
  Timing spread_timing;
  Timing progression_timing;
  Timing history_timing;
  Timing vax_timing;


  // create useful pre-allocated vectors
  vector<size_t> contacts(250); // reserve and set size, cleared before later usage

  // override dovax=true if any vax parameters within ModelParameters instance mp are empty
    if (model.dovax) { 
      if ((Vax::names.size() <= 1) | (mp.vaxset.size() == 0) | (mp.vaxschedset.size() == 0))
            model.dovax = false;
    }

  // access density factor for current locale
  auto locale_pos = find(mp.geodata.fips.begin(), mp.geodata.fips.end(), model.locale);
  if (locale_pos == mp.geodata.fips.end()) {
    throw std::runtime_error("Invalid locale input: " + std::to_string(model.locale) + ". Must match a locale from geodata.");
  }
  auto locale_idx = locale_pos - mp.geodata.fips.begin();

  float density_factor = mp.geodata.density[locale_idx];

  fmt::print("\n");


  // day loop
  for (int d_i = 1; d_i <= model.ndays; ++d_i) {
    // start a new day
    sim::incr_day();
    sim::ds.day = sim::get_day();
    series.init_history_series(d_i);

    // run beginning of day seed cases
    for (auto& sc : seedcases)
      if (sc.startofday && sc.triggerday == sim::ds.day)
        sc(series);

    // do vaccination if using vaccination
    if (model.dovax) {
      vax_timing.start();
      vaccinate(sim::get_day(),
               mp.vaxschedset,
               mp.vaxset,
               pop,
               series);
      vax_timing.cum();
    }


    // Loop through all people and process infectious ones (no vector allocation needed)
    for (size_t p = 1; p <= pop.popn; ++p) {

      // get an agent at index p
      auto person = pop.agent(p);
      if (person.status() != INFECTIOUS || person.sickday() >= sim::ds.day) continue;

      spread_timing.start();
      // spread kernel
      auto spr_duration = person.duration();      
      auto spr_variant = person.variant();    
      auto sendrisk = mp.infectparams[idx(spr_variant)].sendrisk[spr_duration];
      if (sendrisk > 0.0) {
        // sim::ds.starting_spreaders++;
        spread(pop, series, person, mp.socialdata, mp.infectparams, mp.vaxset,
               model.dovax, contacts, density_factor, model.indoor_seq);
      }
      spread_timing.cum();

      // progression kernel
      progression_timing.start();
      progression(person, series, mp.progressionset, mp.infectparams, mp.trvec, model.dovax, mp.vaxset);
      progression_timing.cum();

    } // end persons loop

    // update history series
    // history_timing.start();
    // update_series(pop, series);
    // history_timing.cum();

    if (d_i == 90) {
      std::vector<size_t> rows;
      for (size_t p = 1; p <= pop.popn; ++p) {
        auto person = pop.agent(p);
        if (person.status() == RECOVERED && person.agegrp() == AGE80_UP) {
          rows.push_back(p);
          if (rows.size() == 20) break;
        }
      }
      print_agent_pop_table(pop, rows, {"status", "agegrp", "cond", "duration"});
    }


    // Print daily outcomes
    // fmt::println("Day {:4}: spreaders: {:6}, contacts: {:7}, touched: {:7}, newly infected: {:6}, recovered: {:6}, died: {:5}",
    //              sim::ds.day, sim::ds.starting_spreaders, sim::ds.num_contacts, sim::ds.num_touched,
    //              sim::ds.num_new_infected, sim::ds.num_new_recovered, sim::ds.num_died);

    // run end of day cases

    // cleanup sim::ds
    // sim::ds.reset();

  } // day loop

  history_timing.start();
  series.finalize_series();
  history_timing.cum();

  //
  // at end of simulation
  // 

  std::vector<size_t> reinfected_rows;
  for (size_t p = 1; p <= pop.popn; ++p) {
    if (pop.sickday_hist[p].count > 1) reinfected_rows.push_back(p);
  }
  print_agent_pop_table(pop, reinfected_rows, {"status", "agegrp", "sickday_hist"});


  // print some series and a summary
  // print_selected_series({ {"now_infected", "total"},
  //                         {"new_infected", "total"},
  //                         {"new_recovered", "total"},
  //                         {"new_dead", "total"} },
  //                       series);

  // write the some series columns to csv
  serialize_selected_series(
    { {"now_unexposed", "total"}, {"now_infected","total"},{"now_recovered", "total"},{"now_dead","total"},
                                  {"new_infected","total"},{"new_recovered", "total"},{"new_dead","total"}}, 
    series, "test_series", {"code", "epi_sim", "series_output"});

  SummaryData sumstruct = print_summary(pop);

  fmt::println("Spread time: {} Progression time: {} History time: {} Vaccination time: {}", 
        spread_timing.show(), progression_timing.show(), history_timing.show(), vax_timing.show());

  if (!model.dovax) 
    seriesplot({{"now_infected", "total"},
              {"now_unexposed", "total"}, 
              {"now_recovered", "total"}, 
              {"now_dead", "total"}}, 
              series, model.caldays, sumstruct, "Cumulative Covid Outcome");
  else
      seriesplot({{"now_infected", "total"},
              {"now_unexposed", "total"}, 
              {"now_recovered", "total"}, 
              {"now_dead", "total"}, 
              {"now_vaccinated", "total"}},
              series, model.caldays, sumstruct, "Cumulative Covid Outcome");

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  seriesplot({{"now_dead", "age0_19"},
          {"now_dead", "age20_39"}, 
          {"now_dead", "age40_59"}, 
          {"now_dead", "age60_79"},
          {"now_dead", "age80_up"}}, 
          series, model.caldays, sumstruct, "Cumulative Died by Age Group", true);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  seriesplot({{"new_infected", "total"}},
          // {"now_dead", "age20_39"}, 
          // {"now_dead", "age40_59"}, 
          // {"now_dead", "age60_79"},
          // {"now_dead", "age80_up"}}, 
          series, model.caldays, sumstruct, "New Infection Cases", false);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  seriesplot({{"new_dead", "total"}},
          // {"now_dead", "age20_39"}, 
          // {"now_dead", "age40_59"}, 
          // {"now_dead", "age60_79"},
          // {"now_dead", "age80_up"}}, 
          series, model.caldays, sumstruct, "Daily Deaths", false);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  seriesplot({{"net_infected", "total"},
          {"now_infected", "total"}, 
          {"new_infected", "total"}}, 
          // {"now_dead", "age60_79"},
          // {"now_dead", "age80_up"}}, 
          series, model.caldays, sumstruct, "Compare Infection Metrics", false);
        
} // end runsim function


// Breakdown by age group: unexposed, infected, reinfected, dead, recovered
SummaryData print_summary(PopData & pop)
{
    const size_t n_ages = 5;  // Age0_19..Age80_up (indices 1..5)

    SummaryData sd;  // index 1..5 = age groups, index 6 = total, index 0 unused

    // no sorting or filtering needed
    for (size_t p = 1; p <= pop.popn; ++p) {
      uint8_t ag = pop.agegrp[p];
      if (pop.status[p] == UNEXPOSED) sd.unexposed[ag]++;
      if (pop.variant_hist[p].count > 0)  sd.infected[ag]++;
      if (pop.variant_hist[p].count > 1)  sd.reinfected[ag]++;
      if (pop.status[p] == DEAD) sd.dead[ag]++;
      if (pop.status[p] == RECOVERED) sd.recovered[ag]++;
    }

    auto ages = [](auto& arr) { return std::span(arr).subspan(1, 5); };
    sd.unexposed[6]  = sum(ages(sd.unexposed));
    sd.infected[6]   = sum(ages(sd.infected));
    sd.reinfected[6] = sum(ages(sd.reinfected));
    sd.recovered[6]  = sum(ages(sd.recovered));
    sd.dead[6]       = sum(ages(sd.dead));

    fmt::println("\n{:<12} {:>11} {:>10} {:>12} {:>11} {:>8} {:>10}",
                 "Age Group", "Unexposed","Infected", "Reinfected", "Recovered", "Dead", "Death %");
    fmt::println("{:-<80}", "");
    for (size_t ag = 1; ag <= n_ages; ++ag) {
      double death_pct = sd.infected[ag] > 0 ? 100.0 * sd.dead[ag] / sd.infected[ag] : 0.0;
      fmt::println("{:<12} {:>11} {:>10} {:>12} {:>11} {:>8} {:>9.2f}%",
                  Agegrp::names[ag], sd.unexposed[ag], sd.infected[ag], sd.reinfected[ag], sd.recovered[ag], sd.dead[ag], death_pct);
    }

    double total_death_pct = sd.infected[6] > 0 ? 100.0 * sd.dead[6] / sd.infected[6] : 0.0;
    fmt::println("{:<12} {:>11} {:>10} {:>12} {:>11} {:>8} {:>9.2f}%", "Total",
                 sd.unexposed[6], sd.infected[6], sd.reinfected[6], sd.recovered[6], sd.dead[6], total_death_pct);
    fmt::println("(Note: Remaining still infected across all ages: {})",
                 std::count_if(pop.status.begin(), pop.status.end(),
                              [](auto s) { return s == INFECTIOUS; }));
    return sd;
}
