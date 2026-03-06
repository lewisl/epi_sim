#include "lib_includes.h"

#include "population.h"
#include "sim.h"
#include "random.h"
#include "helpers.h"
#include <stdexcept>
#include "disease_modeling.h"

// make_sick: make one person sick
// declaration is in population.h
void PopData::make_sick(size_t p, Variant var, Condition condition, uint8_t durationdays) {
  cond[p] = condition;
  duration[p] = durationdays;
  status[p] = Stat::Infectious;
  if (variant_count[p] < 16) {
    variant[p][variant_count[p]] = var;
    sickday[p][variant_count[p]] = sim::get_day();
    variant_count[p]++;
  } else {
      std::shift_left(variant[p].begin(), variant[p].end(), 1);
      std::shift_left(sickday[p].begin(), sickday[p].end(), 1);
      variant[p].back() = var;
      sickday[p].back() = sim::get_day();
      ++variant_count[p];
      std::cerr << "Variant overflow for person " << p
              << ". Oldest variant lost.\n";
      std::cerr << "variant_count increased to " << variant_count[p] << "\n";  
  }
}

// make_well: make one person better->recovered and uninfected
// declaration is in population.h
void PopData::make_well(size_t p) {
  cond[p] = Cond::Uninfected;
  status[p] = Stat::Recovered;
  duration[p] = 0;
  if (recovday_count[p] < 16) {
    recovday[p][recovday_count[p]] = sim::get_day();
    recovday_count[p]++;
  } else {
      std::shift_left(recovday[p].begin(), recovday[p].end(), 1);
      recovday[p].back() = sim::get_day();
      ++recovday_count[p];
      std::cerr << "Recovday overflow for person " << p
              << ". Oldest recovday lost.\n";
      std::cerr << "recovday_count increased to " << recovday_count[p] << "\n";  
  }
}

/*
Returns the number of contacts that someone spreading the disease will make on a day. This
method uses the spreadcase applicable to the current spreader but with contactfactors set by
a spreadcase.
*/
int how_many_contacts(float density_factor, float indoor_factor,
                      float gammashape, uint8_t spr_agegrp, uint8_t spr_cond,
                      const array<array<float, 5>, 4> &contactfactors) {

  // temporary debugging fudge TODO
  // indoor_factor = 1.0;
  
  auto scale = density_factor * indoor_factor *
               contactfactors[zidx(spr_cond)][zidx(spr_agegrp)];
  // if (sim::get_day() == 2) {
  //   fmt::println("density {}, indoor {}, cf {}, scale {}, cond {}, agegrp {}", density_factor, indoor_factor,
  //                contactfactors[zidx(spr_cond)][zidx(spr_agegrp)], scale, spr_cond, spr_agegrp);
  // }

  
  return xo::gamma_int(gammashape, scale, 12);
}

// Update passed in vector of contacts, which are indices to the population table
void get_contacts(const PopData &pop, float density_factor, float indoor_factor, float gammashape, uint8_t spr_agegrp,
                            uint8_t spr_cond, const array<array<float, 5>, 4> &contactfactors, vector<size_t> &contacts) {
  auto num_contacts = how_many_contacts(density_factor, indoor_factor, gammashape, spr_agegrp, spr_cond, contactfactors);
  xo::get_n_draws<size_t>(1, pop.popn, num_contacts, contacts);
}


//clang-format off
uint8_t touch_map(Status target_status, Condition target_cond) {
  switch (target_status)
  {
  case Stat::Unexposed:     return uint8_t{0};
  case Stat::Recovered:     return uint8_t{1};
  case Stat::Infectious:
    switch (target_cond)
    {
    case Cond::Nil:         return uint8_t{2};
    case Cond::Mild:        return uint8_t{3};
    case Cond::Sick:        return uint8_t{4};  
    case Cond::Severe:      return uint8_t{5};
    default:
      throw std::runtime_error("Invalid condition input for touch_map");
    }
  default:
    throw std::runtime_error("Invalid status input for touch_map");
  }
}
//clang-format on


bool istouched(const PopData &pop, size_t contact, const array<array<float, 5>, 6> &touchfactors, float indoor_factor) {
  Status target_status = pop.status[contact];
  Condition target_condition = pop.cond[contact];
  if ((target_status == Stat::Unexposed) || (target_status == Stat::Recovered)) {
    uint8_t target_touchmap = touch_map(target_status, target_condition);
    float touchprob{0.0};

    if (indoor_factor == 1.0f) {
      touchprob = touchfactors[idx(target_touchmap)][zidx(pop.agegrp[contact])];
    } else {
      touchprob = std::clamp(
          touchfactors[idx(target_touchmap)][zidx(pop.agegrp[contact])] *
              indoor_factor,
          0.0f, 0.97f);
      }
    return xo::bernoulli(touchprob) == 1;
  }
  return false;
}

float infectrisk(vector<InfectParams> &infectparams, uint8_t spr_variant,
                 uint8_t spr_duration, uint8_t contact_agegrp, float recovfactor, float vaxfactor) {
    // spreader person characteristics
    auto sendrisk = infectparams[spr_variant].sendrisk[spr_duration];

    // contact person characteristics
    auto recvrisk = infectparams[spr_variant].recvrisk[zidx(contact_agegrp)];

    // vax_recov is meant to be a function for separation of concerns, but it is just minimum for now
    auto vax_recov = std::min(vaxfactor, recovfactor);

    // fmt::println("================= vax_recov {}", vax_recov);

    auto combinedfactor = recvrisk * sendrisk * vax_recov;
    auto risk = std::clamp(combinedfactor, 0.0f, 1.0f);    // required because combinedfactor could exceed 1.0

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
    float recovfactor = 
        recoveffect(pop, thisday, contact, spr_variant, infectparams);


    // effect on transmission based on whether, when, and which vaccine contact received
    // TODO vaxfactor = dovax ? vaxeffect(thisday, contact, vaxset, infectfactor, spr_variant) : 1.0;

    // binomial probability of the contact getting infected from the contact with this spreader
    float risk = infectrisk(infectparams, spr_variant, pop.duration[spreader],
                            pop.agegrp[contact], recovfactor, 1.0);

    // if (pop.recovday_count[contact] == 0)
    //   fmt::println("*** ================= recovfactor {}, risk {}, bernoulli {}", recovfactor, risk, xo::bernoulli(risk));


    // multiplicative factor for debugging TODO: had used 0.84f before implementing density_factor
    return xo::bernoulli(risk) == 1;  // return a bool, not 0 or 1  1.0f 0.935f 0.763f
}


/*
    recoveffect(const PopData &pop, size_t thisday, size_t contact, uint8_t 
      vector<InfectParams> &infectparams, float csig=6.0, float decay_lower=0.15)

Immunity from recovery for a single person.
defaults: csig = 6.0, decay_lower = 0.15
*/
float recoveffect(const PopData &pop, size_t thisday, size_t contact, uint8_t spr_variant,
                  vector<InfectParams> &infectparams, float csig, float decay_lower) {

    float factor = 1.0f; // return value

    if (pop.recovday_count[contact] > 0) {
      
        size_t recovday = pop.get_recovday(contact);
        size_t days_post_recov = thisday - recovday;

        if (days_post_recov >= 0) {
            uint8_t contact_variant = pop.get_variant(contact);

            // get the max immunity for the variant that target recovered from against the variant of the spreader
            float immstrength = infectparams[contact_variant].recovery_immunity[spr_variant];

            // get the declined value
            float immhalflife = infectparams[contact_variant].immunehalflife;

            // float immdecline = lindecay(days_post_recov, immhalflife, decay_lower);
            float decay = sigdecay(days_post_recov, immhalflife, csig, decay_lower);
            float rise = effect_rise(days_post_recov);
            float time_mod = rise * decay;

            factor = 1.0f - (time_mod * immstrength);
        }
    }
  return factor;
}

// gradual decay of vaccine effectiveness or recovery immunity based on assumed half-life
// and rise

/*
    effect_rise(days_since; mineff=0.65, delay_days=14)
  
Immunity effectiveness from vaccination or recovery ramps up.
Returns a value between mineff and 1.0. Linear increase.
Default mineff = 0.65, delay_days = 14
*/
float effect_rise(size_t days_since, float mineff, float delay_days) {
  float y = 0.0f;
  if (days_since >= delay_days) y = 1.0f;
  else y = mineff + (days_since/delay_days * (1.0f - mineff));  // rises from mineff to just below 1.0
  return y;
}

float lindecay(size_t t, float h, float lower) {
    float y = 0.5 / -h * t  + 1.0;
    y = y < lower ? lower : y;
    return y;
}

float expdecay(size_t t, float h) {
  float y = exp(-(log(2) / h) * t);
  return y;
}

float sigdecay(size_t t, float h, float csig, float decay_lower) {
  float tfl = static_cast<float>(t);
  float y =
      fmax(1.0 / (1.0 + exp((tfl - h) / (tfl / csig + (h / csig)))), decay_lower);
  return y;
}

float tbrk(float h, float lower) {
  float y = 2.0f * h - (2.0f * h * lower);
  return y;
}

float intercept(size_t t, float hl) {
  float tfl = static_cast<float>(t);
  return -0.3 * tfl / hl + 1.0;
}