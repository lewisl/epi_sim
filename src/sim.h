#pragma once

#include "lib_includes.h"

#include "population.h"
#include "parameters.h"
#include "random.h"
#include "series.h"
namespace fs = std::filesystem;

// Forward declaration
struct Model;
struct DayData;

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

  vector<size_t> operator()(DayData & series) {
    vector<size_t> seeded_persons;
    size_t count_of_seeds = 0;
    for (auto filt : filtervec) {
      count_of_seeds = 0;
      for (int i = 1; count_of_seeds < filt.count && i <= pop.popn; ++i) {
        auto seed_person = pop.agent(i);
        if (seed_person.agegrp() == filt.agegrp) {
          seed_person.make_sick(filt.variant, series, filt.condition, filt.duration );
          seeded_persons.push_back(i);
          ++count_of_seeds;
        }
      }
    }
    if (seeded_persons.empty())
      std::cerr << "No row found matching filter criteria\n";
    return seeded_persons;
  }
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

// Simulation runner function
void runsim(Model& model, vector<SeedCase> seedcases);
