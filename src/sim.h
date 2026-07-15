#pragma once

#include "lib_includes.h"

#include "population.h"
#include "parameters.h"
#include "random.h"
#include "series.h"
#include "cases.h"
#include "timing.h"

// Forward declaration
struct Model;

// Note: no defaults so we get errors for bad/incomplete inputs
struct Config {
  int days;
  int locale;
  string calendar_start {};
  std::string seed {};
  std::string social_dist {};
  bool dovax {false};
  bool do_social_distancing {false};
  bool do_rings {false};
  bool debug {false};
  int rt_sim_interval{};
  std::vector<double> age_dist{};
  std::string geodata {};
  std::string variants {};
  std::string social_params {};
  std::string vaccines {};
  std::string vax_sched_dir {};
  std::string rings {};  // empty = rings disabled
  std::string output_dir {};
  std::string case_label {};
};

// parameters used throughout the simulation that are better global
//    than passed into every function
namespace sim {
    inline int current_day = 0;
    inline bool debug = false;
    // Instrumentation only; reset at the beginning of each main simulation.
    inline Timing history_timing{};

    inline int get_day() {
        return current_day;
    }

    inline void incr_day() { ++current_day; }

    inline void reset_day() { current_day = 0; }

    struct daystats {  // a useful midstream debug tool: not being used; needs accumulators in the right places
      int day{};
      int starting_spreaders{};
      int num_contacts{};
      int num_touched{};
      int num_new_infected{};
      int num_died{};
      int num_new_recovered{};

      void reset() {
        starting_spreaders = 0;
        num_contacts = 0;
        num_touched = 0;
        num_new_infected = 0;
        num_died = 0;
        num_new_recovered = 0;
      }
    };

    inline daystats ds{};
}



struct SummaryData {
  std::array<int, 7> unexposed{};
  std::array<int, 7> infected{};
  std::array<int, 7> reinfected{};
  std::array<int, 7> recovered{};
  std::array<int, 7> dead{};
  // index 1..5 = age groups, index 6 = total, index 0 unused
};



// Simulation runner
void runsim(Model& model);
