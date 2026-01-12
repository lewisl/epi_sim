// population: organize data about each person in the locale

#include <cstdint>
#include <vector>
#include <array>
#include <cstdint>

using std::vector;
using std::array;


class PopData {
  std::uint32_t popn;

  // vectors of pseudo enums as uint8_t or
  // vectors of vector for repeating events happening to a person
  vector<uint8_t> status; // default unexposed = 1
  vector<uint8_t> agegrp;
  vector<uint8_t> cond;
  vector<uint8_t> duration;
  vector<array<uint8_t, 16>> variant; // = [Symbol[] for _ in 1:pop],
  vector<std::uint8_t> variant_events;
  vector<array<uint8_t, 16>> sickday;
  vector<std::uint8_t> sickday_events;
  vector<array<uint8_t, 16>> recovday;
  vector<std::uint8_t> recovday_events;
  vector<uint8_t> deadday;
  vector<uint8_t> ring;
  vector<uint8_t> sdcase;
  vector<array<uint8_t, 16>> tested; // pseudo bool 0 = false, 1 = true
  vector<std::uint8_t> tested_events;
  vector<array<uint8_t, 16>> testday;
  vector<uint8_t> quar; // pseudo bool
  vector<uint8_t> quarday;
  vector<uint8_t> vaxstatus;    //  = fill(:none, pop), :none, :first, :full, :booster,  maybe others
  vector<array<uint8_t, 16>> vaxrcvd;  // :none,  vaccine symbols  :Pfizer, :Moderna, :JnJ _
  vector<std::uint8_t> vax_events;
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
        variant_events(n),
        sickday(n),
        sickday_events(n),
        recovday(n),
        recovday_events(n),
        deadday(n, 0),
        ring(n, 0),
        sdcase(n, 0),
        tested(n),
        tested_events(n),
        testday(n),
        quar(n, 0),
        quarday(n, 0),
        vaxstatus(n, 0),
        vaxrcvd(n),
        vax_events(n),
        vaxday(n)
        // clang-format on


      {
          if (n <= 0) {
              throw std::invalid_argument(
                  "Bad size input. Must be a positive integer.");
          }
      }

  public:
    enum class Column : uint8_t {
      status,
      agegrp,
      cond,
      duration,
      variant,
      variant_events,
      sickday,
      sickday_events,
      recovday,
      recovday_events,
      deadday,
      ring,
      sdcase,
      tested,
      tested_events,
      quar,
      quarday,
      vaxstatus,
      vaxrcvd,
      vax_events,
      vaxday,
    };

  // template <typename Action>
  // void apply_to_columns(const std::vector<Column>& selected, Action&& act) {
  //   for (auto col : selected) {
  //       switch (col) {
  //           case Column::status:    act(status_slab); break;
  //           case Column::Rings:     act(rings_slab);  break;
  //           // Add new columns here, and they "exist" everywhere else instantly
  //       }
  //   }
  // }

};
