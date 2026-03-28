
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


// TODO later add parameters for runcases, showr0, silent, dovax?, vaxscheds?
vector<SeedCase> load_seed_cases(const json& jdata, PopData& pop, const ModelParams& mp) {
  vector<SeedCase> seedcases;
  for (const auto& sc : jdata) {
    int triggerday    = sc["triggerday"];
    bool startofday   = sc["startofday"];
    vector<SeedFilter> filtervec;
    for (const auto& f : sc["filtervec"]) {
      auto agegrp_opt = trait_from_string<Agegrp>(f["agegrp"].get<string>());  // uses return type option
      if (!agegrp_opt) {   
        fmt::println("WARNING: unknown agegrp '{}' in seed file, skipping filter", f["agegrp"].get<string>());
        continue;
      }
      auto cond_opt = trait_from_string<Condition>(f["condition"].get<string>());
      if (!cond_opt) {
        fmt::println("WARNING: unknown condition '{}' in seed file, skipping filter", f["condition"].get<string>());
        continue;
      }
      string variant_str = f["variant"].get<string>();
      auto var_iter = std::find_if(mp.variants.begin(), mp.variants.end(),
                              [&](const Variant& v) { return v.name() == variant_str; });
      if (var_iter == mp.variants.end()) {
        fmt::println("WARNING: unknown variant '{}' in seed file, skipping filter", variant_str);
        continue;
      }
      filtervec.push_back({*agegrp_opt, *cond_opt,
                           f["duration"].get<uint8_t>(), *var_iter,
                           f["count"].get<int8_t>()});
    }
    seedcases.emplace_back(triggerday, startofday, std::move(filtervec), pop);
  }
  return seedcases;
}

void runsim(Model& model, vector<SeedCase> seedcases) {
  ModelParams& mp = model.mp;  // all disease, vaccine, social parameters
  PopData &pop = model.pop;    // all person data

  // seed the random number generator
  xo::seed(99999);  // have used 12345

  // create vector set for series statistics
  DayData series(model.ndays);

  // reset day counter to zero
  sim::reset_day();

  // setup timers for performance metering
  Timing spread_timing;
  Timing progression_timing;
  Timing history_timing;
  Timing vax_timing;


  // create useful pre-allocated vectors
  vector<size_t> contacts(250); // reserve and set size, cleared before later usage

  // override dovax=true if any vax parameters within ModelParameters instance mp are empty
    if (model.dovax) { 
      if ( (mp.vaxlist.size() == 0) | (mp.vaxset.vaxset.size() == 0) | (mp.vaxsched.vaxesincluded.size() == 0))
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

    // run beginning of day seed cases
    for (auto& sc : seedcases)
      if (sc.startofday && sc.triggerday == sim::ds.day)
        sc(series);

    // do vaccination if using vaccination
    if (model.dovax) {
      vax_timing.start();
      vaccinate(sim::get_day(),
               mp.vaxsched,
               mp.vaxset,
               mp.vaxlist,
               pop);
      vax_timing.cum();
    }


    // Loop through all people and process infectious ones (no vector allocation needed)
    for (size_t p = 1; p <= pop.popn; ++p) {

      // get an agent at index p
      auto person = pop.agent(p);
      if (person.status() != Stat::Infectious || person.get_sickday() >= sim::ds.day) continue;

      spread_timing.start();
      // spread kernel
      auto spr_duration = person.duration();      
      auto spr_variant = person.get_variant();    
      auto sendrisk = mp.infectparams[idx(spr_variant)].sendrisk[spr_duration];
      if (sendrisk > 0.0) {
        sim::ds.starting_spreaders++;
        spread(pop, series, person, mp.socialdata, mp.infectparams, contacts, density_factor, model.indoor_seq);
      }
      spread_timing.cum();

      // progression kernel
      progression_timing.start();
      progression(person, series, mp.progressionset, mp.infectparams, mp.trvec, model.dovax, mp.vaxset);
      progression_timing.cum();

    } // end persons loop

    // update history series
    history_timing.start();
    update_series(pop, series);
    history_timing.cum();

    if (d_i == 90) {
      std::vector<size_t> rows;
      for (size_t p = 1; p <= pop.popn; ++p) {
        auto person = pop.agent(p);
        if (person.status() == Stat::Infectious && person.agegrp() == Age::Age80_up) {
          rows.push_back(p);
          if (rows.size() == 20) break;
        }
      }
      print_agent_pop_table(pop, rows, {"status", "agegrp", "cond", "duration"});
    }


    // Print daily outcomes
    // fmt::println("Day {:4}: spreaders: {:6}, contacts: {:7}, touched: {:7}, newly infected: {:6}, recovered: {:6}, died: {:5}",
    //              sim::ds.day, sim::ds.starting_spreaders, sim::ds.num_contacts, sim::ds.num_touched,
    //              sim::ds.num_new_infected, sim::ds.num_recovered, sim::ds.num_died);

    // run end of day cases

    // cleanup sim::ds
    sim::ds.reset();

  } // day loop

  finalize_series(series);
 
  //
  // at end of simulation
  // 


  // print some series and a summary
  print_selected_series({ {"now_infected", "total"},
                          {"new_infected", "total"},
                          {"new_recovered", "total"},
                          {"new_dead", "total"} },
                        series);

  SummaryData sumstruct = print_summary(pop);

  fmt::println("Spread time: {} Progression time: {} History time: {} Vaccination time: {}", 
        spread_timing.show(), progression_timing.show(), history_timing.show(), vax_timing.show());

  seriesplot({{"now_infected", "total"},
            {"now_unexposed", "total"}, 
            {"now_recovered", "total"}, 
            {"now_dead", "total"}}, 
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
      if (pop.status[p] == Stat::Unexposed) sd.unexposed[ag]++;
      if (pop.variant_count[p] > 0)  sd.infected[ag]++;
      if (pop.variant_count[p] > 1)  sd.reinfected[ag]++;
      if (pop.status[p] == Stat::Dead) sd.dead[ag]++;
      if (pop.status[p] == Stat::Recovered) sd.recovered[ag]++;
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
                              [](auto s) { return s == Stat::Infectious; }));
    return sd;
}
