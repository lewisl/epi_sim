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
              const vector<SocialDistancing>& sd_cases,
              const RingTraits& ringtraits,
              const std::vector<std::vector<size_t>>& ring_members,
              const std::vector<size_t>& ring_lengths) {

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

    // select which people are contacted; fill the pre-allocated contacts buffer
    contacts.clear();
    const size_t spr_ring = person.ring().v;
    if (ring_members.empty() || spr_ring == 0) {
      // rings disabled — global mixing, byte-identical to prior behavior
      xo::append_n_draws<size_t>(1, pop.popn, num_contacts, contacts);
    } else {
      // ring-aware selection: split contacts into same-ring and out-of-ring
      const auto& mem = ring_members[spr_ring];    // pop indices for in ring members
      const size_t P = pop.popn - mem.size();  // out-of-ring pool size
      const float p_out = ringtraits.out_ring_prob[spr_ring][spr_agegrp.v];
      const int k_out = (P == 0) ? 0    // num contacts outside of spreader's ring
          : std::binomial_distribution<int>{num_contacts, p_out}(xo::get_gen());
      const int k_in = num_contacts - k_out;

      xo::append_n_draws<size_t>(0, mem.size() - 1, k_in, contacts);   // in-ring ordinals
      if (k_out) xo::append_n_draws<size_t>(0, P - 1, k_out, contacts);  // out-ring ordinals

      // map ordinals -> 1-based person ids, in place
      for (int i = 0; i < k_in; ++i) contacts[i] = mem[contacts[i]];  // in-ring contacts
      for (int i = k_in; i < num_contacts; ++i) {  // sieve across candidate contacts in other rings
        size_t maybe_idx = contacts[i];
        for (size_t ring_num = 1; ring_num < ring_lengths.size(); ++ring_num) {
          if (ring_num == spr_ring) continue;   // skip the spreader's ring
          const size_t ring_sz = ring_lengths[ring_num];
          if (maybe_idx < ring_sz) { contacts[i] = ring_members[ring_num][maybe_idx]; break; } // test will pass eventually
          maybe_idx -= ring_sz;  // maybe an index in the next ring
        }
      }
    }

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
