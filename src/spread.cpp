#include "lib_includes.h"

#include "population.h"
#include "disease_modeling.h"
#include "series.h"
#include "sim.h"

  // simplified early version of spread  TODO need vaxset, dovax sdcases
  void spread(PopData &pop, DayData & series, size_t p, SocialParams &social,
              vector<InfectParams> &infectparams, vector<size_t> &contacts,
              float density_factor,
              vector<float> &indoor_seq,
              SpreadDebugTrace* spread_debug_trace) {

  auto spreader = pop.agent(p);
  auto thisday = sim::get_day();

  // retrieve needed parameters (use const references to avoid copying)
  const auto& contactfactors = social.contactfactors;
  const auto& touchfactors = social.touchfactors;
  auto gammashape = social.gammashape;
  auto spr_variant = spreader.get_variant();
  auto indoor_factor = indoor_seq[zidx(thisday)];
  auto spr_sendrisk = infectparams[idx(spr_variant)].sendrisk[idx(spreader.duration())];
  // auto indoor_factor = 1.0;  // TODO just for debugging logic, remove later

  // which contacts does the infected person have? use pre-allocated buffer contacts
  get_contacts(pop, density_factor, indoor_factor, gammashape, spreader.agegrp(), spreader.cond(), contactfactors, contacts);
  sim::ds.num_contacts += contacts.size();

  if (spread_debug_trace != nullptr &&
      thisday <= spread_debug_trace->config.max_days &&
      spread_debug_trace->spreaders.size() < spread_debug_trace->config.max_spreaders) {
    spread_debug_trace->spreaders.push_back({
        .day = size_t(thisday),
        .spreader_id = p,
        .spr_agegrp = spreader.agegrp().name(),
        .spr_cond = spreader.cond().name(),
        .spr_duration = spreader.duration(),
        .spr_variant = spr_variant.name(),
        .indoor_factor = indoor_factor,
        .density_factor = density_factor,
        .contact_factor = contact_factor(contactfactors, spreader.agegrp(), spreader.cond()),
        .contact_scale = contact_scale(density_factor, indoor_factor, spreader.agegrp(), spreader.cond(), contactfactors),
        .num_contacts = contacts.size(),
        .sendrisk = spr_sendrisk,
    });
  }

  size_t contact_order = 0;
  for (auto c : contacts) {
    ++contact_order;
    auto target_agegrp = pop.agegrp[c];
    auto target_status = pop.status[c];
    auto target_cond = pop.cond[c];
    auto touchprob = touch_probability(pop, c, touchfactors, indoor_factor);
    auto touched = false;
    auto infected = false;
    InfectRiskComponents comps{};
    auto have_comps = false;

    if (touchprob > 0.0f && xo::bernoulli(touchprob)) {
      touched = true;
      sim::ds.num_touched++;
      comps = infectrisk_components(pop, c, spreader.id, infectparams, thisday);
      have_comps = true;
      if (xo::bernoulli(comps.risk)) {  // TODO need vaxset, dovax
        infected = true;
        sim::ds.num_new_infected++;
        auto this_contact = pop.agent(c);
        pop.make_sick(this_contact, spr_variant, series);
      }
    }

    if (spread_debug_trace != nullptr &&
        thisday <= spread_debug_trace->config.max_days &&
        spread_debug_trace->contacts.size() < spread_debug_trace->config.max_contacts) {
      if (!have_comps) {
        comps = infectrisk_components(pop, c, spreader.id, infectparams, thisday);
      }
      auto touch_factor_value =
          ((target_status == Stat::Unexposed) || (target_status == Stat::Recovered))
              ? touchfactors[idx(touch_map(target_status, target_cond))][zidx(target_agegrp)]
              : 0.0f;
      spread_debug_trace->contacts.push_back({
          .day = size_t(thisday),
          .spreader_id = p,
          .contact_order = contact_order,
          .contact_id = c,
          .targ_agegrp = target_agegrp.name(),
          .targ_status = target_status.name(),
          .targ_cond = target_cond.name(),
          .indoor_factor = indoor_factor,
          .touch_factor = touch_factor_value,
          .touch_prob = touchprob,
          .touched = touched,
          .sendrisk = comps.sendrisk,
          .recvrisk = comps.recvrisk,
          .recovfactor = comps.recovfactor,
          .vaxfactor = comps.vaxfactor,
          .infect_risk = comps.risk,
          .infected = infected,
      });
    }
  }
}
