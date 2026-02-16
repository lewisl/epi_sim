#pragma once

#include "lib_includes.h"

#include "population.h"
#include "parameters.h"
#include "random.h"

// parameters used throughout the simulation that are better global
//    than passed into every function
namespace sim {
    inline int current_day = 0;
    
    inline int get_day() { 
        return current_day; 
    }
    
    inline void increment_day() { 
        ++current_day; 
    }
}

struct SeedFilter {
  uint8_t agegrp;     //filter criterion
  uint8_t condition;  // update value
  uint8_t duration;   // update value
  uint8_t variant;    // update value
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
          pop.make_sick(i, filt.variant, filt.condition, filt.duration );
          seeded_persons.push_back(i);
          count_of_seeds++;
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
