#include "lib_includes.h"

#include "population.h"
#include "disease_modeling.h"
#include "series.h"
#include "sim.h"

  // simplified early version of spread  TODO need vaxset, dovax sdcases
  void spread(PopData &pop, DayData & series, PopData::AgentView person, SocialParams &social,
              vector<InfectParams> &infectparams, const VaxSet& vaxset, bool dovax,
              vector<size_t> &contacts, float density_factor,
              vector<float> &indoor_seq) {

  auto thisday = sim::get_day();

  // retrieve needed parameters (use const references to avoid copying)
  const auto& contactfactors = social.contactfactors;
  const auto& touchfactors = social.touchfactors;
  auto gammashape = social.gammashape;
  auto spr_variant = person.get_variant();
  auto spr_agegrp = person.agegrp();
  auto spr_cond = person.cond();
  auto indoor_factor = indoor_seq[zidx(thisday)];

  // which contacts does the infected person have? use pre-allocated buffer contacts
  auto contact_factor = contactfactors[zidx(spr_cond)][zidx(spr_agegrp)];
  auto scale = density_factor * indoor_factor * contact_factor;
  auto num_contacts = xo::gamma_int(gammashape, scale, 12);      
  xo::get_n_draws<size_t>(1, pop.popn, num_contacts, contacts);  // this function clears contacts before refilling it

  sim::ds.num_contacts += contacts.size();  // daily summary stat

  for (auto c : contacts) {
    auto contact = pop.agent(c);
    auto c_status = contact.status();
    float touchprob {0.0};

    // determine touchprob
    if ((c_status == Stat::Unexposed) || (c_status == Stat::Recovered)) {
      uint8_t target_touchmap = touch_map(c_status, contact.cond());
      auto baseprob = touchfactors[idx(target_touchmap)][zidx(contact.agegrp())];
      if (indoor_factor == 1.0f) {
        touchprob =  baseprob;
      }
      touchprob = std::clamp(baseprob * indoor_factor, 0.0f, 1.0f);
    }  else touchprob = 0.0f;

    // if touched; test if infected; if infected; make the contact sick
    if (touchprob > 0.0f && xo::bernoulli(touchprob) == 1.0f) { //short circuit the bernoulli RNG draw
      sim::ds.num_touched++;  // daily summary stat

      if (isinfected(contact, person, infectparams, vaxset, dovax, thisday)) {
        sim::ds.num_new_infected++;
        contact.make_sick(spr_variant, series); // contact is pop.agent(c) from above
      }
    }
  }
}
