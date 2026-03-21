#include "lib_includes.h"

#include "population.h"
#include "disease_modeling.h"
#include "series.h"
#include "sim.h"

  // simplified early version of spread  TODO need vaxset, dovax sdcases
  void spread(PopData &pop, DayData & series, PopData::AgentView person, SocialParams &social,
              vector<InfectParams> &infectparams, vector<size_t> &contacts,
              float density_factor,
              vector<float> &indoor_seq) {

  auto thisday = sim::get_day();

  // retrieve needed parameters (use const references to avoid copying)
  const auto& contactfactors = social.contactfactors;
  const auto& touchfactors = social.touchfactors;
  auto gammashape = social.gammashape;
  auto spr_variant = person.get_variant();
  auto indoor_factor = indoor_seq[zidx(thisday)];

  // which contacts does the infected person have? use pre-allocated buffer contacts
  get_contacts(pop, density_factor, indoor_factor, gammashape, person.agegrp(), person.cond(), contactfactors, contacts);
  sim::ds.num_contacts += contacts.size();

  for (auto c : contacts) {
    auto contact = pop.agent(c);
    auto touchprob = touch_probability(contact, touchfactors, indoor_factor);

    if (touchprob > 0.0f && xo::bernoulli(touchprob)) {
      sim::ds.num_touched++;
      if (isinfected(contact, person, infectparams, thisday)) {  // TODO need vaxset, dovax
        sim::ds.num_new_infected++;
        auto this_contact = pop.agent(c);
        pop.make_sick(this_contact, spr_variant, series);
      }
    }
  }
}
