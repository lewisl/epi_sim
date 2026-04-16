#pragma once

#include "lib_includes.h"

#include "population.h"
#include "parameters.h"
#include "random.h"
#include "series.h"
#include "cases.h"

namespace fs = std::filesystem;

// Forward declaration
struct Model;

struct Config {
  int days {180};
  int locale {38015};
  string calendar_start {"2020-01-01"};
  bool dovax {false};
  bool debug {false};
  fs::path geodata {"geo2data.csv"};
  fs::path variants {"variants.json"};
  fs::path social {"socialparams.json"};
  fs::path vaccines {"vaccines.json"};
  fs::path vax_sched_dir {"vaccine_100k"};
};

// parameters used throughout the simulation that are better global
//    than passed into every function
namespace sim {
    inline int current_day = 0;
    inline bool debug = false;

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
void runsim(Model& model, vector<SeedCase> seedcases);
