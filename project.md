## TODO
- r0 calculations
- fix cumbersome seedcase code?
- use comments in json files!
- test and trace
- revisit whether severe people should stay in the social distancing case.
- revisit if compliance is an upfront thing or on any day a person is spreading or contactable
- Decide what to do about Touch factors where we ignore people who are sick because they can't get infected again?--this
    only matters if we model viral load and if contact affects progression to more severe condition
- maybe add contruction time guards to trait structs as in:
    >constexpr explicit Agegrp(uint8_t v) noexcept : v(v < 6 ? v : 0) {}
    constexpr explicit Agegrp(int val) noexcept : v(val >= 0 && val < 6 ? static_cast<uint8_t>(val) : 0) {}
- guided user interface in webview: projected by c++ code, gets user inputs, responds appropriately
- distribution packaging
- port to Windows?  port to Linux?

## Done
- achieve better repeatability and comparison of simulation runs by bundling inputs and outputs for a simulation run in suitably named directories
- consolidate the series serialization code for printing and serialization
- implement rings
- clean up test:  update for latest API changes.
- social distancing cases
- add variant and vax columns to HistorySeries
- unify printing and serialization of popdata: after lots of reworking
- serialization of popdata
- serialization of series
- make key series functions class functions for HistorySeries. Result:  major speed up!
- remove debug counter in hotloop or comment out or use debug switch
- clean up series name containers ease maintainability and reduce order dependence
