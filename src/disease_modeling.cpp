#include "lib_includes.h"

#include "population.h"
#include "series.h"
#include "sim.h"
#include "random.h"
#include "helpers.h"
#include <stdexcept>
#include "disease_modeling.h"

namespace {

const VaxParams& require_vax_params(const VaxSet& vaxset, const string& name) {
  for (const auto& [vax_name, params] : vaxset.vaxset) {
    if (vax_name == name) return params;
  }
  throw std::runtime_error("Unknown vaccine in VaxSet: " + name);
}

float require_named_factor(const vector<std::pair<string, float>>& entries,
                           const string& key,
                           const string& context) {
  const auto it = std::find_if(entries.begin(), entries.end(),
                               [&](const auto& entry) { return entry.first == key; });
  if (it == entries.end()) {
    throw std::runtime_error(fmt::format("Missing {} factor for key '{}'", context, key));
  }
  return it->second;
}

float require_effectiveness(const VaxParams& params,
                            const string& shot_name,
                            const string& variant_name) {
  const auto shot_it = std::find_if(params.effectiveness.begin(), params.effectiveness.end(),
                                    [&](const auto& entry) { return entry.first == shot_name; });
  if (shot_it == params.effectiveness.end()) {
    throw std::runtime_error(fmt::format("Missing vaccine effectiveness for shot '{}'", shot_name));
  }

  return require_named_factor(shot_it->second, variant_name,
                              fmt::format("effectiveness('{}')", shot_name));
}

} // namespace

// make_sick: make one person sick
// declaration is in population.h
void PopData::AgentView::make_sick(Variant var,  HistorySeries & series, Condition condition, uint8_t durationdays) {
  auto today = sim::get_day();
  series.delta_series(SeriesName::new_infected, agegrp(), today, 1);
  series.delta_series(SeriesName::now_infected, agegrp(), today, 1);

  if (status() == Stat::Recovered) {
    series.delta_series(SeriesName::now_recovered, agegrp(), today, -1);
  } else {
    if (status() == Stat::Unexposed) {
      series.delta_series(SeriesName::now_unexposed, agegrp(), today, -1);
    }
  }
  cond() = condition;
  duration() = durationdays;
  status() = Stat::Infectious;
  // increment_series(series, SeriesName::new_infected, agegrp(), sim::get_day());

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
      if (sim::debug) {
        std::cerr << "Variant overflow for person " << id
                << ". Oldest variant lost.\n";
        std::cerr << "variant_count increased to " << variant_cnt << "\n";
      }
  }
}


/* 
  make_well: make one person better->recovered and uninfected
declaration is in population.h PopData::AgentView
method applied to an AgentView instance:  person.make_well()
as a method of AgentView, the instance variable is not used to apply methods or access members 
*/
void PopData::AgentView::make_well(HistorySeries & series) {    // the object is person--the implied argument
  auto today = sim::get_day();
  series.delta_series(SeriesName::now_recovered, agegrp(), today, 1);
  series.delta_series(SeriesName::new_recovered, agegrp(), today, 1);
  series.delta_series(SeriesName::now_infected, agegrp(), today, -1);

  cond() = Cond::Uninfected; // equivalent to person.cond() in other functions where person defined
  status() = Stat::Recovered; 
  // increment_series(series, SeriesName::new_recovered, agegrp(), sim::get_day());
  duration() = 0; 
  // update recovday
  if (recovday_count() < 16) {
    all_recovdays()[recovday_count()] = sim::get_day();
  } else {
      std::shift_left(all_recovdays().begin(), all_recovdays().end(), 1);
      all_recovdays().back() = sim::get_day();
      if (sim::debug) {
        std::cerr << "Recovday overflow for person " << i
                << ". Oldest recovday lost.\n";
        std::cerr << "  Recovday_count increased to " << recovday_count() << "\n";
      }
  }
  ++recovday_count();
}

// this is an AgentView method:  where is the person?  called as person.make_dead(series)
void PopData::AgentView::make_dead(HistorySeries & series) {
    auto today = sim::get_day();
    series.delta_series(SeriesName::now_dead, agegrp(), today, 1);
    series.delta_series(SeriesName::new_dead, agegrp(), today, 1);
    series.delta_series(SeriesName::now_infected, agegrp(), today, -1);

  // update the person: update deadday and status for the person
  deadday() = today;   
  status() = Stat::Dead; // TODO will we need to set cond to uninfected for any other logic?
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

    auto combined_immunity = vax_recov(vaxfactor, recovfactor);

    auto combinedfactor = recvrisk * sendrisk * combined_immunity;
    auto risk = std::clamp(combinedfactor, 0.0f, 1.0f);    // required because combinedfactor could exceed 1.0

  return risk;
}
                  
bool isinfected(PopData::AgentView contact, PopData::AgentView spreader,
                vector<InfectParams> &infectparams, const VaxSet& vaxset,
                bool dovax, int thisday) {
    uint8_t spr_variant = spreader.get_variant();
    float recovfactor = recoveffect(contact, thisday, spr_variant, infectparams);
    float vaxfactor = dovax ? vaxeffect(thisday, contact, vaxset, spr_variant) : 1.0f;
    float risk = infectrisk(infectparams, spr_variant, spreader.duration(),
                            contact.agegrp(), recovfactor, vaxfactor);
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

float vaxeffect(size_t thisday, PopData::AgentView person, const VaxSet& vaxset,
                uint8_t target_variant, float csig, float decay_lower) {
  if (person.vaxstatus() == Vaxstat::none || person.vax_count() == 0) return 1.0f;

  const uint8_t vax_count = person.vax_count();
  const uint8_t vax_idx = person.vaxrcvd()[zidx(vax_count)];
  const int16_t vaxday = person.vaxday()[zidx(vax_count)];
  const string vax_name = person.vax_labels().to_str(vax_idx);
  const auto& params = require_vax_params(vaxset, vax_name);

  if (target_variant >= Variant::names.size()) {
    throw std::runtime_error(fmt::format("Invalid variant index in vaxeffect: {}", target_variant));
  }

  const string variant_name = Variant::names[target_variant];
  const string shot_name = person.vaxstatus().name();
  const float infectfactor = require_named_factor(params.infectfactor, variant_name, "infectfactor");
  const float vaccine_effect = require_effectiveness(params, shot_name, variant_name);

  const int days_after_vax = std::max<int>(static_cast<int>(thisday) - static_cast<int>(vaxday), 0);
  const int days_after_full_effect = std::max(days_after_vax - params.full_effect_days, 0);
  const float rise = effect_rise(static_cast<size_t>(days_after_vax),
                                 params.day1_effect,
                                 static_cast<float>(params.full_effect_days));
  const float decay = sigdecay(static_cast<size_t>(days_after_full_effect),
                               static_cast<float>(params.halflife),
                               csig,
                               decay_lower);
  const float time_mod = rise * decay;

  return std::max(1.0f - (time_mod * vaccine_effect * infectfactor), 0.0f);
}

float vax_recov(float vaxfactor, float recovfactor) {
  return std::min(vaxfactor, recovfactor);
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
