#include "population.h"
#include "parameters.h"
#include "sim.h"
#include "progression.h"
#include "random.h"
#include "disease_modeling.h"


namespace {
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
  with a progression probability array, or increment the number of days
  the person has been sick.
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
  
}  // anonymous namespace


/*
People who have become infected progress through conditions from
nil (asymptomatic) to mild to sick to severe, depending on their
age group, illness duration, and transition probabilities. Outcomes
can also be recovery or death.
*/
void progression(AgentView person, AllSeries& series, const ProgressionSet& progset,
                 vector<InfectParams>& infectparams, bool dovax,
                 const VaxSet& vaxset) {

    const auto today = sim::get_day();

    // Extract traits that this function does not update before selecting an outcome.
    const auto p_variant = person.variant();
    const auto p_vaxstatus = person.vaxstatus();
    const auto duration = person.duration();

    const auto& tree = progset.progression[p_variant].tree;
    const int16_t entry_idx =
        tree.entry_index[zidx(person.agegrp())][static_cast<size_t>(duration)];
    if (entry_idx != NO_PROGRESSION_ENTRY) {
      // The source table remains immutable; this person's protection adjustments
      // are applied to a local, allocation-free copy of the six outcomes.
      auto probvec = tree.entries[static_cast<size_t>(entry_idx)][zidx(person.cond())];

      const float recoveff = recoveffect(person, today, p_variant, infectparams);
      float vaxeff = 1.0f;
      if (dovax && p_vaxstatus != Vaxstat::none) {
        vaxeff = vaxeffect(today, person, vaxset, p_variant);
      }

      const float risk = riskfactor(recoveff, vaxeff);
      redistribute_probability(probvec, risk, duration);
      do_progression(person, series, probvec, today);
    } else {
      ++person.duration();
    }
}
