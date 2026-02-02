// population: organize data about each person in the locale

#ifndef POPULATION
#define POPULATION

#include <cstdint>
#include <vector>
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include "parameters.h"

using std::array;
using std::vector;
using std::string;


class PopData {
  public:
  std::uint32_t popn;

  // vectors of pseudo enums as uint8_t or
  // vectors of vector for repeating count happening to a person
  vector<uint8_t> status; // default unexposed = 1
  vector<uint8_t> agegrp;
  vector<uint8_t> cond;
  vector<uint8_t> duration;
  vector<array<uint8_t, 16>> variant; 
  vector<std::uint8_t> variant_count;
  vector<array<uint8_t, 16>> sickday;
  vector<std::uint8_t> sickday_count;
  vector<array<uint8_t, 16>> recovday;
  vector<std::uint8_t> recovday_count;
  vector<uint8_t> deadday;
  vector<uint8_t> ring;
  vector<uint8_t> sdcase;
  vector<array<uint8_t, 16>> tested; // pseudo bool 0 = false, 1 = true
  vector<std::uint8_t> tested_count;
  vector<array<uint8_t, 16>> testday;
  vector<uint8_t> quar; // pseudo bool
  vector<uint8_t> quarday;
  vector<uint8_t> vaxstatus;    //  = fill(:none, pop), :none, :first, :full, :booster,  maybe others
  vector<array<uint8_t, 16>> vaxrcvd;  // :none,  vaccine symbols  :Pfizer, :Moderna, :JnJ _
  vector<std::uint8_t> vax_count;
  vector<array<uint8_t, 16>> vaxday;  // = vec of vec of sim day

  // domains of valid values for columns
      // what is the stub to use for int valued columns that print as ints?
  RuntimeEnum cond_lbl;
  RuntimeEnum status_lbl;
  RuntimeEnum agegrp_lbl;
  RuntimeEnum variant_lbl;
  RuntimeEnum vax_lbl;
  RuntimeEnum vaxstatus_lbl;
  RuntimeEnum Justint;
  RuntimeEnum true_false;

  // constructor
      // clang-format off
  PopData(size_t n, RuntimeEnum status_lbl, RuntimeEnum agegrp_lbl, RuntimeEnum cond_lbl,
          RuntimeEnum variant_lbl, RuntimeEnum vax_lbl, RuntimeEnum vaxstatus_lbl,
          RuntimeEnum true_false, RuntimeEnum Justint)
      : popn(n), status(n, 1), agegrp(n, 1), cond(n, 1), duration(n, 0),
        variant(n), variant_count(n), sickday(n), sickday_count(n),
        recovday(n), recovday_count(n), deadday(n, 0), ring(n, 0),
        sdcase(n, 0), tested(n), tested_count(n), testday(n), quar(n, 0), quarday(n, 0),
        vaxstatus(n, 0), vaxrcvd(n), vax_count(n), vaxday(n),
        status_lbl (status_lbl), agegrp_lbl(agegrp_lbl), cond_lbl(cond_lbl), variant_lbl(variant_lbl),
        vax_lbl(vax_lbl), vaxstatus_lbl(vaxstatus_lbl), true_false(true_false), Justint(Justint)
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
        case Column::status:          action(status, status_lbl);        break;
        case Column::agegrp:          action(agegrp, agegrp_lbl);        break;
        case Column::cond:            action(cond, cond_lbl);            break;
        case Column::duration:        action(duration, Justint);         break;
        case Column::variant:         action(variant, variant_lbl);      break;
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
        case Column::vaxstatus:       action(vaxstatus, true_false);     break;
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
        std::cout << "   " << label.to_str(vec[current_row]) << "\t|";
      }

      void operator()(const std::vector<array<uint8_t, 16>> &vec, RuntimeEnum label) {
        std::cout << "   " <<  label.to_str(vec[current_row][0]) << "...    | ";
      }
    };

    void print_table(const vector<int> &rows, const vector<Column> &cols) {
      for (int r : rows) {
        std::cout << r << ":\t";
        CellPrinter printer{r};
        apply_to_columns(cols, printer);
        std::cout << "\n";
      }
    }

};

#endif