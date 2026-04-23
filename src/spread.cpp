#include "lib_includes.h"

#include "population.h"
#include "disease_modeling.h"
#include "cases.h"
#include "series.h"
#include "sim.h"

  // simplified early version of spread  TODO need vaxset, dovax sdcases
  void spread(PopData &pop, AllSeries & series, AgentView person, SocialParams &social,
              vector<InfectParams> &infectparams, const VaxSet& vaxset, bool dovax,
              vector<size_t> &contacts, float density_factor,
              vector<float> &indoor_seq,
              const vector<SocialDistancing>& sd_cases) {

    auto thisday = sim::get_day();

    // retrieve needed parameters (use const references to avoid copying)
    // spreader's contactfactors are overridden by their sdcase, if any
    const uint8_t spr_sd = person.sdcase();
    const auto& contactfactors = spr_sd == 0 ? social.contactfactors
                                             : sd_cases[zidx(spr_sd)].contactfactors;
    auto gammashape = social.gammashape;
    auto spr_variant = person.variant();
    auto spr_agegrp = person.agegrp();
    auto spr_cond = person.cond();
    auto indoor_factor = indoor_seq[zidx(thisday)];

    // which contacts does the infected person have? use pre-allocated buffer contacts
    auto contact_factor = contactfactors[zidx(spr_cond)][zidx(spr_agegrp)];
    auto scale = density_factor * indoor_factor * contact_factor;
    auto num_contacts = xo::gamma_int(gammashape, scale, 12);
    xo::get_n_draws<size_t>(1, pop.popn, num_contacts, contacts);  // this function clears contacts before refilling it

    // sim::ds.num_contacts += contacts.size();  // daily summary stat

    for (auto c : contacts) {
      auto contact = pop.agent(c);
      auto c_status = contact.status();
      float touchprob {0.0};

      // determine touchprob
      if ((c_status == UNEXPOSED) || (c_status == RECOVERED)) {
        // contact's touchfactors are overridden by their sdcase, if any
        const uint8_t c_sd = contact.sdcase();
        const auto& touchfactors = c_sd == 0 ? social.touchfactors
                                             : sd_cases[zidx(c_sd)].touchfactors;
        uint8_t target_touchmap = touch_map(c_status, contact.cond());
        auto baseprob = touchfactors[idx(target_touchmap)][zidx(contact.agegrp())];
        if (indoor_factor == 1.0f) {
          touchprob =  baseprob;
        }
        touchprob = std::clamp(baseprob * indoor_factor, 0.0f, 1.0f);
      }  else touchprob = 0.0f;

      // if touched; test if infected; if infected; make the contact sick
      if (touchprob > 0.0f && xo::bernoulli(touchprob) == 1.0f) { //short circuit the bernoulli RNG draw
        // sim::ds.num_touched++;  // daily summary stat

        if (isinfected(contact, person, infectparams, vaxset, dovax, thisday)) {
          contact.make_sick(spr_variant, series); // contact is pop.agent(c) from above
        }
      }
  }
}
