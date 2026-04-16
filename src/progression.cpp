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
*/ 
void progression(AgentView person, AllSeries & series, ProgressionSet &progset, vector<InfectParams> &infectparams,
                 array<float, 6> &probvec, bool dovax, VaxSet &vaxset) {
    auto today = sim::get_day();
    // extract traits for current person -- won't update these
    const auto p_variant = person.variant();   
    const auto p_vaxstatus = person.vaxstatus();  

    // set scope to function level
    float risk{};
    float recoveff{};

    const auto &probtree = progset.progression[p_variant].tree;
    const auto & age_probs = probtree.at(zidx(person.agegrp()));
    if (auto it = age_probs.find(person.duration()); it != age_probs.end()) {
      const auto& src = it->second[zidx(person.cond())];  // zidx copies the input value
      std::copy(src.begin(), src.end(), probvec.begin());  // make a copy so we can alter this person's probs

      recoveff = recoveffect(person, today, p_variant, infectparams);                            
                                                      
      float vaxeff = 1.0f;

      risk = riskfactor(recoveff, vaxeff);
      if (dovax && p_vaxstatus != Vaxstat::none) {
        vaxeff = vaxeffect(today, person, vaxset, p_variant);
      }

      risk = riskfactor(recoveff, vaxeff);
      redistribute_probability(probvec, risk, person.duration());  // update probvec in place
      do_progression(person, series, probvec, today);  // update person in place even passed as value--change is in row of PopData
      
    } else {   // not at a break point; no probvec applies
      ++person.duration();
    }
}

/*
Redistribute the probabilty of progressing through conditions based on
vaccination, recovery from prior infection and the variant of the patient.
The point here is that partial immunity should also reduce disease severity.
If no partial immunity then nothing should change.
*/
void redistribute_probability(array<float, 6> &probvec, float riskfactor, uint8_t duration) {
    float tot_excess = 0.0f;
    for (auto to_idx : {Progressmap::ToSick, Progressmap::ToSevere, Progressmap::ToDead}) {
        float excess = probvec[to_idx] * (1.0f - riskfactor);
        probvec[to_idx] -= excess;
        tot_excess += excess;
    }

    if (duration == DURATIONLIM) {
      probvec[Progressmap::ToRecover] += tot_excess;  // on the last day, do we want to skew towards survival?
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
void do_progression(AgentView person, AllSeries & series, const array<float,6> &probvec, size_t today) {  // PopData &pop, size_t p

  uint8_t outcome = xo::categorical_fast(probvec);  // range is 0..5

  if (outcome == Progressmap::ToDead) {  // for outcome == 5

    person.make_dead(series);  // pass the series vectors to update the simulation history

  } else if (outcome == Progressmap::ToRecover) {     // for outcome == 0

      person.make_well(series);
      
  } else {
      person.cond() = static_cast<Condition>(outcome);  // this ONLY works because conds are 1..4 in Progressmap
      ++person.duration(); 
  }
}

float riskfactor(float recoveff, float vaxeff) {
  return std::clamp(std::min(recoveff, vaxeff), 0.0f, 1.0f);}
