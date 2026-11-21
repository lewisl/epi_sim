#pragma once


#include "population.h"
#include "parameters.h"
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
    struct DayCounter {
      int base = 0;   // 0 for Mainline/R0; (outer_day - 1) for Rt
      int day  = 0;   // local day: 0 for seed_gen1, then 1..ndays or 1..DURATIONLIM

      int  get() const { return base + day; }  // real/parallel-universe day
      int  alt() const { return day; }         // local 1..ndays or 1..DURATIONLIM index
      void inc(int n = 1) { day += n; }
      void reset() { base = 0; day = 0; }
      void start_from(const DayCounter& outer) {
        base = outer.get() - 1;
        day  = 0;
      }
    };

    template <typename Tag>
    struct Counter {
      inline static DayCounter state{};
    };

    struct Mainline {};
    struct R0 {};
    struct Rt {};

    inline DayCounter* active = &Counter<Mainline>::state;
    inline bool debug = false;
    // Instrumentation only; reset at the beginning of each main simulation.
    inline Timing history_timing{};

    inline int get_day() {
        return active->get();
    }

    inline int alt_day() {
        return active->alt();
    }

    inline void incr_day() { active->inc(); }

    inline void reset_day() { active->reset(); }

    template <typename Tag>
    struct UseCounter {
      DayCounter* prev;
      UseCounter() : prev(active) { active = &Counter<Tag>::state; }
      ~UseCounter() { active = prev; }
      UseCounter(const UseCounter&) = delete;
      UseCounter& operator=(const UseCounter&) = delete;
    };

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
Histories runsim(Model& model);

// post_simulation plotting and serialization
void post_simulation(Model & model, Histories & histories);
