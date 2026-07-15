
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


void runsim(Model& model) {  // vector<SeedCase>& seedcases, vector<SocialDistancing>& sd_cases
  ModelParams& mp = model.mp;  // all disease, vaccine, social parameters
  PopData &pop = model.pop;    // all person data
  vector<SeedCase>& seedcases = model.seedcases;
  vector<SocialDistancing> & sd_cases = model.sd_cases;
  vector<double> & age_dist = model.age_dist;

  // seed the random number generator
  xo::seed(99999);  // have used 12345

  // create vector set for series statistics.
  // Ring slot count: max(Ring::names.size(), 1). With no rings defined
  // Ring::names is empty -> 1 slot (index 0); with N rings defined the
  // size is N+1 (sentinel + names) -> N+1 slots, index 0 = RING_ALL.
  size_t n_ring_slots = std::max<size_t>(Ring::names.size(), 1);
  AllSeries series(model.ndays, pop, Variant::names.size(),
                   Vax::names.size(), n_ring_slots);

  // reset day counter to zero
  sim::reset_day();
  sim::debug = model.debug;
  sim::history_timing.reset();

  // setup timers for performance metering
  Timing spread_timing;
  Timing progression_timing;
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
      if (sc.startofday && sc.triggerday == sim::ds.day) {
        auto seeds = sc(pop, series);
        std::string filt;
        for (const auto& t : sc.filter.terms)
          filt += fmt::format("{}{}={}", filt.empty() ? "" : ",", t.trait, t.val);
        std::string chg;
        for (const auto& t : sc.change.terms)
          chg += fmt::format("{}{}={}", chg.empty() ? "" : ",", t.trait, t.val);
        fmt::println("Seed day {} count: {} filter: [{}] change: [{}]", d_i, seeds.size(), filt, chg);
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
               series);
      vax_timing.cum();
    }
    if (model.rt_sim_interval) {
    if (d_i % model.rt_sim_interval == 0) {
      rt_sim(pop, model);
    }}

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
               model.dovax, contacts, density_factor, model.indoor_seq, sd_cases,
               mp.ringtraits, model.ring_members, model.ring_lengths);
      }
      spread_timing.cum();

      // progression kernel
      progression_timing.start();
      progression(person, series, mp.progressionset, mp.infectparams, mp.trvec, model.dovax, mp.vaxset);
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

  } // day loop

  sim::history_timing.start();
  series.finalize_series();
  // debugging only: series.validate_variant_invariant();
  sim::history_timing.cum();

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

  // print some series and a summary
  // print_selected_series({ {"now_infectious", "total"},
  //                         {"new_infectious", "total"},
  //                         {"new_recovered", "total"},
  //                         {"new_dead", "total"} },
  //                       series);

  // write series + PopData columns to csv (skipped in headless runs)
  const std::string output_timestamp = model.headless ? std::string{} : make_timestamp_token();
  if (!model.headless) {
    if (model.output_dir.empty()) {
      throw std::runtime_error("Model output_dir is not configured.");
    }
    std::filesystem::create_directories(model.output_dir);

    serialize_selected_series(
      {{"now_infectious", "total"},
      {"new_infectious", "total"},
      {"new_dead",       "total"},
      {"now_dead",       "total"}},
      series, case_artifact_path(model, "series", output_timestamp, "csv"));

    pop_to_csv(pop, pop.all_idx, "all",
               OutSpec(case_artifact_path(model, "pop", output_timestamp, "csv")));
  }

  SummaryData sumstruct = print_summary(pop); fmt::println("");

  fmt::println("Spread time: {} Progression time: {} History time: {} Vaccination time: {}", 
        spread_timing.show(), progression_timing.show(), sim::history_timing.show(), vax_timing.show());

  if (model.headless) return;  // headless runs skip browser plots

  if (!model.dovax)
    seriesplot({{"now_infectious", "total"},
              {"now_unexposed", "total"},
              {"now_recovered", "total"},
              {"now_dead", "total"}},
              series, model.caldays, sumstruct, "Cumulative Covid Outcome", false,
              case_artifact_path(model, "Cumulative Covid Outcome", output_timestamp, "html"));
  else
      seriesplot({{"now_infectious", "total"},
              {"now_unexposed", "total"},
              {"now_recovered", "total"},
              {"now_dead", "total"},
              {"now_vaccinated", "total"}},
              series, model.caldays, sumstruct, "Cumulative Covid Outcome", false,
              case_artifact_path(model, "Cumulative Covid Outcome", output_timestamp, "html"));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  seriesplot({{"now_dead", "age0_19"},
          {"now_dead", "age20_39"}, 
          {"now_dead", "age40_59"}, 
          {"now_dead", "age60_79"},
          {"now_dead", "age80_up"}}, 
          series, model.caldays, sumstruct, "Cumulative Died by Age Group", true,
          case_artifact_path(model, "Cumulative Died by Age Group", output_timestamp, "html"));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  seriesplot({{"new_infectious", "total"}},
          // {"now_dead", "age20_39"},
          // {"now_dead", "age40_59"},
          // {"now_dead", "age60_79"},
          // {"now_dead", "age80_up"}},
          series, model.caldays, sumstruct, "New Infection Cases", false,
          case_artifact_path(model, "New Infection Cases", output_timestamp, "html"));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  seriesplot({{"new_dead", "total"}},
          // {"now_dead", "age20_39"}, 
          // {"now_dead", "age40_59"}, 
          // {"now_dead", "age60_79"},
          // {"now_dead", "age80_up"}}, 
          series, model.caldays, sumstruct, "Daily Deaths", false,
          case_artifact_path(model, "Daily Deaths", output_timestamp, "html"));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // net_infected removed (Step 1 refactor)
        
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
