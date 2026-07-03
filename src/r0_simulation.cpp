#include <algorithm>
#include <cmath>

#include <vector>
#include <span>
#include <algorithm>
#include <cassert>

#include <fmt/base.h>
#include <fmt/format.h> // only get what I use: about 12k in the executable!
#include <fmt/ranges.h> // for printing containers like vector
#include <fmt/ostream.h> // to use ostream file handles and << >> operators

#include "parameters.h"
#include "population.h"
#include "r0_simulation.h"
#include "progression.h"
#include "spread.h"
#include "disease_modeling.h"
#include "setup.h"
#include "traits.h"

// forward declaration
namespace {
  double run_r0_sim(Model & model, PopData & rtpop, Variant variant, int scale);
}

/*
    r0_sim(popsize, age_dist, progressionset, trvec, infectset, vaxset, 
                socialparams, density_factor=1.0, scale=3)

Simulates r0. This overload creates a population and tracks how many infections
are caused by first generation spreaders and NOT spreaders who were infected by the
first generation (or later). 
*/

double r0_sim(Model & model) {
  
  int scale = 3;
  Variant use_variant{1};  // this will be the required base variant
  if (Variant::names.size() <= 1) {
    throw std::runtime_error("r0_sim requires a loaded base variant at index 1.");
  }
  if (Variant::names[1] != "base") {
    throw std::runtime_error("r0_sim requires Variant index 1 to be 'base'.");
  }

  int popn = 200'000; 
 
  // synthesized population
  PopData r0_pop = PopData(popn, model.age_dist);

  return run_r0_sim(model, r0_pop, use_variant, scale);
}


/*
    double rt_sim(locdat, age_dist, progressionset, trvec, infectset, vaxset, 
                socialparams, density_factor, scale)

Simulates r at time t given the characteristics of the simulation
you are running. This shows how r is affected by public health
measures and the characteristics of the population over time based on your simulation
parameters. This simulates r(t).

It will be called during a simulation run in function runsim so that it uses the
context of the current simulation with its input parameters and current population state.
*/
void rt_sim(PopData locdat, Model & model) {

    // create the simulation population--> this is all we have to do in the front end function
    PopData rtpop {locdat};
    int scale = 3;
    Variant use_variant = 1;

    const int saved_day = sim::current_day;
    sim::reset_day();



    // create a subset of the actual model indoor_seq for running the rt simulation
    vector<float> rt_indoor_seq{};
    rt_indoor_seq.reserve(DURATIONLIM);

    // create DURATIONLIM long vector of indoor_seq. 
    size_t seq_len = static_cast<size_t>(DURATIONLIM);

    assert(saved_day < model.indoor_seq.size());

    std::size_t available = model.indoor_seq.size() - saved_day;
    std::size_t take = std::min(seq_len, available);

    rt_indoor_seq.insert(rt_indoor_seq.end(), model.indoor_seq.begin() + saved_day, model.indoor_seq.begin() + saved_day + take);

    if (take < seq_len) {
        rt_indoor_seq.resize(seq_len, model.indoor_seq.back());
    }


    std::optional<double> rt = run_rt_sim(model, rtpop, use_variant, scale, rt_indoor_seq);
    sim::current_day = saved_day;
    sim::ds.day = saved_day;

    if (!rt) {
      fmt::println("Could not seed spreaders for rt_simulation. Simulation continuing.");
    }
    else {
      fmt::println("rt at simulaton day {} = {}", saved_day, *rt);
    }
  }

namespace{

struct SimDayGuard {
  int current_day = sim::current_day;
  sim::daystats ds = sim::ds;

  ~SimDayGuard() {
    sim::current_day = current_day;
    sim::ds = ds;
  }
};

void seed_gen1(vector<size_t> & gen1_spreaders, PopData & rtpop, AllSeries & r0series, vector<double> & age_dist, Variant usevariant, int scale) {
// WE DO NEED TO TRICK DAY TO USE make_sick--we could use day 0, which is normally not used to hold the seeding counts 
  // Per-age seeding budget proportional to age_dist / min(age_dist), scaled.
  double min_share = std::ranges::min(age_dist);
  vector<int> cnt_by_age{};
  for (size_t i = 0; i < age_dist.size(); ++i) {
    cnt_by_age.push_back(static_cast<int>(std::round((age_dist[i] / min_share) * scale)));
  }

  fmt::println("spreaders by age: {} = {}", cnt_by_age, sum(cnt_by_age));
 
  vector<int> remaining{cnt_by_age};  // make a copy
  for (size_t p = 1; p <= rtpop.popn; ++p) {
    auto person = rtpop.agent(p);
    const size_t age_idx = zidx(person.agegrp());  // index into remaining 
    if (remaining[age_idx] > 0) {
      person.make_sick(usevariant, r0series);
      gen1_spreaders.push_back(p);
      --remaining[age_idx];
    }
  }
}

size_t spread_and_count(PopData& pop, AllSeries& series, AgentView person,
                        SocialParams& social, std::vector<InfectParams>& infectparams,
                        const VaxSet& vaxset, bool dovax, std::vector<size_t>& contacts,
                        float density_factor, const std::vector<float>& indoor_seq) {
  const int thisday = sim::get_day();
  const auto& contactfactors = social.contactfactors;
  const float gammashape = social.gammashape;
  const Variant spr_variant = person.variant();
  const Agegrp spr_agegrp = person.agegrp();
  const Condition spr_cond = person.cond();
  const float indoor_factor = indoor_seq[static_cast<size_t>(thisday - 1)];

  const float contact_factor = contactfactors[zidx(spr_cond)][zidx(spr_agegrp)];
  const float contact_scale = density_factor * indoor_factor * contact_factor;
  const int num_contacts = xo::gamma_int(gammashape, contact_scale, 12);

  contacts.clear();
  xo::append_n_draws<size_t>(1, pop.popn, num_contacts, contacts);

  size_t newly_infected = 0;
  for (size_t c : contacts) {
    auto contact = pop.agent(c);
    const Status c_status = contact.status();
    float touchprob = 0.0f;

    if (c_status == UNEXPOSED || c_status == RECOVERED) {
      const uint8_t target_touchmap = touch_map(c_status, contact.cond());
      const float baseprob = social.touchfactors[idx(target_touchmap)][zidx(contact.agegrp())];
      touchprob = std::clamp(baseprob * indoor_factor, 0.0f, 1.0f);
    }

    // count the infection but don't change population state per academic defintion of R0
    if (touchprob > 0.0f && xo::bernoulli(touchprob) == 1.0f
        && isinfected(contact, person, infectparams, vaxset, dovax, thisday)) {
      ++newly_infected;
    }
  }

  return newly_infected;
}

double run_r0_sim(Model & model, PopData & r0pop, Variant variant, int scale) {

  // Snapshot the global day counter so a midstream simulation (future use)
  // isn't disturbed. We drive the R0 day loop starting from day 1.
  const int saved_day = sim::current_day;
  sim::reset_day();


  // deref or create arguments needed for spread
  int locale = model.locale;
  bool dovax = model.dovax;         
  VaxSet & vaxset = model.mp.vaxset;  
  ProgressionSet & progressionset = model.mp.progressionset;
  vector<InfectParams> & infectparams = model.mp.infectparams;  
  SocialParams & socialparams = model.mp.socialdata;
  AllSeries r0series(DURATIONLIM, r0pop, Variant::names.size(), 1, 1);  
  vector<size_t> contacts(250); // reserve and set size, cleared before later usage
  array<float, 6> probvec{};
  vector<double> age_dist = model.age_dist;

  double density_factor = 1.0;
  vector<size_t> gen1_spreaders{};  // mutated in place


  seed_gen1(gen1_spreaders, r0pop, r0series, age_dist, variant, scale);
  const size_t gen1_spreader_cnt = gen1_spreaders.size();
  if (gen1_spreader_cnt == 0) {
    sim::current_day = saved_day;
    sim::ds.day = saved_day;
    throw std::runtime_error("No spreaders assigned.");
  }

  size_t r0_infected = 0;
  vector<float> indoor_seq(DURATIONLIM, 1.0f);  // assume no seasonality for abstract r0 simulation

  //
  // mini simulation loop
  //
  for (size_t d=1; d <= DURATIONLIM; ++d) {   // day loop
    // start a new day
    sim::incr_day();
    sim::ds.day = sim::get_day();

    // loop over gen 1 spreaders
    for (size_t p : gen1_spreaders) {
      auto person = r0pop.agent(p);
      const Duration spr_duration = person.duration();
      const Variant spr_variant = person.variant();
      const float sendrisk = infectparams[idx(spr_variant)].sendrisk[spr_duration];
      if (sendrisk <= 0.0f) continue;
      if (person.status() != INFECTIOUS) continue;

      const size_t infected = spread_and_count(r0pop, r0series, person, socialparams,
                              infectparams, vaxset, dovax, contacts, density_factor, indoor_seq);
      r0_infected += infected;
    }

    // daily progression for spreaders: to get better or die
    for (size_t p : gen1_spreaders) {
      auto person = r0pop.agent(p);
      progression(person, r0series, progressionset, infectparams, probvec, dovax, vaxset);
    }
}
  fmt::println("r0 infected: {}", r0_infected);
  double r0 = static_cast<double>(r0_infected) / static_cast<double>(gen1_spreader_cnt);

  // Restore the global day counter.
  sim::current_day = saved_day;
  sim::ds.day = saved_day;

  return r0;
}

//
// simulation for rt with context of actual simulation
//
std::optional<double> run_rt_sim(Model & model, PopData & rtpop, Variant variant, int scale, vector<float> indoor_seq) {

  // Snapshot the global day counter so a midstream simulation (future use)
  // isn't disturbed. We drive the R0 day loop starting from day 1.
  // const int saved_day = sim::current_day;
  // sim::reset_day();

  // deref or create arguments needed for spread
  int locale = model.locale;
  bool dovax = model.dovax;         
  VaxSet & vaxset = model.mp.vaxset;  
  ProgressionSet & progressionset = model.mp.progressionset;
  vector<InfectParams> & infectparams = model.mp.infectparams;  
  SocialParams & socialparams = model.mp.socialdata;

  // access density factor for current locale
  auto locale_pos = find(model.mp.geodata.fips.begin(), model.mp.geodata.fips.end(), model.locale);
  if (locale_pos == model.mp.geodata.fips.end()) {
    throw std::runtime_error("Invalid locale input: " + std::to_string(model.locale) + ". Must match a locale from geodata.");
  }
  auto locale_idx = locale_pos - model.mp.geodata.fips.begin();

  float density_factor = model.mp.geodata.density[locale_idx];


  // keep track of spreaders and outcomes
  AllSeries rtseries(DURATIONLIM, rtpop, Variant::names.size(), 1, 1);  
  // keep track of the other infected people and throw it away
  AllSeries throwaway_series(DURATIONLIM, rtpop, Variant::names.size(), 1, 1);
  vector<size_t> contacts(250); // reserve and set size, cleared before later usage
  array<float, 6> probvec{};
  vector<double> age_dist = model.age_dist;

  vector<size_t> gen1_spreaders{};  // mutated in place


  seed_gen1(gen1_spreaders, rtpop, rtseries, age_dist, variant, scale);
  const size_t gen1_spreader_cnt = gen1_spreaders.size();
  if (gen1_spreader_cnt == 0) {
    return std::nullopt;  // will result in neg. message in caller, but simulation can continue
  }

  size_t rt_infected = 0;

  //
  // mini simulation loop
  //
  for (size_t d=1; d <= DURATIONLIM; ++d) {   // day loop
    // start a new day for the side loop of DURATIONLIM DAYS.
    sim::incr_day();
    sim::ds.day = sim::get_day();
    rtseries.init_history_series(d);

    // loop over gen 1 spreaders
    for (size_t p : gen1_spreaders) {
      auto person = rtpop.agent(p);
      const Duration spr_duration = person.duration();
      const Variant spr_variant = person.variant();
      const float sendrisk = infectparams[idx(spr_variant)].sendrisk[spr_duration];
      if (sendrisk <= 0.0f) continue;
      if (person.status() != INFECTIOUS) continue;

      spread(rtpop, rtseries, person, socialparams, infectparams, vaxset,
              dovax, contacts, density_factor, indoor_seq, model.sd_cases,
              model.mp.ringtraits, model.ring_members, model.ring_lengths);
      rt_infected += rtseries.new_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[d];
    }

    // daily progression for spreaders: to get better or die
    for (size_t p : gen1_spreaders) {
      auto person = rtpop.agent(p);
      progression(person, rtseries, progressionset, infectparams, probvec, dovax, vaxset);
    }
  

    // now simulate the rest of the population to provide realistic context
    // ALSO exclude the seeded spreaders!
    for (size_t p = 1; p <= rtpop.popn; ++p) {

      // get an agent at index p
      auto person = rtpop.agent(p);
      if (person.status() != INFECTIOUS || person.sickday() >= sim::ds.day || std::ranges::contains(gen1_spreaders, p)) continue;

      // spread kernel
      auto spr_duration = person.duration();      
      auto spr_variant = person.variant();    
      auto sendrisk = infectparams[idx(spr_variant)].sendrisk[spr_duration];
      if (sendrisk > 0.0) {
        // sim::ds.starting_spreaders++;
        spread(rtpop, throwaway_series, person, socialparams, infectparams, vaxset,
                dovax, contacts, density_factor, model.indoor_seq, model.sd_cases,
                model.mp.ringtraits, model.ring_members, model.ring_lengths);
      }

      // progression kernel
      progression(person, throwaway_series, progressionset, infectparams, probvec, dovax, vaxset);

    } // end persons loop

  }


  fmt::println("rt infected: {}", rt_infected);
  double rt = static_cast<double>(rt_infected) / static_cast<double>(gen1_spreader_cnt);

  // // Restore the global day counter.
  // sim::current_day = saved_day;
  // sim::ds.day = saved_day;

  return rt;
}

}  // end namespace