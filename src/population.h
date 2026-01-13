// population: organize data about each person in the locale

#include <cstdint>
#include <vector>
#include <array>
#include <cstdint>
#include <iostream>

using std::array;
using std::vector;


class PopData {
  public:
  std::uint32_t popn;

  // vectors of pseudo enums as uint8_t or
  // vectors of vector for repeating count happening to a person
  vector<uint8_t> status; // default unexposed = 1
  vector<uint8_t> agegrp;
  vector<uint8_t> cond;
  vector<uint8_t> duration;
  vector<array<uint8_t, 16>> variant; // = [Symbol[] for _ in 1:pop],
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



  // constructor and destructor
      // clang-format off
  PopData(size_t n)
      : popn(n),
        status(n, 1),
        agegrp(n, 1),
        cond(n, 1),
        duration(n, 0),
        variant(n),
        variant_count(n),
        sickday(n),
        sickday_count(n),
        recovday(n),
        recovday_count(n),
        deadday(n, 0),
        ring(n, 0),
        sdcase(n, 0),
        tested(n),
        tested_count(n),
        testday(n),
        quar(n, 0),
        quarday(n, 0),
        vaxstatus(n, 0),
        vaxrcvd(n),
        vax_count(n),
        vaxday(n)
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
        case Column::status:          action(status);           break;
        case Column::agegrp:          action(agegrp);           break;
        case Column::cond:            action(cond);             break;
        case Column::duration:        action(duration);         break;
        case Column::variant:         action(variant);          break;
        case Column::variant_count:   action(variant_count);    break;
        case Column::sickday:         action(sickday);          break;
        case Column::sickday_count:   action(sickday_count);    break;
        case Column::recovday:        action(recovday);         break;
        case Column::recovday_count:  action(recovday_count);   break;
        case Column::deadday:         action(deadday);          break;
        case Column::ring:            action(ring);             break;
        case Column::sdcase:          action(sdcase);           break;
        case Column::tested:          action(tested);           break;
        case Column::tested_count:    action(tested_count);     break;
        case Column::quar:            action(quar);             break;
        case Column::quarday:         action(quarday);          break;
        case Column::vaxstatus:       action(vaxstatus);        break;
        case Column::vaxrcvd:         action(vaxrcvd);          break;
        case Column::vax_count:       action(vax_count);        break;
        case Column::vaxday:          action(vaxday);           break;
        }  // switch
      }  // for
    } // function


    // 2 action functions for printing the PopData by selected rows and columns
     // functor to print a cell from a vector<uint8_t> or a
    // vector<array<uint8_t,16>\

    struct CellPrinter {
      int current_row;

      void operator()(const vector<uint8_t> &vec) const {
        std::cout << "   " << (int)vec[current_row] << "\t|";
      }

      void operator()(const std::vector<array<uint8_t, 16>> &vec) {
        std::cout << "   " <<  (int)vec[current_row][0] << "...    | ";
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
