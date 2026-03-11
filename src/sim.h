#pragma once

#include "lib_includes.h"

#include "population.h"
#include "parameters.h"
#include "random.h"
#include "series.h"

// Forward declaration
struct Model;

// parameters used throughout the simulation that are better global
//    than passed into every function
namespace sim {
    inline int current_day = 0;

    inline int get_day() {
        return current_day;
    }

    inline void incr_day() { ++current_day; }

    inline void reset_day() { current_day = 0; }

    struct daystats {
      int day{};
      int starting_spreaders{};
      int num_contacts{};
      int num_touched{};
      int num_new_infected{};
      int num_died{};
      int num_recovered{};

      void reset() {
        starting_spreaders = 0;
        num_contacts = 0;
        num_touched = 0;
        num_new_infected = 0;
        num_died = 0;
        num_recovered = 0;
      }
    };

    inline daystats ds{};
}

struct SeedFilter {
  Agegrp agegrp;     //filter criterion
  Condition condition;  // update value
  uint8_t duration;   // update value
  Variant variant;    // update value
  int8_t count;       // number of people to update
};

struct SeedCase {
  PopData& pop;
  int triggerday{0};  // day that seeding happens
  bool startofday{true}; //is it beginning of the day
  // agegrp, number of people, initial condition, initial duration, variant
  std::vector<SeedFilter> filtervec{};

  SeedCase(int triggerday, bool startofday, vector<SeedFilter> filt, PopData& pop)
      : triggerday(triggerday), startofday(startofday), filtervec(filt), pop(pop) {}
  SeedCase() = delete; // default constructor not allowed

  vector<size_t> operator()() {
    vector<size_t> seeded_persons;
    size_t count_of_seeds = 0;
    for (auto filt : filtervec) {
      count_of_seeds = 0;
      for (int i = 1; count_of_seeds < filt.count && i <= pop.popn; ++i) {
        if (pop.agegrp[i] == filt.agegrp) {
          pop.make_sick(pop.agent(i), filt.variant, filt.condition, filt.duration );
          seeded_persons.push_back(i);
          ++count_of_seeds;
          // pop.cond[i] = filt.condition;
          // pop.duration[i] = filt.duration;
          // pop.variant[i][0] = filt.variant;
          // pop.variant_count[i] = 1;
          // pop.status[i] = Trait::Stat::infectious;
          // pop.sickday[i][0] = sim::get_day();
        }
      }
    }
    if (seeded_persons.empty())
      std::cerr << "No row found matching filter criteria\n";
    return seeded_persons;
  }
};

// Simulation runner function
void runsim(Model& model);
