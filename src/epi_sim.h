/* TODO items:
use comments in json files!
remove debug counter in hotloop or comment out or use debug switch
serialization of series
serialization of popdata
fix bug condition in progressiontree loading
clean up test:  update for latest API changes.

Decide what to do about Touch factors where we ignore people who are sick because they can't get infected again--this
    only matters if we model viral load and if contact affects progression to more severe condition
*/

/* commit items
add dummy 0th entry to ProgressionSet so that "base" is item 1
simplify agetree by eliminating spurious struct container
*/

// all of the uses of lazytables or typedtables in Julia will be structs of vectors in c++

// data structures
// DONE:convert the yml files to json before loading with c++
// DONE: geodata
// DONE: SocialParams
// DONE: simulation data per person: popdat
// DONE: history series
// DONE: infectparams
// DONE: SocialParams
// DONE: progression params
//  vax
//  test_and_trace

// CLOSE: setup code

// simulation code
//  simulation runner
//    browser support and commands
//       cpp-httplib examples:  https://github.com/yhirose/cpp-httplib and
//       DONE: nlohmann/json (most common C++ JSON library)
//       DONE: plotly
//  DONE: progression
//  DONE: spread
//  WORKING: collect_history
//    DONE:update_total_agegrps
//    DONE: update totinfected_series
//    update totvaccinated series

// DONE: plotting via local browser
// serialize
