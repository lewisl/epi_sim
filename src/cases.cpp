
#include "lib_includes.h"
#include "parameters.h"
#include "population.h"
#include "cases.h"

using std::string;


// Allowed trait names for Filter and Change terms, validated at load time.
const std::unordered_set<string> FILTER_TRAITS = {
    "status", "agegrp", "cond", "variant", "vaxstatus", "quar", "tested"};
const std::unordered_set<string> CHANGE_TRAITS = {
    "status", "cond", "duration", "variant", "vaxstatus", "quar"};

// Parse a single Term value from JSON given its trait name.
// Throws std::runtime_error on unknown or disallowed values.
int32_t parse_term_val(const string& trait, const json& jval, const ModelParams& mp) {
  if (trait == "status") {
    auto opt = trait_from_string<Status>(jval.get<string>());
    if (!opt) throw std::runtime_error("Unknown status value: " + jval.get<string>());
    return int32_t(uint8_t(*opt));
  }
  if (trait == "agegrp") {
    auto opt = trait_from_string<Agegrp>(jval.get<string>());
    if (!opt) throw std::runtime_error("Unknown agegrp value: " + jval.get<string>());
    return int32_t(uint8_t(*opt));
  }
  if (trait == "cond") {
    auto opt = trait_from_string<Condition>(jval.get<string>());
    if (!opt) throw std::runtime_error("Unknown cond value: " + jval.get<string>());
    return int32_t(uint8_t(*opt));
  }
  if (trait == "vaxstatus") {
    auto opt = trait_from_string<Vaxstatus>(jval.get<string>());
    if (!opt) throw std::runtime_error("Unknown vaxstatus value: " + jval.get<string>());
    return int32_t(uint8_t(*opt));
  }
  if (trait == "variant") {
    const string vname = jval.get<string>();
    auto opt = trait_from_string<Variant>(vname);
    if (!opt)
      throw std::runtime_error("Unknown variant: " + vname);
    uint8_t vidx = static_cast<uint8_t>(*opt);
    if (vidx == 0)
      throw std::runtime_error("Variant 'none' (index 0) is not valid in seed terms");
    return int32_t(vidx);
  }
  // Numeric traits: duration, quar
  return jval.get<int32_t>();
}

// Returns true if all filter terms match the person (AND semantics).
bool matches_filter(AgentView person, const Filter& filt) {
  for (const auto& f : filt.terms) {
    int32_t pval{};
    if      (f.trait == "status")    pval = int32_t(uint8_t(person.status()));
    else if (f.trait == "agegrp")    pval = int32_t(uint8_t(person.agegrp()));
    else if (f.trait == "cond")      pval = int32_t(uint8_t(person.cond()));
    else if (f.trait == "variant")   pval = int32_t(uint8_t(person.variant()));
    else if (f.trait == "vaxstatus") pval = int32_t(uint8_t(person.vaxstatus()));
    else if (f.trait == "quar")      pval = int32_t(person.quar());
    else if (f.trait == "tested")    pval = int32_t(person.testday() != 0);
    if (pval != f.val) return false;
  }
  return true;
}

// Apply change terms to a person via AgentView, routing status changes through
// make_sick / make_well / make_dead to preserve all invariants.
// Guard: make_sick is only applied to unexposed or recovered persons and
// requires an explicit variant term in the change.
void apply_change(AgentView person, const Change& chg, AllSeries& series) {
  // Find a status term if present
  auto status_it = std::find_if(chg.terms.begin(), chg.terms.end(),
                                [](const Term& t) { return t.trait == "status"; });

  if (status_it != chg.terms.end()) {
    Status new_status{static_cast<uint8_t>(status_it->val)};

    if (new_status == INFECTIOUS) {
      if (person.status() != UNEXPOSED && person.status() != RECOVERED) {
        fmt::println("WARNING: skipping make_sick for person {}: status is '{}', must be unexposed or recovered",
                     person.id, person.status().show());
        return;
      }
      std::optional<Variant> var;
      Condition cond = NIL;
      Duration  dur{1};
      for (const auto& t : chg.terms) {
        if      (t.trait == "variant")  var  = Variant{static_cast<uint8_t>(t.val)};
        else if (t.trait == "cond")     cond = Condition{static_cast<uint8_t>(t.val)};
        else if (t.trait == "duration") dur  = t.val;
      }
      if (!var) {
        throw std::runtime_error(
            "SeedCase change with status 'infectious' requires a 'variant' term");
      }
      person.make_sick(*var, series, cond, dur);
      // Fall through to apply any remaining terms not consumed by make_sick
      for (const auto& t : chg.terms) {
        if (t.trait == "status" || t.trait == "variant" ||
            t.trait == "cond"   || t.trait == "duration") continue;
        if      (t.trait == "vaxstatus") person.vaxstatus() = Vaxstatus{static_cast<uint8_t>(t.val)};
        else if (t.trait == "quar")      person.quar()      = static_cast<uint8_t>(t.val);
      }
      return;
    }

    if (new_status == RECOVERED) { person.make_well(series);  return; }
    if (new_status == DEAD)      { person.make_dead(series);   return; }
  }

  // No status routing: apply all terms directly
  for (const auto& t : chg.terms) {
    if      (t.trait == "cond")      person.cond()      = Condition{static_cast<uint8_t>(t.val)};
    else if (t.trait == "duration")  person.duration()  = t.val;
    else if (t.trait == "vaxstatus") person.vaxstatus() = Vaxstatus{static_cast<uint8_t>(t.val)};
    else if (t.trait == "quar")      person.quar()      = static_cast<uint8_t>(t.val);
  }
}


// SeedCase::operator(): find candidates via filter, apply change to up to change.count of them.
vector<size_t> SeedCase::operator()(AllSeries& series) {
  vector<size_t> seeded;
  int matched = 0;
  for (size_t i = 1; i <= pop.popn && matched < change.count; ++i) {
    auto person = pop.agent(i);
    if (matches_filter(person, filter)) {
      apply_change(person, change, series);
      seeded.push_back(i);
      ++matched;
    }
  }
  if (seeded.empty())
    fmt::println("WARNING: no agents matched filter criteria for seedcase on triggerday {}", triggerday);
  return seeded;
}


vector<SeedCase> load_seed_cases(const json& jdata, PopData& pop, const ModelParams& mp) {
  vector<SeedCase> seedcases;
  for (const auto& sc : jdata) {
    int  triggerday  = sc["triggerday"];
    bool startofday  = sc.value("startofday", true);

    // --- parse filter terms ---
    Filter filt;
    for (const auto& t : sc["filter"]) {
      string trait = t["trait"].get<string>();
      if (!FILTER_TRAITS.count(trait))
        throw std::runtime_error("Disallowed filter trait: '" + trait + "'");
      filt.terms.push_back({trait, parse_term_val(trait, t["val"], mp)});
    }

    // --- parse change terms + count ---
    Change chg;
    chg.count = sc["change"]["count"].get<int>();
    for (const auto& t : sc["change"]["terms"]) {
      string trait = t["trait"].get<string>();
      if (!CHANGE_TRAITS.count(trait))
        throw std::runtime_error("Disallowed change trait: '" + trait + "'");
      chg.terms.push_back({trait, parse_term_val(trait, t["val"], mp)});
    }

    seedcases.emplace_back(triggerday, startofday, std::move(filt), std::move(chg), pop);
  }
  return seedcases;
}
