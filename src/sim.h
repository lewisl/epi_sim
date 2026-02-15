#pragma once

#include "lib_includes.h"

#include "parameters.h"
#include "population.h"
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
  uint8_t agegrp;
  uint8_t condition;
  uint8_t duration;
  uint8_t variant;
  int8_t count;
};

struct SeedCase {
  PopData pop;
  int triggerday{0};  // day that seeding happens
  bool startofday{true}; //is it beginning of the day
  // agegrp, number of people, initial condition, initial duration, variant
  std::vector<SeedFilter> filtervec{};

  SeedCase(int triggerday, bool startofday, vector<SeedFilter> filt, PopData pop)
      : triggerday(triggerday), startofday(startofday), filtervec(filt), pop(pop) {}
  SeedCase() = delete; // default constructor not allowed

  void operator()() {
    for (auto filt : filtervec) {
      for (int i = 1; i <= pop.popn; ++i) {
        if (pop.agegrp[i] == filt.agegrp) {
          pop.cond[i] = filt.condition;
          pop.duration[i] = filt.duration;
          pop.variant[i][0] = filt.variant;
          pop.variant_count[i] = 1;
          pop.status[i] = Trait::Stat::infectious;
          pop.sickday[i][0] = sim::get_day();
        }
      }
    }
  }
};

// TODO move this to a new source file disease_modeling.cpp with it's fucking .h
void make_sick(PopData &pop, size_t p, uint8_t variant, uint8_t cond = Trait::Cond::nil) {
  pop.cond[p] = cond;
  pop.duration[p] = 0;
  if (pop.variant_count[p] < 15) {
    pop.variant_count[p]++;
    pop.variant[p][pop.variant_count[p]] = variant;
  } else {
    std::cerr << "Variant overflow for person " << p
              << ". Oldest variant lost.\n";
      std::cerr << "variant_count increased to " << pop.variant_count[p] << "\n";  
    std::shift_left(pop.variant[p].begin(), pop.variant[p].end(), 1);
    pop.variant[p].back() = variant;
  }
}