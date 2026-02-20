#include "lib_includes.h"

#include "population.h"
#include "sim.h"
#include "random.h"
#include "helpers.h"
#include "disease_modeling.h"

// make_sick: make one person sick
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
      std::shift_left(sickday[p].begin(), sickday[p].end(), 1);
      variant[p].back() = var;
      sickday[p].back() = sim::get_day();
      variant_count[p]++;
  }
}

// make_well: make one person better->recovered and uninfected
// declaration is in population.h
void PopData::make_well(size_t p) {
  cond[p] = Trait::Cond::uninfected;
  status[p] = Trait::Stat::recovered;
  duration[p] = 0;
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

// Update passed in vector of contacts, which are indices to the population table
void get_contacts(const PopData &pop, float gammashape, uint8_t spr_agegrp,
                            uint8_t spr_cond, const array<array<float, 5>, 4> &contactfactors, vector<size_t> &contacts) {
  auto num_contacts = how_many_contacts(gammashape, spr_agegrp, spr_cond, contactfactors);
  xo::get_n_draws<size_t>(1, pop.popz, num_contacts, contacts);
}

bool istouched(const PopData &pop, size_t contact, const array<array<float, 5>, 6> &touchfactors) { // TODO add indoor_factor
  // logic copies Julia code but is wrong. only works because of touchfactors input values  TODO:  fix
  uint8_t contact_status = pop.status[contact];
  float touchprob{0.0};
  if ((contact_status == Trait::Stat::unexposed) || (contact_status == Trait::Stat::recovered)) {
    touchprob = touchfactors[idx(contact_status)][idx(pop.agegrp[contact])];
  }
  return xo::bernoulli(touchprob);
}

// TODO add inputs vaxfactor and recovfactor
float infectrisk(vector<InfectParams> &infectparams, uint8_t spr_variant,
                 uint8_t spr_duration, uint8_t contact_agegrp, float vaxfactor= 1.0, float recovfactor= 1.0) {
    // spreader person characteristics
    auto sendrisk = infectparams[spr_variant].sendrisk[spr_duration];

    // contact person characteristics
    auto recvrisk = infectparams[spr_variant].recvrisk[contact_agegrp];

    // vax_recov is meant to be a function for separation of concerns, but it is just minimum for now
    auto vax_recov = vaxfactor > recovfactor ? vaxfactor : recovfactor;
    auto combinedfactor = recvrisk * sendrisk * vax_recov;
    auto risk = std::clamp(combinedfactor, 0.0f, 0.97f);    // required because combinedfactor could exceed 1.0

  return risk;
  }
                  

/*
    isinfected(contact, spreader, vaxset, dovax, infectset, thisday)::Bool

Returns true if the spreader infected the contact or false if not. 
Considers partial immunity if contact has recovered from previous infection.
Considers vaccination status of the contact.
Considers the variant of the disease the spreader is carrying.
*/
bool isinfected(const PopData &pop, size_t contact, size_t spreader, vector<InfectParams> &infectparams, int thisday) {

    uint8_t spr_variant = pop.get_variant(spreader);

    // effect on transmission based on how long ago a previously infected contact got over the disease
    // TODO recovfactor = recoveffect(thisday, contact, spr_variant, infectparams);

    // effect on transmission based on whether, when, and which vaccine contact received
    // TODO vaxfactor = dovax ? vaxeffect(thisday, contact, vaxset, infectfactor, spr_variant) : 1.0;

    // binomial probability of the contact getting infected from the contact with this spreader
    float risk =
        infectrisk(infectparams, spr_variant, pop.duration[spreader],
                   pop.agegrp[contact]); // TODO use->, recovfactor, vaxfactor);

    return xo::bernoulli(risk) == 1;  // get a bool not 0 or 1
}