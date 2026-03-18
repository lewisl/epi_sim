
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




// TODO later add parameters for runcases, showr0, silent, dovax?, vaxscheds?
void runsim(Model& model)
    // other arguments to add: runcases, showr0, silent, dovax, vaxscheds?
{
  ModelParams& mp = model.mp;  // all disease, vaccine, social parameters
  PopData &pop = model.pop;    // all person data

  // seed the random number generator
  xo::seed(12345);  // have used 12345

  // create vector set for series statistics
  DayData series(model.ndays);

  // reset day counter to zero
  sim::reset_day();

  // setup timers for performance metering
  // sprtime
  Timing spread_timing;
  // trtime
  Timing progression_timing;
  // histtime
  Timing history_timing;
  // totaltime

  // create SeedCases
  vector<SeedFilter> sf {
    // agegrp, cond, duration, variant, count
    {Age::Age20_39, Cond::Nil, 0, model.mp.variants[1], 3},
    {Age::Age40_59, Cond::Nil, 0, model.mp.variants[1], 3}};
  SeedCase sc1(1, true, sf, pop);

  // create useful pre-allocated vectors
  vector<size_t> contacts(
      250); // reserve and set size, cleared before later usage



  // access density factor for current locale
  auto locale_pos = find(mp.geodata.fips.begin(), mp.geodata.fips.end(), model.locale);
  if (locale_pos == mp.geodata.fips.end()) {
    throw std::runtime_error("Invalid locale input: " + std::to_string(model.locale) + ". Must match a locale from geodata.");
  }
  auto locale_idx = locale_pos - mp.geodata.fips.begin();

  float density_factor = mp.geodata.density[locale_idx];

  fmt::print("\n");

  // start totaltime

  

  // day loop
  for (int d_i = 1; d_i <= model.ndays; ++d_i) {
    // start a new day
    sim::incr_day();
    sim::ds.day = sim::get_day();

    // run beginning of day cases--required SeedCases
    if (sc1.startofday && sc1.triggerday == sim::ds.day) {
      sc1(series);
    }

    // do vaccination if using vaccination

    // Loop through all people and process infectious ones (no vector allocation needed)
    for (size_t p = 1; p <= pop.popn; ++p) {

      // get an agent at index p
      auto person = pop.agent(p);
      if (person.status() != Stat::Infectious) continue;

      spread_timing.start();
      // spread kernel
      auto spr_duration = person.duration();       //pop.duration[p];  // a uint8_t
      auto variant_count = person.variant_count();
      if (variant_count == 0) continue;  // Skip if no variant assigned:  TODO this is really an error that shouldn't happen
      auto spr_variant = person.get_variant();    // variant_count is 1-based, array is 0-based
      auto sendrisk = mp.infectparams[idx(spr_variant)].sendrisk[idx(spr_duration)];
      if (sendrisk > 0.0) {
        sim::ds.starting_spreaders++;
        spread(pop, series, p, mp.socialdata, mp.infectparams, contacts, density_factor, model.indoor_seq);
      }
      spread_timing.cum();

      // progression kernel
      progression_timing.start();
      progression(pop, p, series, mp.progressionset, mp.infectparams, mp.trvec, model.dovax, mp.vaxset);
      progression_timing.cum();

      // cleanup before next person
      // contacts.clear();  // get rid of all the contacts of the previous person!
    } // end persons loop

    // update history series
    history_timing.start();
    update_series(pop, series);
    history_timing.cum();

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
  // print_total_status_series(series);
  print_selected_series({"now_infected", "new_infected", "new_recovered", "new_dead"}, series);

  // Breakdown by age group: infected, reinfected, dead
  {
    // const auto& pop = model.pop;
    const size_t n_ages = 5;  // Age0_19..Age80_up (indices 1..5)

    array<int, 6> unexposed{}, infected{}, reinfected{}, recovered{}, dead{};  // index 1..5

    for (size_t p = 1; p <= pop.popn; ++p) {
      uint8_t ag = pop.agegrp[p];
      if (pop.status[p] == Stat::Unexposed) unexposed[ag]++;
      if (pop.variant_count[p] > 0)  infected[ag]++;
      if (pop.variant_count[p] > 1)  reinfected[ag]++;
      if (pop.status[p] == Stat::Dead) dead[ag]++;
      if (pop.status[p] == Stat::Recovered) recovered[ag]++;
    }
    fmt::println("\n{:<12} {:>11} {:>10} {:>12} {:>11} {:>8} {:>10}",
                 "Age Group", "Unexposed","Infected", "Reinfected", "Recovered", "Dead", "Death %");
    fmt::println("{:-<80}", "");
    for (size_t ag = 1; ag <= n_ages; ++ag) {
      double death_pct = infected[ag] > 0 ? 100.0 * dead[ag] / infected[ag] : 0.0;
      fmt::println("{:<12} {:>11} {:>10} {:>12} {:>11} {:>8} {:>9.2f}%",
                   Agegrp::names[ag], unexposed[ag], infected[ag], reinfected[ag], recovered[ag], dead[ag], death_pct);
    }

    fmt::println("{:<12} {:>11} {:>10} {:>12} {:>11} {:>8} ", "Total",
                 sum(unexposed), sum(infected), sum(reinfected), sum(recovered),
                 sum(dead)); // using sum template in helpers.h for reduce
    fmt::println("(Note: Remaining still infected across all ages: {})",
                 std::count_if(pop.status.begin(), pop.status.end(), [](auto s) { return s == Stat::Infectious; }));
  }

  fmt::println("Spread time: {} Transition time: {} History time: {}", spread_timing.show(), progression_timing.show(), history_timing.show());

  // fmt::println("\n=== Simulation Complete ===");
} // end runsim function
