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
void progression(PopData &pop, size_t p, DayData & series, ProgressionSet &progset, vector<InfectParams> &infectparams,
                 array<float, 6> &probvec, bool dovax, VaxSet &vaxset) {
  auto today = sim::get_day();
  auto person = pop.agent(p);
  // extract traits for current person
  // won't update these
  const auto p_agegrp =  person.agegrp(); // pop.agegrp[p]; //
  const auto p_variant = person.get_variant();  // pop.get_variant(p); // 
  const auto p_vaxstatus = person.vaxstatus();  // pop.vaxstatus[p]; //
  // progression may update these--only duration is modified in this function!
  auto & p_cond = person.cond();  // pop.cond[p];          
  auto & p_status = person.status(); // pop.status[p];   
  auto  p_recovday = person.get_recovday(); // pop.get_recovday(p);      
  auto & p_duration = person.duration(); // pop.duration[p];  

  // set scope to function level
  float risk{};
  float recoveff{};

  // just for debugging output
  // fmt::println("");
  // fmt::println("Before cond: {} duration: {} status: {}", p_cond, p_duration, p_status);

  const auto &probtree = progset.progression[p_variant].tree;
  const auto & age_map = probtree.at(zidx(p_agegrp));
  if (auto it = age_map.find(p_duration + 1); it != age_map.end()) {
    const auto& src = it->second[zidx(p_cond)];  
    std::copy(src.begin(), src.end(), probvec.begin());  // make a copy so we can alter this person's probs

    recoveff = recoveffect(pop, today, p, p_variant, infectparams);                            
                                                    
    float vaxeff = 1.0f; // more to do!

    risk = riskfactor(recoveff, vaxeff);
    redistribute_probability(probvec, risk, p_duration+1);
    do_progression(person, series, probvec);

    // print diagnostics for progression to dead
    // if (p_agegrp == Age::Age80_up && p_cond == Cond::Severe && (p_duration == 18 || p_duration == 24)) {
    //   fmt::println(" recoveff {}, risk {}, before probs {}, after probs {}",
    //                recoveff, risk, src, probvec);
    // }
    
  } else {   // not at a break point; no pr array applies
    ++p_duration;
  };

  // just for debugging output
  // if (recoveff < 1.0f && recoveff > 0.0f)
  //   fmt::println("After  cond: {} duration: {} status: {} recoveff: {}", p_cond, p_duration, p_status, recoveff);


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
void do_progression(PopData::AgentView person, DayData & series, const array<float,6> &probvec) {  // PopData &pop, size_t p

  uint8_t outcome = xo::categorical_fast(probvec);  // range is 0..5

  if (outcome == Progressmap::ToDead) {  // for outcome == 5
    person.make_dead(series);
    // person.deadday() = sim::get_day(); //pop.deadday[p] = sim::get_day();
    //   person.status() = Stat::Dead; // pop.status[p] = Stat::Dead; // todo will we need to set cond to uninfected for any other logic?
    //   sim::ds.num_died++;
  } else if (outcome == Progressmap::ToRecover) {     // for outcome == 0
      person.make_well(series);
      sim::ds.num_recovered++;
  } else {
      person.cond() = static_cast<Condition>(outcome); // pop.cond[p] = static_cast<Condition>(outcome);  // this ONLY works because conds are 1..4 in Progressmap
      ++person.duration(); // ++pop.duration[p];
  }
}

float riskfactor(float recoveff, float vaxeff) {
  return std::clamp(std::min(recoveff, vaxeff), 0.0f, 1.0f);}
