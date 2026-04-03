#pragma once

#include "lib_includes.h"

#include "population.h"
#include "parameters.h"
#include "random.h"
#include "series.h"
namespace fs = std::filesystem;

// Forward declaration
struct Model;
struct HistorySeries;

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

    struct daystats {
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

// A single trait column reference + its expected or new value.
// trait is the AgentView accessor name (e.g. "status", "agegrp", "cond", "variant").
// val is the integer representation of the value, parsed at load time.
struct Term {
  string trait;
  int32_t val;
};

// Filter: all terms must match (AND semantics) for a person to be a candidate.
// Allowed traits: "status", "agegrp", "cond", "variant", "vaxstatus", "quar", "tested"
struct Filter {
  vector<Term> terms;
};

// Change: the trait values to set on each matched candidate, and how many to affect.
// Allowed traits: "status", "cond", "duration", "variant", "vaxstatus", "quar"
// When "status"="infectious" is present, make_sick is called (preserving all invariants).
// Guard: only Stat::Unexposed and Stat::Recovered persons may be made sick.
struct Change {
  vector<Term> terms;
  int count{0};
};

struct SeedCase {
  PopData& pop;
  int triggerday{0};    // simulation day the changes are applied
  bool startofday{true}; // true = beginning of day, false = end of day
  Filter filter;
  Change change;

  SeedCase(int triggerday, bool startofday, Filter filt, Change chg, PopData& pop)
      : triggerday(triggerday), startofday(startofday),
        filter(std::move(filt)), change(std::move(chg)), pop(pop) {}
  SeedCase() = delete;

  vector<size_t> operator()(HistorySeries& series);
};

struct SummaryData {
  std::array<int, 7> unexposed{};
  std::array<int, 7> infected{};
  std::array<int, 7> reinfected{};
  std::array<int, 7> recovered{};
  std::array<int, 7> dead{};
  // index 1..5 = age groups, index 6 = total, index 0 unused
};

// Load seed cases from a parsed JSON array; requires ModelParams for variant lookup.
vector<SeedCase> load_seed_cases(const json& jdata, PopData& pop, const ModelParams& mp);

// Simulation runner
void runsim(Model& model, vector<SeedCase> seedcases);
