#include "lib_includes.h"

#include "population.h"
#include "series.h"
#include "sim.h"
#include "random.h"
#include "helpers.h"
#include <stdexcept>
#include "disease_modeling.h"

namespace {

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
void AgentView::make_sick(Variant var,  HistorySeries & series, Condition condition, uint8_t spr_duration) {
  auto today = sim::get_day();
  series.delta_series(SeriesName::new_infected, agegrp(), today, 1);
  series.delta_series(SeriesName::now_infected, agegrp(), today, 1);

  if (status() == RECOVERED) {
    series.delta_series(SeriesName::now_recovered, agegrp(), today, -1);
  } else {
    if (status() == UNEXPOSED) {
      series.delta_series(SeriesName::now_unexposed, agegrp(), today, -1);
    }
  }
  cond() = condition;
  duration() = spr_duration;
  status() = INFECTIOUS;
  variant() = var;
  sickday() = sim::get_day();
  // increment_series(series, SeriesName::new_infected, agegrp(), sim::get_day());

  auto &history = variant_hist();
  auto &day_history = sickday_hist();
  const bool history_overflow = history.count >= 16;

  history.set(var);
  day_history.set(sim::get_day());

  if (history_overflow && sim::debug) {
    std::cerr << "Variant and sickday overflow for person " << id
              << ". Oldest history entries lost.\n";
    std::cerr << "variant_hist.count increased to " << static_cast<int>(history.count)
              << ", sickday_hist.count increased to " << static_cast<int>(day_history.count) << "\n";
  }
}


/* 
  make_well: make one person better->recovered and uninfected
declaration is in population.h AgentView
method applied to an AgentView instance:  person.make_well()
as a method of AgentView, the instance variable is not used to apply methods or access members 
*/
void AgentView::make_well(HistorySeries & series) {    // the object is person--the implied argument
  auto today = sim::get_day();
  series.delta_series(SeriesName::now_recovered, agegrp(), today, 1);
  series.delta_series(SeriesName::new_recovered, agegrp(), today, 1);
  series.delta_series(SeriesName::now_infected, agegrp(), today, -1);

  cond() = UNINFECTED; // equivalent to person.cond() in other functions where person defined
  status() = RECOVERED; 
  // increment_series(series, SeriesName::new_recovered, agegrp(), sim::get_day());
  duration() = 0; 
  recovday() = today;

  auto& history = recovday_hist();
  const bool history_overflow = history.count >= 16;
  history.set(today);

  if (history_overflow && sim::debug) {
    std::cerr << "Recovday overflow for person " << id
              << ". Oldest history entries lost.\n";
    std::cerr << "recovday_hist.count increased to " << static_cast<int>(history.count) << "\n";
  }
}

// this is an AgentView method:  where is the person?  called as person.make_dead(series)
void AgentView::make_dead(HistorySeries & series) {
    auto today = sim::get_day();
    series.delta_series(SeriesName::now_dead, agegrp(), today, 1);
    series.delta_series(SeriesName::new_dead, agegrp(), today, 1);
    series.delta_series(SeriesName::now_infected, agegrp(), today, -1);

  // update the person: update deadday and status for the person
  deadday() = today;   
  status() = DEAD; // TODO will we need to set cond to uninfected for any other logic?
}


//clang-format off
uint8_t touch_map(Status target_status, Condition target_cond) {
  switch (target_status)
  {
  case UNEXPOSED:           return uint8_t{0};
  case RECOVERED:           return uint8_t{1};
  case INFECTIOUS:
    switch (target_cond)
    {
    case NIL:               return uint8_t{2};
    case MILD:              return uint8_t{3};
    case SICK:              return uint8_t{4};  
    case SEVERE:            return uint8_t{5};
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
                  
bool isinfected(AgentView contact, AgentView spreader,
                vector<InfectParams> &infectparams, const VaxSet& vaxset,
                bool dovax, int thisday) {
    uint8_t spr_variant = spreader.variant();
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
float recoveffect(AgentView contact, size_t thisday,  uint8_t spr_variant,
                  vector<InfectParams> &infectparams, float csig, float decay_lower) {

    float factor = 1.0f; // return value

    if (contact.recovday() > 0) {
        const size_t recovday = static_cast<size_t>(contact.recovday());
        size_t days_post_recov = thisday - recovday;

        if (days_post_recov >= 0) {
            uint8_t contact_variant = contact.variant();

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

float vaxeffect(size_t thisday, AgentView person, const VaxSet& vaxset,
                uint8_t target_variant, float csig, float decay_lower) {
  if (person.vaxstatus() == Vaxstat::none || idx(person.vaxrcvd()) == 0 || person.vaxday() == 0) {
    return 1.0f;
  }

  const auto& params = vaxset.at(person.vaxrcvd());
  const int16_t vaxday = person.vaxday();

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
