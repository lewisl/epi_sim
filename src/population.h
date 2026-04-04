// population: organize data about each person in the locale

#ifndef POPULATION
#define POPULATION

#include "lib_includes.h"


#include "helpers.h"
#include "parameters.h"

// forward declarations
struct HistorySeries;

using std::array;
using std::string;
using std::vector;

// control constants
const vector<double> AGE_DIST = {0.251, 0.271, 0.255, 0.184, 0.039};
const uint8_t DURATIONLIM { 25 };   // maximum length of illness in days for anyone
const std::pair<int, int> DURATIONS  {1, DURATIONLIM};

// setup for numeric and string access to PopData columns
// sickday_count is intentionally omitted from the public column registry because
// the current model treats variant_count as the authoritative infection-history count.
enum class ColumnName : uint8_t {
  status, agegrp, cond, duration, variant, variant_count,
  sickday, recovday, recovday_count, deadday,
  ring, sdcase, tested, tested_count, testday,
  quar, quarday, vaxstatus, vaxrcvd, vax_count, vaxday,
  COUNT
};

inline constexpr std::array<std::string_view, size_t(ColumnName::COUNT)> column_name_labels{
    "status", "agegrp", "cond", "duration", "variant", "variant_count",
    "sickday", "recovday", "recovday_count", "deadday",
    "ring", "sdcase", "tested", "tested_count", "testday",
    "quar", "quarday", "vaxstatus", "vaxrcvd", "vax_count", "vaxday"};

constexpr std::string_view to_string(ColumnName name) {
  return column_name_labels[size_t(name)];
}

class PopData {
  public:
    std::size_t popn; // actual population size
    std::size_t popz; // popn+1: array sizing for 1 indexing all vectors
    vector<int> agegrp_parts; // apportioned population counts for Age0_19..Age80_up

    // constructor
    // clang-format off
    PopData(size_t n, MapEnum<uint8_t> vax_lbl, 
          MapEnum<uint8_t> true_false, MapEnum<int> Justint,
          const vector<double>& age_dist=AGE_DIST)  
          : popn(n), popz(n+1),
            agegrp_parts(apportion(n, vector<float>(age_dist.begin(), age_dist.end()))),
            status(n+1, Stat::Unexposed),
            agegrp(age_distribution(n, agegrp_parts)), // assign realized age buckets from apportioned counts
            cond(n+1, Cond::Uninfected), duration(n+1, 0),
            variant(n+1), variant_count(n+1), sickday(n+1), sickday_count(n+1),
            recovday(n+1), recovday_count(n+1), deadday(n+1, 0), ring(n+1, 0),
            sdcase(n+1, 0), tested(n+1), tested_count(n+1), testday(n+1), quar(n+1, 0), quarday(n+1, 0),
            vaxstatus(n+1, Vaxstat::none), vaxrcvd(n+1), vax_count(n+1), vaxday(n+1),
            vax_lbl(vax_lbl),  true_false(true_false), Justint(Justint) // vaxstatus_lbl(vaxstatus_lbl),
            // clang-format on
      {
          if (n <= 0) {
              throw std::invalid_argument(
                  "Bad size input. Must be a positive integer.");
          }
      }

  // THE VECTORS
  // vectors of pseudo enums as uint8_t or trait classes or
  // vectors of vector for repeating count happening to a person
  vector<Status> status; // default unexposed = 1
  vector<Agegrp> agegrp;
  vector<Condition> cond;
  vector<uint8_t> duration;
  vector<array<Variant, 16>> variant; 
  vector<std::uint8_t> variant_count;
  vector<array<int16_t, 16>> sickday;
  vector<std::uint8_t> sickday_count;
  vector<array<int16_t, 16>> recovday;
  vector<std::uint8_t> recovday_count;
  vector<int16_t> deadday;
  vector<uint8_t> ring;
  vector<uint8_t> sdcase;
  vector<array<uint8_t, 16>> tested; // pseudo bool 0 = false, 1 = true
  vector<std::uint8_t> tested_count;
  vector<array<int16_t, 16>> testday;
  vector<uint8_t> quar; // pseudo bool
  vector<int16_t> quarday;
  vector<Vaxstatus> vaxstatus;  
  vector<array<uint8_t, 16>> vaxrcvd;  // :none,  vaccine symbols  :Pfizer, :Moderna, :JnJ _
  vector<std::uint8_t> vax_count;
  vector<array<int16_t, 16>> vaxday;  // = vec of vec of sim day

  /* 
  lazy access to "rows" across the vectors.  AgentView is a reference to vector/columns
  at one index value.  No row is materialized. We only pay when we access something:
    create: auto a = pop.agent(i)
    use:   if (a.status() == Stat::Unexposed) ...

    to define a function that takes a "row" as its argument: 
          `float risk(PopData::AgentView agent) {
              if (agent.agegrp() == Agegrp::Age80_up) ...;
            }`
    to call this function:
      auto a = pop.agent(i);
      float risk_factor = risk(a); 

  A nested struct definition is implicitly a friend class:  needs to be passed pop in the constructor.
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

    public:  //must have a function for every vector in PopData
      // reference return value means these are Lvalues: read and write to the source vectors in PopData
      const size_t id = i; 
      Status & status() { return pop.status[i]; }
      Agegrp & agegrp() { return pop.agegrp[i]; }
      Condition & cond() { return pop.cond[i]; }
      std::uint8_t & duration() { return pop.duration[i]; }
      array<Variant, 16> & all_variants() { return pop.variant[i]; }
      Variant get_variant() {
        const auto variant_count = pop.variant_count[i];
          const auto &all_variants = pop.variant[i];
        if (variant_count == 0)
          return Variant{};
        else if (variant_count >= 16)
          return all_variants.back();
        else return all_variants[zidx(variant_count)];
        }
      std::uint8_t & variant_count() { return pop.variant_count[i]; }
      array<int16_t, 16> &all_sickdays() { return pop.sickday[i]; }
      int16_t get_sickday() {
        const auto variant_count = pop.variant_count[i];
        const auto &all_sickdays = pop.sickday[i];
        if (variant_count == 0)
          return 0;
        else if (variant_count >= 16)
          return all_sickdays.back();
        else return all_sickdays[zidx(variant_count)];
        }
      array<int16_t, 16> & all_recovdays() { return pop.recovday[i]; }
      int16_t get_recovday() {
        const auto recovday_count = pop.recovday_count[i];
        const auto & all_recovdays = pop.recovday[i];
        if (recovday_count == 0) return 0;
        else if (recovday_count >= 16) return all_recovdays.back();
        else return all_recovdays[zidx(recovday_count)];
      }
      std::uint8_t &recovday_count() { return pop.recovday_count[i]; }
      std::int16_t &deadday() { return pop.deadday[i]; }
      std::uint8_t &ring() { return pop.ring[i]; }
      std::uint8_t &sdcase() { return pop.sdcase[i]; }
      array<uint8_t, 16> &tested() { return pop.tested[i]; }
      uint8_t & tested_count() { return pop.tested_count[i];}
      array<int16_t, 16> &testday() { return pop.testday[i]; }
      uint8_t &quar() { return pop.quar[i]; }  // pseudo bool
      int16_t &quarday() { return pop.quarday[i]; }
      Vaxstatus &vaxstatus() { return pop.vaxstatus[i]; }
      array<uint8_t, 16> &vaxrcvd() { return pop.vaxrcvd[i]; }
      uint8_t &vax_count() { return pop.vax_count[i]; }
      array<int16_t, 16> &vaxday() { return pop.vaxday[i]; }
      const MapEnum<uint8_t> &vax_labels() { return pop.vax_lbl; }
      const MapEnum<uint8_t> &bool_labels() { return pop.true_false; }

      //
      // methods defined in disease_modeling.cpp as PopData::AgentView::make_well,etc.
      // change status of 1 person, keeping all traits consistent. the person is the object: person.make_well(series)
      void make_sick(Variant var, HistorySeries & series, Condition condition = Cond::Nil, uint8_t durationdays = 1);
      void make_well(HistorySeries & series);
      void make_dead(HistorySeries & series);  
      
	  };  // end of struct AgentView

  struct PopColumnSpec {
    ColumnName name;
    std::string_view key;
    std::string (*to_txt_cell)(AgentView person);
  };

  using PopColumnMap = absl::flat_hash_map<std::string_view, PopColumnSpec>;

  //
  // enables easy use of AgentView with any instance variable of class PopData
  //     called with an instance of PopData because it's a PopData method:   auto a = pop.agent(i)
  // encapsulates the person's PopData index. allows fast access to "column" values
  AgentView agent(std::size_t i) { return AgentView{*this, i}; }


  // domains of valid values for columns
      // what is the stub to use for int valued columns that print as ints?
  MapEnum<uint8_t> vax_lbl;
  MapEnum<int> Justint;
  MapEnum<uint8_t> true_false;

  static std::optional<ColumnName> column_name_from_string(std::string_view text);
  static const PopColumnMap& column_map();
  static const PopColumnSpec* find_column(ColumnName name);
  static const PopColumnSpec* find_column(std::string_view key);
  static std::vector<const PopColumnSpec*> resolve_columns(std::span<const std::string_view> col_names);
  static std::vector<const PopColumnSpec*> resolve_columns(const std::vector<std::string>& col_names);
  static std::vector<const PopColumnSpec*> resolve_columns(std::initializer_list<std::string_view> col_names);
  void serialize_selected_columns(std::vector<string> selections, string base_fname,
                                  vector<string> path_steps = {});

    vector<Agegrp> age_distribution(int popn, const vector<int>& counts) {
      assert(counts.size() == 5);

      // +1 for 1-based indexing: index 0 stays Age::Unknown (unused)
      vector<Agegrp> agegrp(popn + 1, Age::Unknown);

      int start_idx{1};
      size_t age_idx = 0;
      for (auto this_age : {Age::Age0_19, Age::Age20_39, Age::Age40_59, Age::Age60_79, Age::Age80_up}) {
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

    
    // how to return if person p is not sick?  does it matter: we might want to know the last variant experienced if any
    // return the most recent (or last) variant of virus infection
    // note we don't allow a set_variant because it should only happen if all the invariants for getting sick are met,
    // which happens in the make_sick method.
    // get_variant as PopData method: can access columns without pop.
    // used by printing
    uint8_t get_variant(size_t p) const {
      if (variant_count[p] == 0) return 0; // maps to "none"
      else if (variant_count[p] >= 16) return variant[p].back();
      else return variant[p][zidx(variant_count[p])];
    }

    // used by printing
    size_t get_recovday(size_t p) const {    // don't need a setter because it happens in make_well()
      if (recovday_count[p] == 0) return 0; // maps to "none"
      else if (recovday_count[p] >= 16) return recovday[p].back();
      else return recovday[p][zidx(recovday_count[p])];    
    }

};

#endif
