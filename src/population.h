// population: organize data about each person in the locale

#pragma once

#include "lib_includes.h"


#include "helpers.h"
#include "parameters.h"

// forward declarations
struct AllSeries;
class PopData;
struct AgentView;

using std::array;
using std::string;
using std::vector;

// control constants
// const vector<double> AGE_DIST = {0.251, 0.271, 0.255, 0.184, 0.039};
const uint8_t DURATIONLIM { 25 };   // maximum length of illness in days for anyone
const std::pair<int, int> DURATIONS  {1, DURATIONLIM};

// setup for numeric and string access to PopData columns
enum class ColumnName : uint8_t {
  status, agegrp, cond, duration, variant, variant_hist,
  sickday, sickday_hist, recovday, recovday_hist, deadday,
  ring, sdcase, testday_hist, testday,
  quar, quarday, vaxstatus, vax, vax_hist, vaxday, vaxday_hist,
  COUNT
};

inline constexpr std::array<std::string_view, size_t(ColumnName::COUNT)> column_name_labels{
    "status", "agegrp", "cond", "duration", "variant", "variant_hist",
    "sickday", "sickday_hist", "recovday", "recovday_hist", "deadday",
    "ring", "sdcase", "testday_hist", "testday",
    "quar", "quarday", "vaxstatus", "vax", "vax_hist", "vaxday", "vaxday_hist"};

constexpr std::string_view to_string(ColumnName name) {
  return column_name_labels[size_t(name)];
}

class PopData {
  public:
    std::size_t popn; // actual population size
    std::size_t popz; // popn+1: array sizing for 1 indexing all vectors
    vector<int> agegrp_parts; // apportioned population counts for age0_19..age80_up
    vector<size_t> all_idx;

    // constructor
    // clang-format off
    PopData(size_t n, const vector<double>& age_dist)
          : popn(n), popz(n+1), 
            agegrp_parts(apportion(n, vector<float>(age_dist.begin(), age_dist.end()))),
            all_idx([n](){std::vector<size_t> v; v.reserve(n); 
                          for (size_t i = 1; i <= n; ++i) v.push_back(i); return v;}()),
            status(n+1, UNEXPOSED),
            agegrp(age_distribution(n, agegrp_parts)), // assign realized age buckets from apportioned counts
            cond(n+1, UNINFECTED), duration(n+1, Duration{0}),
            variant(n+1), variant_hist(n+1), sickday(n+1, Sickday{0}), sickday_hist(n+1),
            recovday(n+1, Recovday{0}), recovday_hist(n+1), deadday(n+1, Deadday{0}), ring(n+1, Ring{0}),
            sdcase(n+1), testday(n+1, Testday{0}), testday_hist(n+1), quar(n+1, 0), quarday(n+1, Quarday{0}),
            vaxstatus(n+1, Vaxstat::none), vax(n+1), vax_hist(n+1), vaxday(n+1, Vaxday{0}), vaxday_hist(n+1) 
          {
              if (n == 0) {
                  throw std::invalid_argument(
                      "Bad size input. Must be a positive integer.");
              }
          }
      // clang-format on

  // THE VECTORS
  // vectors of pseudo enums as uint8_t or trait classes or
  // vectors of vector for repeating count happening to a person
  vector<Status> status; // default unexposed = 1
  vector<Agegrp> agegrp;
  vector<Condition> cond;
  vector<Duration> duration;
  vector<Variant> variant;
  vector<VariantHist> variant_hist;
  vector<Sickday> sickday;
  vector<SickdayHist> sickday_hist;
  vector<Recovday> recovday;
  vector<RecovdayHist> recovday_hist;
  vector<Deadday> deadday;
  vector<Ring> ring;
  vector<SDCase> sdcase;
  vector<Testday> testday;
  vector<TestdayHist> testday_hist;
  vector<uint8_t> quar; // pseudo bool
  vector<Quarday> quarday;
  vector<Vaxstatus> vaxstatus;  
  vector<Vax> vax;
  vector<VaxHist> vax_hist;
  vector<Vaxday> vaxday;
  vector<VaxdayHist> vaxday_hist;

  
  AgentView agent(std::size_t i);

    vector<Agegrp> age_distribution(int popn, const vector<int>& counts) {
      assert(counts.size() == 5);

      // +1 for 1-based indexing: index 0 stays UNKNOWN (unused)
      vector<Agegrp> agegrp(popn + 1, UNKNOWN);

      int start_idx{1};
      size_t age_idx = 0;
      for (auto this_age : {AGE0_19, AGE20_39, AGE40_59, AGE60_79, AGE80_UP}) {
        int num_of_age = counts[age_idx++];  // pair each age with its own count (not nested)
        fill(agegrp.begin() + start_idx, agegrp.begin() + start_idx + num_of_age, this_age);
        start_idx += num_of_age;
      }

      return agegrp;
    }
    
    vector<int> apportion(int n, vector<float> splits) {
      assert(n > 0);
      assert(splits.size() > 0);
      assert(approx_equal(std::accumulate(splits.begin(), splits.end(), 0.0), 1.0));

      // std::cout << "\n=== APPORTION DEBUG (n=" << n << ") ===\n";

      vector<int> parts;
      for (size_t i = 0; i < splits.size(); ++i) {
        int part = static_cast<int>(round(n * splits[i]));
        parts.push_back(part);
        // std::cout << "  parts[" << i << "] = round(" << n << " * " << splits[i]
        //           << ") = " << part << "\n";
      }

      // // Calculate total before adjustment
      int total_before = std::accumulate(parts.begin(), parts.end(), 0);
      // std::cout << "Total before adjustment: " << total_before << "\n";

      // // fix rounding error
      int diff = total_before - n;
      // std::cout << "Difference (total - n): " << diff << "\n";

      if (diff != 0) {
        // std::cout << "Adjusting parts.back() from " << parts.back()
        //           << " to " << (parts.back() - diff) << "\n";
        parts.back() -= diff;
      }

      assert(parts.back() > 0);  // should always be true for large n and reasonable splits

      return parts;
    }

};


/* 
  lazy access to "rows" across the vectors.  AgentView is a reference to vector/columns
  at one index value.  No row is materialized. We only pay when we access something:
    create: auto a = pop.agent(i)
    use:   if (a.status() == UNEXPOSED) ...

    to define a function that takes a "row" as its argument: 
          `float risk(AgentView agent) {
              if (agent.agegrp() == AGE80_UP) ...;
            }`
    to call this function:
      auto a = pop.agent(i);
      float risk_factor = risk(a); 

  */
  struct AgentView {
    private:
      PopData& pop;
      std::size_t i;  // the index to semantic rows and elements of the vectors
      friend class PopData;  // allows PopData methods to access private members and methods

      // Private constructor
      AgentView(PopData &p, std::size_t index) : pop(p), i(index) {
        if (i < 1 || i > pop.popn) throw std::runtime_error("Input i must be > 1 and less than number of people.");
        }

    public:
      //a method for every vector in PopData that returns its element type
      // reference return value means these are Lvalues: read and write to the source vectors in PopData
      const size_t id = i; 
      Status & status() { return pop.status[i]; }
      Agegrp & agegrp() { return pop.agegrp[i]; }
      Condition & cond() { return pop.cond[i]; }
      Duration & duration() { return pop.duration[i]; }
      Variant & variant() { return pop.variant[i]; }
      VariantHist & variant_hist() { return pop.variant_hist[i]; }
      Sickday &sickday() { return pop.sickday[i]; }
      SickdayHist & sickday_hist() { return pop.sickday_hist[i]; }
      // int16_t get_sickday() { return static_cast<int16_t>(pop.sickday[i]); }
      Recovday &recovday() { return pop.recovday[i]; }
      RecovdayHist & recovday_hist() { return pop.recovday_hist[i]; }
      Deadday &deadday() { return pop.deadday[i]; }
      Ring &ring() { return pop.ring[i]; }
      SDCase &sdcase() { return pop.sdcase[i]; }
      Testday &testday() { return pop.testday[i]; }
      TestdayHist & testday_hist() { return pop.testday_hist[i]; }
      uint8_t &quar() { return pop.quar[i]; }  // pseudo bool
      Quarday &quarday() { return pop.quarday[i]; }
      Vaxstatus &vaxstatus() { return pop.vaxstatus[i]; }
      Vax &vax() { return pop.vax[i]; }
      VaxHist &vax_hist() { return pop.vax_hist[i]; }
      Vaxday &vaxday() { return pop.vaxday[i]; }
      VaxdayHist &vaxday_hist() { return pop.vaxday_hist[i]; }
      // end of column related methods

      //
      // methods defined in disease_modeling.cpp as AgentView::make_well,etc.
      // change status of 1 person, keeping all traits consistent. the person is the object: person.make_well(series)
      void make_sick(Variant var, AllSeries & series, Condition condition = NIL, uint8_t durationdays = 1);
      void make_well(AllSeries & series);
      void make_dead(AllSeries & series);  
      
	  };  // end of struct AgentView

// using ShowFn = std::string (AgentView::*)() const;


inline AgentView PopData::agent(std::size_t i) {
  return AgentView{*this, i};
}
