/* TODO items:
vaccination
vaccination series vectors
use comments in json files!
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
// convert the yml files to json before loading with c++
    // options:  tojson is a tiny (127 sloc), header only library to convert xml and
      // yaml documents into nlohmann::json objects.
    // website: https://jsonformatter.org/yaml-to-json
//  geodata
//  SocialParams
//  simulation data per person: popdat
//  history series
//  geodata
//  infectparams
//  spreadparams
//  SocialParams
//  progression params
//  vax
//  test_and_trace

// setup code

// simulation code
//  simulation runner
//    browser support and commands
//       cpp-httplib examples:  https://github.com/yhirose/cpp-httplib and
//       nlohmann/json (most common C++ JSON library)
//       plotly
//  progression
//  spread
//  collect_history
//    update_total_agegrps
//    update totinfected_series
//    update totvaccinated series

// plotting
//   browser support
// serialize
//   plotly