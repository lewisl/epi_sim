// population: organize data about each person in the locale

#ifndef POPULATION
#define POPULATION

#include "lib_includes.h"


#include "helpers.h"
#include "parameters.h"

using std::array;
using std::string;
using std::vector;

// control constants
const vector<double> AGE_DIST = {0.251, 0.271, 0.255, 0.184, 0.039};
const uint8_t DURATIONLIM { 25 };   // maximum length of illness in days for anyone
const std::pair<int, int> DURATIONS  {1, DURATIONLIM};


class PopData {
  public:
    std::size_t popn; // actual population size
    std::size_t popz; // popn+1: array sizing for 1 indexing all vectors

  // vectors of pseudo enums as uint8_t or
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
  vector<Vaxstatus> vaxstatus;    //  = fill(:none, pop), :none, :first, :full, :booster,  maybe others
  vector<array<uint8_t, 16>> vaxrcvd;  // :none,  vaccine symbols  :Pfizer, :Moderna, :JnJ _
  vector<std::uint8_t> vax_count;
  vector<array<int16_t, 16>> vaxday;  // = vec of vec of sim day

  // domains of valid values for columns
      // what is the stub to use for int valued columns that print as ints?
  RuntimeEnum vax_lbl;
  RuntimeEnum Justint;
  RuntimeEnum true_false;

  // helper function for creating age group distribution defined below
      // vector<int> apportion(int n, vector<float> splits);

  // constructor
      // clang-format off
  PopData(size_t n, RuntimeEnum vax_lbl, 
          RuntimeEnum true_false, RuntimeEnum Justint,
          const vector<double>& age_dist=AGE_DIST)  
      : popn(n), popz(n+1),status(n+1, Stat::Unexposed),
        agegrp(age_distribution(n, age_dist)), // does not create a vector: splits pop into agegrps
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
  
    enum class Column : uint8_t {
      status,
      agegrp,
      cond,
      duration,
      variant,
      variant_count,
      sickday,
      sickday_count,
      recovday,
      recovday_count,
      deadday,
      ring,
      sdcase,
      tested,
      tested_count,
      quar,
      quarday,
      vaxstatus,
      vaxrcvd,
      vax_count,
      vaxday,
    };

    // methods for processing selections of rows and columns
    template <typename Action>
    void apply_to_columns(const vector<Column> &cols, Action &&action) {
      for (auto col : cols) {
        switch (col) {
        case Column::status:          action(status);                    break;
        case Column::agegrp:          action(agegrp);                    break;
        case Column::cond:            action(cond);                      break;
        case Column::duration:        action(duration, Justint);         break;
        case Column::variant:         action(variant);                   break;
        case Column::variant_count:   action(variant_count, Justint);    break;
        case Column::sickday:         action(sickday, Justint);          break;
        case Column::sickday_count:   action(sickday_count, Justint);    break;
        case Column::recovday:        action(recovday, Justint);         break;
        case Column::recovday_count:  action(recovday_count, Justint);   break;
        case Column::deadday:         action(deadday, Justint);          break;
        case Column::ring:            action(ring, Justint);             break;
        case Column::sdcase:          action(sdcase, Justint);           break;
        case Column::tested:          action(tested, true_false);        break;
        case Column::tested_count:    action(tested_count, Justint);     break;
        case Column::quar:            action(quar, true_false);          break;
        case Column::quarday:         action(quarday, Justint);          break;
        case Column::vaxstatus:       action(vaxstatus);                 break;
        case Column::vaxrcvd:         action(vaxrcvd, vax_lbl);          break;
        case Column::vax_count:       action(vax_count, Justint);        break;
        case Column::vaxday:          action(vaxday, Justint);           break;
        }  // switch
      }  // for
    } // function


    // 2 action functions for printing the PopData by selected rows and columns
     // functor to print a cell from a vector<uint8_t> or a
    // vector<array<uint8_t,16>\

    struct CellPrinter {
      int current_row;

      void operator()(const vector<uint8_t> &vec, RuntimeEnum label) const {
        fmt::print("   {}\t|", label.to_str(vec[current_row]));
      }

      void operator()(const vector<int16_t> &vec, RuntimeEnum label) const {
        fmt::print("   {}\t|", label.to_str(vec[current_row]));
      }

      void operator()(const std::vector<array<uint8_t, 16>> &vec, RuntimeEnum label) {
        fmt::print("   {}...    | ", label.to_str(vec[current_row][0]));
      }

      void operator()(const std::vector<array<int16_t, 16>> &vec, RuntimeEnum label) {
        fmt::print("   {}...    | ", label.to_str(vec[current_row][0]));
      }

      // TraitType overload: uses built-in .name() — no RuntimeEnum label needed
      template<typename T>
        requires requires(const T& t) { { t.name() } -> std::convertible_to<std::string>; }
      void operator()(const vector<T>& vec) const {
        fmt::print("   {}\t|", vec[current_row].name());
      }

      // TraitType array overload: for vector<array<T, 16>> where T has .name()
      template<typename T>
        requires requires(const T& t) { { t.name() } -> std::convertible_to<std::string>; }
      void operator()(const std::vector<array<T, 16>> &vec) const {
        fmt::print("   {}...    | ", vec[current_row][0].name());
      }
    };

    void print_table(const vector<size_t> &rows, const vector<Column> &cols) {
      for (int r : rows) {
        fmt::print("{}:\t", r);
        CellPrinter printer{r};
        apply_to_columns(cols, printer);
        fmt::print("\n");
      }
    }

    vector<Agegrp> age_distribution(int popn, const auto &age_parts) {
      assert(age_parts.size() == 5);

      // Convert proportions to counts using apportion
      vector<float> proportions(age_parts.begin(), age_parts.end());
      vector<int> counts = apportion(popn, proportions);  // distribute popn people

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

      // // Calculate total after adjustment
      // int total_after = std::accumulate(parts.begin(), parts.end(), 0);
      // std::cout << "Total after adjustment: " << total_after << "\n";
      // std::cout << "Expected (n): " << n << "\n";
      // std::cout << "Match: " << (total_after == n ? "YES" : "NO") << "\n";
      // std::cout << "=== END APPORTION DEBUG ===\n\n";

      assert(parts.back() > 0);  // should always be true for large n and reasonable splits

      return parts;
    }

    // some complex getters and setters that modify multiple vectors at the same index
    // make_sick definition is in disease_modeling.cpp
    void make_sick(size_t p, Variant var, Condition condition = Cond::Nil,
                   uint8_t durationdays = 0);
    
    // how to return if person p is not sick?  does it matter: we might want to know the last variant experienced if any
    // return the most recent (or last) variant of virus infection
    // note we don't allow a set_variant because it should only happen if all the invariants for getting sick are met,
    // which happens in the make_sick method.
    uint8_t get_variant(size_t p) const {
      if (variant_count[p] == 0) return 0; // maps to "none"
      else if (variant_count[p] >= 16) return variant[p].back();
      else return variant[p][zidx(variant_count[p])];
    }
    uint8_t get_variant(size_t p, size_t v_count) const {  // not sure this is needed--return a specific variant
      assert(v_count > 0 && v_count <= variant_count[p]);
      return variant[p][zidx(v_count)];
    }

    size_t get_recovday(size_t p) const {    // don't need a setter because it happens in make_well()
      if (recovday_count[p] == 0) return 0; // maps to "none"
      else if (recovday_count[p] >= 16) return recovday[p].back();
      else return recovday[p][zidx(recovday_count[p])];    
    }

    void incr_duration(size_t p) {
      if (duration[p] < DURATIONLIM) ++duration[p];
    }

    void make_well(size_t p);
};


#endif