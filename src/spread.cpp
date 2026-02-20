#include "lib_includes.h"

#include "population.h"
#include "disease_modeling.h"
#include "sim.h"

// simplified early version of spread  TODO need vaxset, dovax sdcases
void spread(PopData &pop, size_t p, SocialParams &social, vector<InfectParams> &infectparams, vector<size_t> &contacts) {

  auto spreader = p;
  auto thisday = sim::get_day();

  // retrieve needed parameters (use const references to avoid copying)
  const auto& contactfactors = social.contactfactors;
  const auto& touchfactors = social.touchfactors;
  auto gammashape = social.gammashape;
  // indoor_factor = indoor_seq[thisday];   // TODO

  // which contacts does the infected person have? use pre-allocated buffer contacts
  get_contacts(pop, gammashape, pop.agegrp[spreader], pop.cond[spreader], contactfactors, contacts);

  for (auto c : contacts) {
    if (istouched(pop, c, touchfactors)) {
      if (isinfected(pop, c, spreader, infectparams, thisday)) {  // TODO need vaxset, dovax
        pop.make_sick(c, pop.variant[spreader][pop.variant_count[spreader] - 1]);
      }
    }
  }
}