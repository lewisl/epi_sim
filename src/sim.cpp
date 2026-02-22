
#include "lib_includes.h"

#include "parameters.h"
#include "population.h"
#include "random.h"
#include "sim.h"
#include "setup.h"
#include "disease_modeling.h"
#include "spread.h"
#include "progression.h"




// TODO later add parameters for runcases, showr0, silent, dovax?, vaxscheds?
void runsim(Model& model)
    // other arguments to add: runcases, showr0, silent, dovax, vaxscheds?
{
  ModelParams& mp = model.mp;  // all disease, vaccine, social parameters
  PopData &pop = model.pop;    // all person data

  // seed the random number generator
  xo::seed(12345);

  // setup before day loop starts
  //    alias names for series columns

  // reset day counter to zero
  sim::reset_day();

  // setup timers for performance metering
      // sprtime
      // trtime
      // histtime
  // totaltime

  // create SeedCases
  vector<SeedFilter> sf {
    {Trait::Age::age20_39, Trait::Cond::nil, 5, model.mp.variants("base"), 3},
    {Trait::Age::age40_59, Trait::Cond::nil, 5, model.mp.variants("base"), 3}};
  SeedCase sc1(1, true, sf, pop);

  auto seeded = sc1();

  // create useful pre-allocated vectors
  vector<size_t> contacts(250);

  // std::cout << "Seeded " << seeded.size() << " people: ";
  // for (auto p : seeded) {
  //   std::cout << p << " ";
  // }
  fmt::print("\n");

  // start totaltime

  // day loop
  for (int d_i = 1; d_i <= model.ndays; ++d_i) {
    sim::incr_day();

    // run beginning of day cases--required SeedCases

    // do vaccination if using vaccination

    // Loop through all people and process infectious ones (no vector allocation needed)
    int infectious_count = 0;
    for (size_t p = 1; p <= pop.popn; ++p) {
      if (pop.status[p] != Trait::Stat::infectious) continue;

      infectious_count++;

      // spread kernel
      auto spr_duration = pop.duration[p];  // a uint8_t
      auto variant_count = pop.variant_count[p];
      if (variant_count == 0) continue;  // Skip if no variant assigned
      auto spr_variant = pop.get_variant(p);    // variant_count is 1-based, array is 0-based
      auto sendrisk = mp.infectparams[static_cast<size_t>(spr_variant)].sendrisk[spr_duration];
      if (sendrisk > 0.0) spread(pop, p, mp.socialdata, mp.infectparams, contacts);

      // progression kernel
      progression(pop, p);

    }  // end person loop

    // Count recovered people (only for final report)
    // int recovered_count = 0;
    // for (size_t i = 1; i <= pop.popn; i++) {
    //   if (pop.status[i] == Trait::Stat::recovered) recovered_count++;
    // }

    // Print daily infection count
    // fmt::println("Day {}: {} infectious, {} recovered", sim::get_day(), infectious_count, recovered_count);

    // run end of day cases

    // update history series


    }  // end day loop
}
