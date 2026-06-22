#include <algorithm>

#include "parameters.h"
#include "population.h"
#include "r0_simulation.h"
#include "progression.h"
#include "ring_traits.h"
#include "spread.h"
#include "setup.h"
#include "traits.h"
#include <fmt/base.h>
#include <fmt/format.h> // only get what I use: about 12k in the executable!
#include <fmt/ranges.h> // for printing containers like vector
#include <fmt/ostream.h> // to use ostream file handles and << >> operators

// forward declaration
namespace {
  float run_r0_sim(Model & model, PopData & r0pop, Variant variant, int scale);
}

/*
    r0_sim(popsize, age_dist, progressionset, trvec, infectset, vaxset, 
                socialparams, density_factor=1.0, scale=3)

Simulates r0 or rt. The first overload creates a population and tracks how many infections
are caused by first generation spreaders and NOT spreaders who were infected by the
first generation (or later). 
*/

float r0_sim(Model & model) {
  
  int scale = 3;
  Variant use_variant{1};  // this will be the required base variant
  int popn = 200'000; 
 
  // synthesized population
  PopData r0_pop = PopData(popn, AGE_DIST);

  return run_r0_sim(model, r0_pop, use_variant, scale);
}


/*
    r0_sim(locdat, age_dist, progressionset, trvec, infectset, vaxset, 
                socialparams, density_factor, scale)

The second overload simulates r at time t given the characteristics of the simulation
you are running. This shows how r is affected by public health
measures and the characteristics of the population over time based on your simulation
parameters. This simulates r(t).

It will be called as part of the simulation run in function runsim so that it uses the
context of the current simulation with its input parameters.
*/
float r0_sim(PopData locdat, Model & model) {
  
    // create the simulation population--> this is all we have to do in the front end function
    PopData r0pop {locdat};
    int scale = 3;
    Variant use_variant = 1;

    return run_r0_sim(model, r0pop, use_variant, scale);
  }

namespace{

float run_r0_sim(Model & model, PopData & r0pop, Variant variant, int scale) {

    float r0{};   // return value

    // deref or create arguments needed for spread
    int locale = model.locale;
    bool dovax = model.dovax;         
    VaxSet & vaxset = model.mp.vaxset;  
    ProgressionSet & progressionset = model.mp.progressionset;
    vector<InfectParams> & infectparams = model.mp.infectparams;  
    SocialParams & socialparams = model.mp.socialdata;
    AllSeries r0series(DURATIONLIM, r0pop, Variant::names.size(), 1, 1);  // not used but required by the spread and progression APIs
    vector<size_t> contacts(250); // reserve and set size, cleared before later usage
    array<float, 6> probvec{};

    // access density factor for current locale
    auto locale_pos = find(model.mp.geodata.fips.begin(), model.mp.geodata.fips.end(), locale);
    if (locale_pos == model.mp.geodata.fips.end()) {
      throw std::runtime_error("Invalid locale input: " + std::to_string(locale) + ". Must match a locale from geodata.");
    }
    auto locale_idx = locale_pos - model.mp.geodata.fips.begin();

    float density_factor = model.mp.geodata.density[locale_idx];


    // seed spreaders in each age group proportional to age distribution
    array<int, 5> cnt_by_agedist;  // TODO should extract the count of AGE_DIST and not assume it
    uint8_t i=0;
    int remaining_seeds = 0;  // decrement counter for seeding loop below
    float min_age_dist = std::ranges::min(AGE_DIST);
    for (auto val : AGE_DIST) {
      cnt_by_agedist[i] = val / (min_age_dist * scale); // bad boy: implicit conversion
      remaining_seeds += cnt_by_agedist[i];
      i++;
      }


    for (size_t p = 1; p <= r0pop.popn; ++p) {
      auto person = r0pop.agent(p);
      switch (person.agegrp()) 
      {
        case AGE0_19: {
          if (cnt_by_agedist[0] > 0) {
            person.make_sick(variant,r0series); 
            --cnt_by_agedist[0]; 
            --remaining_seeds; 
            break;}
          else break;
          }
        case AGE20_39: {
          if (cnt_by_agedist[1] > 0) {
            person.make_sick(variant,r0series); 
            --cnt_by_agedist[1]; 
            --remaining_seeds;
            break;}
          else break;
          }
        case AGE40_59: {
          if (cnt_by_agedist[2] > 0) {
            person.make_sick(variant,r0series); 
            --cnt_by_agedist[2]; 
            --remaining_seeds;
            break;}
          else break;
          }
        case AGE60_79: {
          if (cnt_by_agedist[3] > 0) {
            person.make_sick(variant,r0series); 
            --cnt_by_agedist[3]; 
            --remaining_seeds;
            break;}
          else break;
          }
        case AGE80_UP: {
          if (cnt_by_agedist[4] > 0) {
            person.make_sick(variant,r0series); 
            --cnt_by_agedist[4]; 
            --remaining_seeds;
            break;}
          else break;
          }
        default:
          throw std::runtime_error("Invalid agegrp for seeding r0_simulation.");
      }  
    if (remaining_seeds == 0) break;
    }

  // set infect_idx based on seeding: never update so measure only 1st gen spreaders
  // slower than within loop testing, but this is a short run and this makes it
  // easier to limit spreaders to the initial seeding
  vector<size_t> gen1_spreaders{};
  for (size_t p = 1; p <= r0pop.popn; ++p) {
    auto person = r0pop.agent(p);
    if (person.status() == INFECTIOUS) {
      gen1_spreaders.push_back(p);
      }
    }
  auto gen1_infect_idx{gen1_spreaders};   // copy
  auto gen1_spreader_cnt = gen1_spreaders.size();
  int r0_infected = 0;
  int cnt_new_infected = 0;
  vector<float> indoor_seq(DURATIONLIM, 1.0f);  // assume no seasonality for abstract r0 simulation
  vector<SocialDistancing> empty_sdcases{};
  RingTraits empty_ringtraits{};
  std::vector<std::vector<size_t>> empty_ringmembers{};
  std::vector<size_t> empty_ringlengths{};

  //
  // mini simulation loop
  //
  for (size_t d=1; d <= DURATIONLIM; ++d) {   // day loop
    // start a new day
    sim::incr_day();
    sim::ds.day = sim::get_day();

    // loop over gen 1 spreaders
    for (auto i : gen1_infect_idx ) {
      auto person = r0pop.agent(i);
      // fmt::println("person's status: {}", person.status().show());   // OK
      cnt_new_infected = spread(r0pop, r0series, person, socialparams, infectparams,
            vaxset, dovax, contacts, density_factor, indoor_seq,
            empty_sdcases, empty_ringtraits, empty_ringmembers, empty_ringlengths);
      r0_infected += cnt_new_infected;
      // fmt::println("cnt_nex_infected on day {}: {}", d, cnt_new_infected);
      }

    // loop over infectious persons for progression  
    for (size_t p = 1; p <= r0pop.popn; ++p) {
      auto person = r0pop.agent(p);
      if (person.status() == INFECTIOUS)
          progression(person, r0series, progressionset, infectparams, probvec, dovax, vaxset);
      }

      // remove gen1 spreaders who have progressed out of infectious
      std::erase_if(gen1_infect_idx, 
        [&r0pop](size_t p) {auto person = r0pop.agent(p);
                return (person.status() != INFECTIOUS);}
        );

  }
      r0 = static_cast<float>(r0_infected) / static_cast<float>(gen1_spreader_cnt);

  return r0;
}
}  // end namespace