#include "lib_includes.h"

#include "population.h"
#include "disease_modeling.h"
#include "sim.h"

  // simplified early version of spread  TODO need vaxset, dovax sdcases
  void spread(PopData &pop, size_t p, SocialParams &social,
              vector<InfectParams> &infectparams, vector<size_t> &contacts,
              float density_factor,
              vector<float> &indoor_seq) {

  auto spreader = pop.agent(p);
  auto thisday = sim::get_day();

  // retrieve needed parameters (use const references to avoid copying)
  const auto& contactfactors = social.contactfactors;
  const auto& touchfactors = social.touchfactors;
  auto gammashape = social.gammashape;
  auto spr_variant = spreader.get_variant();
  // auto indoor_factor = indoor_seq[zidx(thisday)];
  auto indoor_factor = 1.0;  // TODO just for debugging logic, remove later

  // which contacts does the infected person have? use pre-allocated buffer contacts
  get_contacts(pop, density_factor, indoor_factor, gammashape, spreader.agegrp(), spreader.cond(), contactfactors, contacts);
  sim::ds.num_contacts += contacts.size();

  for (auto c : contacts) {
    if (istouched(pop, c, touchfactors, indoor_factor)) {
      sim::ds.num_touched++;
      if (isinfected(pop, c, spreader.id, infectparams, thisday)) {  // TODO need vaxset, dovax
        sim::ds.num_new_infected++;
        auto this_contact = pop.agent(c);
        pop.make_sick(this_contact, spr_variant);
      }
    }
  }
}