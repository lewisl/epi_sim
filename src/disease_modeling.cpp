#include "lib_includes.h"

#include "population.h"
#include "series.h"
#include "sim.h"
#include "random.h"
#include "helpers.h"
#include <stdexcept>
#include "disease_modeling.h"

// make_sick: make one person sick
// declaration is in population.h
void PopData::AgentView::make_sick(Variant var,  DayData & series, Condition condition, uint8_t durationdays) {
  cond() = condition;
  duration() = durationdays;
  status() = Stat::Infectious;
  increment_series(series, SeriesName::new_infected, agegrp(), sim::get_day());

  auto &variant_vec = all_variants();
  auto &variant_cnt = variant_count();
  auto &sickday_vec = all_sickdays();
  
  if (variant_cnt < 16) {
    variant_vec[variant_cnt] = var;
    sickday_vec[variant_cnt] = sim::get_day();
    variant_cnt++;   // there is no sickday_count; it's the same as variant_count
  } else {
      std::shift_left(variant_vec.begin(), variant_vec.end(), 1);
      std::shift_left(sickday_vec.begin(), sickday_vec.end(), 1);
      variant_vec.back() = var;
      sickday_vec.back() = sim::get_day();
      ++variant_cnt;
      std::cerr << "Variant overflow for person " << id
              << ". Oldest variant lost.\n";
      std::cerr << "variant_count increased to " << variant_cnt << "\n";  
  }
}


/* 
  make_well: make one person better->recovered and uninfected
declaration is in population.h PopData::AgentView
method applied to an AgentView instance:  person.make_well()
as a method of AgentView, the instance variable is not used to apply methods or access members 
*/
void PopData::AgentView::make_well(DayData & series) {    // the object is person--the implied argument
  cond() = Cond::Uninfected; // equivalent to person.cond() in other functions where person defined
  status() = Stat::Recovered; 
  increment_series(series, SeriesName::new_recovered, agegrp(), sim::get_day());
  duration() = 0; 
  // update recovday
  if (recovday_count() < 16) {
    all_recovdays()[recovday_count()] = sim::get_day();
  } else {
      std::shift_left(all_recovdays().begin(), all_recovdays().end(), 1);
      all_recovdays().back() = sim::get_day();
      std::cerr << "Recovday overflow for person " << i
              << ". Oldest recovday lost.\n";
      std::cerr << "  Recovday_count increased to " << recovday_count() << "\n";  
  }
  ++recovday_count();
}

// this is an AgentView method:  where is the person?  called as person.make_dead(series)
void PopData::AgentView::make_dead(DayData & series) {
  // update the person: update deadday and status for the person
  deadday() = sim::get_day();   
  status() = Stat::Dead; // TODO will we need to set cond to uninfected for any other logic?
  // update the history of the simulation
  sim::ds.num_died++;
  increment_series(series, SeriesName::new_dead, agegrp(), deadday());
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
                  

bool isinfected(PopData::AgentView contact, PopData::AgentView spreader, vector<InfectParams> &infectparams, int thisday) {
    uint8_t spr_variant = spreader.get_variant();
    float recovfactor = recoveffect(contact, thisday, spr_variant, infectparams);
    float risk = infectrisk(infectparams, spr_variant, spreader.duration(),
                            contact.agegrp(), recovfactor);
    return xo::bernoulli(risk) == 1;  // return a bool, not 0 or 1  1.0f 0.935f 0.763f
}


/*
    recoveffect(const PopData &pop, size_t thisday, size_t contact, uint8_t 
      vector<InfectParams> &infectparams, float csig=6.0, float decay_lower=0.15)

Immunity from recovery for a single person.
defaults: csig = 6.0, decay_lower = 0.15
*/
float recoveffect(PopData::AgentView contact, size_t thisday,  uint8_t spr_variant,
                  vector<InfectParams> &infectparams, float csig, float decay_lower) {

    float factor = 1.0f; // return value

    if (contact.recovday_count() > 0) {
      
        size_t recovday = contact.get_recovday();
        size_t days_post_recov = thisday - recovday;

        if (days_post_recov >= 0) {
            uint8_t contact_variant = contact.get_variant();

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
