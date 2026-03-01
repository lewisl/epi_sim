#include "lib_includes.h"

#include "population.h"
#include "parameters.h"
#include "sim.h"
#include "progression.h"
#include "random.h"
#include "disease_modeling.h"

/*
    progression(PopData &pop, size_t p, ProgressionSet &progset, vaxset, vector<float> probvec)

People who have become infected progress through conditions from
nil (asymptomatic) to mild to sick to severe, depending on their
agegroup, days of being exposed, and some probability. Finally,  
they move to recovered or dead. Note: p is the contacted person.
*/      // before probvec InfectParams &infpar, 
void progression(PopData &pop, size_t p, ProgressionSet &progset, vector<InfectParams> infectparams,
                 array<float, 6> &probvec,
                 bool dovax, VaxSet vaxset) {
  auto today = sim::get_day();

  // extract traits for current person
  auto & p_duration = pop.duration[p];
  const auto p_agegrp = pop.agegrp[p];
  auto & p_cond = pop.cond[p];
  auto & p_status = pop.status[p];
  const auto p_vaxstatus = pop.vaxstatus[p];  // won't update this
  auto p_recovday = pop.get_recovday(p); // will be changed my make_well
  auto p_variant = pop.get_variant(p);
  // get scope to function level
  float risk{};
  float recoveff{};

  // just for debugging output
  // fmt::println("");
  // fmt::println("Before cond: {} duration: {} status: {}", p_cond, p_duration, p_status);

  auto &probtree = progset.progression[p_variant].tree;

  if (probtree.at(p_agegrp - 1).contains(p_duration)) {
    const auto& src = probtree[p_agegrp - 1][p_duration][p_cond - 1];  // compile time pointer
    std::copy(src.begin(), src.end(), probvec.begin());  // make a copy so we can alter this person's probs

    recoveff = recoveffect(pop, today, p, p_variant, infectparams);                            
                                                    
    float vaxeff = 1.0f; // more to do!

    risk = riskfactor(recoveff, vaxeff);
    redistribute_probability(probvec, risk, p_duration);
    do_progression(pop, p, probvec);
    
  } else {   // not at a break point; no pr array applies
    ++p_duration;
  };

  // just for debugging output
  if (recoveff < 1.0f && recoveff > 0.0f)
    fmt::println("After  cond: {} duration: {} status: {} recoveff: {}", p_cond, p_duration, p_status, recoveff);


}

/*
Redistribute the probabilty of progressing through conditions based on
vaccination, recovery from prior infection and the variant of the patient.
*/
void redistribute_probability(array<float, 6> &probvec, float riskfactor, uint8_t duration) {
    float tot_excess = 0.0f;
    for (auto to_idx : {Progressmap::ToSick, Progressmap::ToSevere, Progressmap::ToDead}) {
        float excess = probvec[to_idx] * (1.0f - riskfactor);
        probvec[to_idx] -= excess;
        tot_excess += excess;
    }

    if (duration == DURATIONLIM) {
        probvec[Progressmap::ToRecover] += tot_excess;
    } else {
        tot_excess /= 3.0f;
        for (auto to_idx : {Progressmap::ToRecover, Progressmap::ToNil, Progressmap::ToMild}) {
            probvec[to_idx] += tot_excess;
        }
    }
}

/*
    doprogression!(PopData &pop, size_t p, const vector<float> &probvec)

Progress an infected person to a new condition or status if called
with a progression array (trvec) or increment
the number of days the person has been sick.
*/
void do_progression(PopData &pop, size_t p, const array<float,6> &probvec) {

  uint8_t outcome = xo::categorical_fast(probvec);  // range is 0..5

  if (outcome == Progressmap::ToDead) {  // for outcome == 5
    pop.deadday[p] = sim::get_day();
    pop.status[p] = Stat::Dead;   // todo will we need to set cond to uninfected for any other logic?
  } else if (outcome == Progressmap::ToRecover) {     // for outcome == 0
    pop.make_well(p);
  } else {
    pop.cond[p] = static_cast<Condition>(outcome);  // turns out this works because conds are 1..4 in Progressmap
    ++pop.duration[p];
  }
}

float riskfactor(float recoveff, float vaxeff) {
  return std::clamp(std::min(recoveff, vaxeff), 0.0f, 0.97f);}
