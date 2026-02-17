#include "lib_includes.h"

#include "population.h"
#include "sim.h"
#include "random.h"
#include "helpers.h"
#include "disease_modeling.h"

// make one person sick
// declaration is in population.h
void PopData::make_sick(size_t p, uint8_t var, uint8_t condition, uint8_t durationdays) {
  cond[p] = condition;
  duration[p] = durationdays;
  status[p] = Trait::Stat::infectious;
  if (variant_count[p] < 16) {
    variant[p][variant_count[p]] = var;
    sickday[p][variant_count[p]] = sim::get_day();
    variant_count[p]++;
  } else {
      std::cerr << "Variant overflow for person " << p
              << ". Oldest variant lost.\n";
      std::cerr << "variant_count increased to " << variant_count[p] << "\n";  
      std::shift_left(variant[p].begin(), variant[p].end(), 1);
      variant[p].back() = var;
      std::shift_left(sickday[p].begin(), sickday[p].end(), 1);
      sickday[p].back() = sim::get_day();
      variant_count[p]++;
  }
}

/*
Returns the number of contacts that someone spreading the disease will make on a day. This
method uses the spreadcase applicable to the current spreader but with contactfactors set by
a spreadcase.
*/
int how_many_contacts(float gammashape, uint8_t spr_agegrp,
                     uint8_t spr_cond, const array<array<float, 5>, 4>& contactfactors) {
  // TODO density_factor, indoor_factor
  auto scale = contactfactors[idx(spr_cond)][idx(spr_agegrp)];
  return xo::gamma_int(gammashape, scale, 12);
}

// Return a vector of contacts, which are indices to the population table
vector<size_t> get_contacts(PopData pop, float gammashape, uint8_t spr_agegrp,
                            uint8_t spr_cond, const array<array<float, 5>, 4>& contactfactors) {
  auto num_contacts = how_many_contacts(gammashape, spr_agegrp, spr_cond, contactfactors);
  auto draws = xo::get_n_draws(1, pop.popz, num_contacts);
  return vector<size_t>(draws.begin(), draws.end());
}