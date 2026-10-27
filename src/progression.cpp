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

  probvec is 0-indexed 0..5
  */
  void redistribute_probability(array<float, 6> &probvec, float riskfactor, uint8_t duration) {
      float tot_excess = 0.0f;
      //                           4                  5                   6
      for (auto to_idx : {Progressmap::ToSick, Progressmap::ToSevere, Progressmap::ToDead}) {
          float excess = probvec[(std::to_underlying(to_idx))] * (1.0f - riskfactor);
          probvec[std::to_underlying(to_idx)] -= excess;
          tot_excess += excess;
      }

      if (duration == DURATIONLIM) {
        probvec[std::to_underlying(Progressmap::ToRecover)] += tot_excess;  // on the last day, do we want to skew towards survival?
      } else {
        tot_excess /= 3.0f;
        //                           1                     2                   3
        for (auto to_idx : {Progressmap::ToRecover, Progressmap::ToNil, Progressmap::ToMild}) {
            probvec[(std::to_underlying(to_idx))] += tot_excess;
        }
      }
  }

  /*
      doprogression!(PopData &pop, size_t p, const vector<float> &probvec)

  Progress an infected person to a new condition or status if called
  with a progression probability array, or increment the number of days
  the person has been sick.
  */
  void do_progression(AgentView person, Histories& histories,
                      const array<float, 6>& probvec, size_t today) {

    uint8_t outcome = xo::categorical_fast(probvec);  // adjust range to 1-indexing

    if (outcome == std::to_underlying(Progressmap::ToDead)) {  

      person.make_dead(histories);

    } else if (outcome == std::to_underlying(Progressmap::ToRecover)) {     

        person.make_well(histories);
        
    } else {   // valid outcomes are 1..6. we need 2..5 mapped to 1..4
        person.cond() = static_cast<Condition>(outcome) + uint8_t{1};  // this ONLY works because conds are 1..4 in Progressmap
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
void progression(AgentView person, Histories& histories, const ProgressionSet& progset,
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
      // Transition rows represent the current infected condition: nil through severe.
      // Condition IDs 0 (error) and 1 (uninfected) have no rows.
      // Subtract 2 to map condition IDs 2..5 to matrix rows 0..3.
      auto probvec = tree.entries[static_cast<size_t>(entry_idx)][person.cond() - uint8_t{2}];

      const float recoveff = recoveffect(person, today, p_variant, infectparams);
      float vaxeff = 1.0f;
      if (dovax && p_vaxstatus != Vaxstat::none) {
        vaxeff = vaxeffect(today, person, vaxset, p_variant);
      }

      const float risk = riskfactor(recoveff, vaxeff);
      redistribute_probability(probvec, risk, duration);
      do_progression(person, histories, probvec, today);
    } else {
      ++person.duration();
    }
}
