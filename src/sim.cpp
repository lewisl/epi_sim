#include <thread>
#include "cases.h"
#include "helpers.h"
#include "parameters.h"
#include "population.h"
#include "random.h"
#include "sim.h"
#include "setup.h"
#include "spread.h"
#include "progression.h"
#include "timing.h"
#include "series.h"
#include "plot.h"
#include "pop_serialize.h"
#include "vaccination.h"
#include "r0_simulation.h"

// forward declarations
SummaryData print_summary(PopData & pop);

namespace {

std::filesystem::path case_artifact_path(const Model& model,
                                         std::string_view artifact,
                                         std::string_view timestamp,
                                         std::string_view extension) {
  const std::string case_label = sanitize_filename_component(model.case_label);
  const std::string artifact_label = sanitize_filename_component(artifact);
  return model.output_dir / fmt::format("{}_{}_{}.{}", case_label, artifact_label,
                                        timestamp, extension);
}

}  // namespace


Histories runsim(Model& model) {  // vector<SeedCase>& seedcases, vector<SocialDistancing>& sd_cases
  ModelParams& mp = model.mp;  // all disease, vaccine, social parameters
  PopData &pop = model.pop;    // all person data
  vector<SeedCase>& seedcases = model.seedcases;
  vector<SocialDistancing> & sd_cases = model.sd_cases;
  vector<double> & age_dist = model.age_dist;
  install_runtime_trait_names(model);   // set names vectors for compile time traits
  // override dovax=true if any vax parameters within ModelParameters instance mp are empty
  if ((Vax::names.size() <= 1) | (mp.vaxset.size() == 0) | (mp.vaxschedset.size() == 0))
        model.dovax = false;

  // seed the random number generator
  xo::seed(99999);  // have used 12345

  const size_t real_variant_count = Variant::names.size() - 1;
  const size_t real_vax_count = !model.dovax || Vax::names.empty()
                              ? 0 : Vax::names.size() - 1;
  const size_t real_ring_count = !model.do_rings || Ring::names.empty()
                               ? 0 : Ring::names.size() - 1;
  Histories histories(model.ndays, pop, real_variant_count,
                      real_vax_count, real_ring_count);

  // reset day counter to zero
  sim::reset_day();
  sim::debug = model.debug;


  // setup timers for performance metering
  sim::history_timing.reset();
  Timing spread_timing;
  Timing progression_timing;
  Timing vax_timing;


  // create useful pre-allocated vectors
  vector<size_t> contacts(250); // reserve and set size, cleared before later usage


  // access density factor for current locale
  auto locale_pos = find(mp.geodata.fips.begin(), mp.geodata.fips.end(), model.locale);
  if (locale_pos == mp.geodata.fips.end()) {
    throw std::runtime_error("Invalid locale input: " + std::to_string(model.locale) + ". Must match a locale from geodata.");
  }
  auto locale_idx = locale_pos - mp.geodata.fips.begin();
  float density_factor = mp.geodata.density[locale_idx];

  
  //
  // day loop
  //
  for (int d_i = 1; d_i <= model.ndays; ++d_i) {
    // start a new day
    sim::incr_day();
    sim::ds.day = sim::get_day();

    sim::history_timing.start();
    histories.init_history_series(d_i);
    sim::history_timing.cum();

    // run beginning of day seed cases
    for (auto& sc : seedcases)
      if (sc.startofday && sc.triggerday == sim::ds.day) {
        auto seeds = sc(pop, histories);
        std::string filt;
        for (const auto& t : sc.filter.terms)
          filt += fmt::format("{}{}={}", filt.empty() ? "" : ",", t.trait, t.val);
        std::string chg;
        for (const auto& t : sc.change.terms)
          chg += fmt::format("{}{}={}", chg.empty() ? "" : ",", t.trait, t.val);
        fmt::println("\nSeed day {} count: {} filter: [{}] change: [{}]", d_i, seeds.size(), filt, chg);
      }

    // run social distancing cases
    apply_sd_cases_for_day(sim::ds.day, sd_cases, pop);

    // do vaccination if using vaccination
    if (model.dovax) {
      vax_timing.start();
      vaccinate(sim::get_day(),
               mp.vaxschedset,
               mp.vaxset,
               pop,
               histories);
      vax_timing.cum();
      }

    // run rt simulation if interval != 0  
    if (model.rt_sim_interval) {
    if (d_i % model.rt_sim_interval == 0) {
      rt_sim(pop, model);
    }}

    //
    // Loop through all people and process infectious ones (no vector allocation needed)
    //
    for (size_t p = 1; p <= pop.popn; ++p) {

      // get an agent at index p
      auto person = pop.agent(p);
      if (person.status() != INFECTIOUS || person.sickday() >= sim::ds.day) continue;

      // spread kernel
      spread_timing.start();
      auto spr_duration = person.duration();      
      auto spr_variant = person.variant();    
      auto sendrisk = mp.infectparams[idx(spr_variant)].sendrisk[spr_duration];
      if (sendrisk > 0.0) {
        // sim::ds.starting_spreaders++;
        spread(pop, histories, person, mp.socialdata, mp.infectparams, mp.vaxset,
               model.dovax, contacts, density_factor, model.indoor_seq, sd_cases,
               mp.ringtraits, model.ring_members, model.ring_lengths);
      }
      spread_timing.cum();

      // progression kernel
      progression_timing.start();
      progression(person, histories, mp.progressionset, mp.infectparams,
                  model.dovax, mp.vaxset);
      progression_timing.cum();

    } // end persons loop

    // simple debug print to console
    // if (d_i == 90) {
    //   std::vector<size_t> rows;
    //   for (size_t p = 1; p <= pop.popn; ++p) {
    //     auto person = pop.agent(p);
    //     if (person.status() == RECOVERED && person.agegrp() == AGE80_UP) {
    //       rows.push_back(p);
    //       if (rows.size() == 10) break;
    //     }
    //   }
    //   pop_print(pop, rows, {"status", "agegrp", "cond", "variant_hist"}, std::cout);
    // }


    // run end of day cases

    // cleanup sim::ds
    // sim::ds.reset();

  } // end day loop


  //
  // at end of simulation
  // 

  // a debug example to print people who have been reinfected
  // std::vector<size_t> reinfected_rows;
  // for (size_t p = 1; p <= pop.popn; ++p) {
  //   if (pop.sickday_hist[p].count > 1) reinfected_rows.push_back(p);
  //   if (reinfected_rows.size() == 10) break;
  // }
  // 
  // pop_print(pop, reinfected_rows, {"status", "agegrp", "sickday_hist", "variant_hist"}, std::cout);

  // print some histories and a summary
  // print_selected_histories({ {"now_infectious", "total"},
  //                         {"new_infectious", "total"},
  //                         {"new_recovered", "total"},
  //                         {"new_dead", "total"} },
  //                          histories);

  // write series + PopData columns to csv (skipped in headless runs)
  const std::string output_timestamp = model.headless ? std::string{} : make_timestamp_token();
  if (!model.headless) {
    if (model.output_dir.empty()) {
      throw std::runtime_error("Model output_dir is not configured.");
    }
    std::filesystem::create_directories(model.output_dir);

    serialize_selected_histories(
        {{"now", "infectious", "total"}, // select with strings
        {"new_", "infectious", "total"},
        {"new_", "dead",       "total"},
        {"now", "dead",       "total"}},
      histories, case_artifact_path(model, "series", output_timestamp, "csv"));

    pop_to_csv(pop, pop.all_idx, "all",
               OutSpec(case_artifact_path(model, "pop", output_timestamp, "csv")));
  }

  SummaryData sumstruct = print_summary(pop); fmt::println("");

  fmt::println("Spread time: {} Progression time: {} History time: {} Vaccination time: {}", 
        spread_timing.show(), progression_timing.show(), sim::history_timing.show(), vax_timing.show());

  if (model.headless) return histories;  // headless runs skip browser plots


  //
  // create and output plots
  //
  if (!model.dovax)
    historyplot(
                // select histories using an initializer list of strings
                {{"now", "infectious", "total"},
                {"now", "unexposed", "total"},
                {"now", "recovered", "total"},
                {"now", "dead", "total"}},
            histories, model.caldays, sumstruct, "Cumulative Covid Outcome", false,
            case_artifact_path(model, "Cumulative Covid Outcome", output_timestamp, "html"));
  else
    historyplot(
                // select histories using an initializer list of strings
                {{"now", "infectious", "total"},
                {"now", "unexposed", "total"},
                {"now", "recovered", "total"},
                {"now", "dead", "total"},
                {"now", "vaccinated", "total"}},
            histories, model.caldays, sumstruct, "Cumulative Covid Outcome", false,
            case_artifact_path(model, "Cumulative Covid Outcome", output_timestamp, "html"));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  historyplot({{"now", "dead", "age0_19"},
          {"now", "dead", "age20_39"}, 
          {"now", "dead", "age40_59"}, 
          {"now", "dead", "age60_79"},
          {"now", "dead", "age80_up"}}, 
          histories, model.caldays, sumstruct, "Cumulative Died by Age Group", true,
          case_artifact_path(model, "Cumulative Died by Age Group", output_timestamp, "html"));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  historyplot({{"new_", "infectious", "total"}},
          histories, model.caldays, sumstruct, "New Infection Cases", false,
          case_artifact_path(model, "New Infection Cases", output_timestamp, "html"));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  historyplot({{"new_", "dead", "total"}},
          histories, model.caldays, sumstruct, "Daily Deaths", false,
          case_artifact_path(model, "Daily Deaths", output_timestamp, "html"));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // output of the simulation captured in terminal app
  return histories;


        
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
                 std::count_if(pop.status.begin() + 1, pop.status.end(),
                              [](auto s) { return s == INFECTIOUS; }));
    return sd;
}
