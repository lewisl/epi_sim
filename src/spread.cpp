#include "lib_includes.h"

#include "population.h"
#include "disease_modeling.h"
#include "sim.h"

// simplified early version of spread
void spread(PopData pop, size_t p, SocialParams social, InfectSet infect) {

  auto thisday = sim::get_day();

  // retrieve needed parameters
  auto contact_factors = social.contactfactors;
  auto touch_factors = social.touchfactors;
  auto gammashape = social.gammashape;
  // indoor_factor = indoor_seq[thisday];   // TODO

  auto contact_param = contact_factors; // TODO

  // how many contacts does the infected person have?
  auto contacts = get_contacts(pop, gammashape, pop.agegrp[p], pop.cond[p], contact_factors);

  fmt::println(" the contacts are: \n{}\n", contacts);

}