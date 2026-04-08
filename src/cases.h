#pragma once

#include "lib_includes.h"
#include "population.h"

// A single trait column reference + its expected or new value.
// trait is the AgentView accessor name (e.g. "status", "agegrp", "cond", "variant").
// val is the integer representation of the value, parsed at load time.
struct Term {
  string trait;
  int32_t val;
};

// Filter: all terms must match (AND semantics) for a person to be a candidate.
// Allowed traits: "status", "agegrp", "cond", "variant", "vaxstatus", "quar", "tested"
// "tested" is derived from latest testday != 0.
struct Filter {
  vector<Term> terms;
};

// Change: the trait values to set on each matched candidate, and how many to affect.
// Allowed traits: "status", "cond", "duration", "variant", "vaxstatus", "quar"
// When "status"="infectious" is present, make_sick is called (preserving all invariants)
// and a "variant" term is required.
// Guard: only UNEXPOSED and RECOVERED persons may be made sick.
struct Change {
  vector<Term> terms;
  int count{0};
};

struct SeedCase {
  PopData& pop;
  int triggerday{0};    // simulation day the changes are applied
  bool startofday{true}; // true = beginning of day, false = end of day
  Filter filter;
  Change change;

  SeedCase(int triggerday, bool startofday, Filter filt, Change chg, PopData& pop)
      : triggerday(triggerday), startofday(startofday),
        filter(std::move(filt)), change(std::move(chg)), pop(pop) {}
  SeedCase() = delete;

  vector<size_t> operator()(HistorySeries& series);
};


void apply_change(AgentView person, const Change& chg, HistorySeries& series);
bool matches_filter(AgentView person, const Filter& filt);
// Load seed cases from a parsed JSON array; requires ModelParams for variant lookup.
vector<SeedCase> load_seed_cases(const json& jdata, PopData& pop, const ModelParams& mp);
