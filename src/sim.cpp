
#include "lib_includes.h"

#include "parameters.h"
#include "population.h"
#include "random.h"
#include "sim.h"
#include "setup.h"
#include "disease_modeling.h"
#include "spread.h"




// TODO later add parameters for runcases, showr0, silent, dovax?, vaxscheds?
void runsim(Model& model)
    // other arguments to add: runcases, showr0, silent, dovax, vaxscheds?
{
  ModelParams& mp = model.mp;  // all disease, vaccine, social parameters
  PopData& pop = model.pop;    // all person data

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

  std::cout << "Seeded " << seeded.size() << " people: ";
  for (auto p : seeded) {
    std::cout << p << " ";
  }
  std::cout << "\n";

  // start totaltime

  // day loop
  for (int d_i = 1; d_i <= model.ndays; ++d_i) {
    sim::incr_day();

    // run beginning of day cases--required SeedCases

    // do vaccination if using vaccination

    // infectious iterator:  we only loop through people who have the virus
    auto infectious = std::views::iota(size_t{1}, pop.status.size())
      | std::views::filter([&](size_t i) {
        return pop.status[i] == Trait::Stat::infectious;
    });

    for (auto p : infectious) {

      // spread kernel
      auto spr_duration = pop.duration[p];  // a uint8_t
      auto variant_count = pop.variant_count[p];
      if (variant_count == 0) continue;  // Skip if no variant assigned
      auto spr_variant = pop.variant[p][variant_count - 1];    // variant_count is 1-based, array is 0-based
      auto sendrisk = mp.infectset.infectparams[static_cast<size_t>(spr_variant)].second.sendrisk[spr_duration];
      if (sendrisk > 0.0) spread(pop, p, mp.socialdata, mp.infectset);


    }  // end infectious loop

    // run end of day cases

    // update history series


    }  // end day loop
}
