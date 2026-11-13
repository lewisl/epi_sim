
#include "parameters.h"
#include "population.h"
#include "cases.h"
#include "helpers.h"
#include "random.h"

using std::string;

////////////////////////////////
/*
Seed Cases
*/
////////////////////////////////

// Allowed trait names for Filter and Change terms.
const std::unordered_set<string> FILTER_TRAITS = {
    "status", "agegrp", "cond", "variant", "vaxstatus", "quar", "tested", "ring"};
// Seed cases may only make people infectious (see SeedCase constructor).
const std::unordered_set<string> CHANGE_TRAITS = {
    "status", "cond", "duration", "variant"};

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
  if (trait == "ring") {
    const string rname = jval.get<string>();
    auto opt = trait_from_string<Ring>(rname);
    if (!opt)
      throw std::runtime_error("Unknown ring: " + rname);
    uint8_t ridx = static_cast<uint8_t>(*opt);
    if (ridx == 0)
      throw std::runtime_error("Ring index 0 (unused sentinel) is not valid in seed terms");
    return int32_t(ridx);
  }
  // Numeric traits: duration, quar
  const int32_t value = jval.get<int32_t>();
  if (trait == "duration" && (value < 0 || value > DURATIONLIM)) {
    throw std::runtime_error(fmt::format(
        "Duration value {} is outside the supported range 0..{}", value, DURATIONLIM));
  }
  return value;
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
    else if (f.trait == "ring")      pval = int32_t(uint8_t(person.ring()));
    if (pval != f.val) return false;
  }
  return true;
}

// Apply change terms to a person via AgentView, routing status changes through
// make_sick / make_well / make_dead to preserve all invariants.
// Guard: make_sick is only applied to unexposed or recovered persons and
// requires an explicit variant term in the change.
void apply_change(AgentView person, const Change& chg, Histories& histories) {
  // Find a status term if present
  auto status_it = std::find_if(chg.terms.begin(), chg.terms.end(),
                                [](const Term& t) { return t.trait == "status"; });

  if (status_it != chg.terms.end()) {
    Status new_status{status_it->val};

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
        if      (t.trait == "variant")  var  = Variant{t.val};
        else if (t.trait == "cond")     cond = Condition{t.val};
        else if (t.trait == "duration") dur  = t.val;
      }
      if (!var) {
        throw std::runtime_error(
            "SeedCase change with status 'infectious' requires a 'variant' term");
      }
      person.make_sick(*var, histories, cond, dur);
      // Fall through to apply any remaining terms not consumed by make_sick
      for (const auto& t : chg.terms) {
        if (t.trait == "status" || t.trait == "variant" ||
            t.trait == "cond"   || t.trait == "duration") continue;
        if      (t.trait == "vaxstatus") person.vaxstatus() = Vaxstatus{t.val};
        else if (t.trait == "quar")      person.quar()      = static_cast<uint8_t>(t.val);
      }
      return;
    }

    if (new_status == RECOVERED) { person.make_well(histories);  return; }
    if (new_status == DEAD)      { person.make_dead(histories);   return; }
  }

  // No status routing: apply all terms directly
  for (const auto& t : chg.terms) {
    if      (t.trait == "cond")      person.cond()      = Condition{t.val};
    else if (t.trait == "duration")  person.duration()  = t.val;
    else if (t.trait == "vaxstatus") person.vaxstatus() = Vaxstatus{t.val};
    else if (t.trait == "quar")      person.quar()      = static_cast<uint8_t>(t.val);
  }
}


// SeedCase constructor: the single enforcement point for seed invariants.
SeedCase::SeedCase(int triggerday, bool startofday, Filter filt, Change chg)
    : triggerday(triggerday), startofday(startofday),
      filter(std::move(filt)), change(std::move(chg)) {
  auto fail = [triggerday](std::string msg) {
    throw std::runtime_error(fmt::format("Seed case on triggerday {}: {}", triggerday, msg));
  };
  // name lookup that can't throw on an out-of-range value
  auto name_of = [](const std::vector<std::string>& names, int32_t v) {
    return (v >= 0 && static_cast<size_t>(v) < names.size()) ? names[v] : fmt::format("{}", v);
  };

  if (triggerday < 1) fail(fmt::format("triggerday must be >= 1 (got {})", triggerday));
  if (change.count < 1) fail(fmt::format("change count must be >= 1 (got {})", change.count));

  bool has_status = false, has_variant = false;
  std::unordered_set<string> seen;
  for (const auto& t : change.terms) {
    if (!CHANGE_TRAITS.count(t.trait))
      fail(fmt::format("change trait '{}' is not allowed; seeds may only set status, variant, cond, duration", t.trait));
    if (!seen.insert(t.trait).second)
      fail(fmt::format("change trait '{}' appears more than once", t.trait));

    if (t.trait == "status") {
      if (t.val != int32_t(uint8_t(INFECTIOUS)))
        fail(fmt::format("status change must be 'infectious' (got '{}')", name_of(Status::names, t.val)));
      has_status = true;
    } else if (t.trait == "variant") {
      if (t.val < 1 || static_cast<size_t>(t.val) >= Variant::names.size())
        fail(fmt::format("variant must be a loaded variant other than 'none' (got '{}')",
                         name_of(Variant::names, t.val)));
      has_variant = true;
    } else if (t.trait == "cond") {
      if (t.val < int32_t(uint8_t(NIL)) || t.val > int32_t(uint8_t(SEVERE)))
        fail(fmt::format("cond must be nil, mild, sick, or severe (got '{}')", name_of(Condition::names, t.val)));
    } else if (t.trait == "duration") {
      if (t.val < 1 || t.val > DURATIONLIM)
        fail(fmt::format("duration must be in 1..{} (got {})", DURATIONLIM, t.val));
    }
  }
  if (!has_status || !has_variant)
    fail("change terms must include status 'infectious' and a variant");
}

// SeedCase::operator(): make up to change.count eligible matching people infectious.
// Only UNEXPOSED or RECOVERED people count, so mid-run seeds still reach count when
// earlier matches are already infectious or dead.
vector<size_t> SeedCase::operator()(PopData& pop, Histories& histories) {
  vector<size_t> seeded;
  for (size_t i = 1; i <= pop.popn && static_cast<int>(seeded.size()) < change.count; ++i) {
    auto person = pop.agent(i);
    if (person.status() != UNEXPOSED && person.status() != RECOVERED) continue;
    if (!matches_filter(person, filter)) continue;
    apply_change(person, change, histories);
    seeded.push_back(i);
  }
  if (static_cast<int>(seeded.size()) < change.count)
    fmt::println("WARNING: seedcase on triggerday {} seeded {} of {} requested; "
                 "not enough unexposed or recovered agents matched the filter",
                 triggerday, seeded.size(), change.count);
  return seeded;
}


SeedCase load_seed_case(const json& sc, const ModelParams& mp) {
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

  return SeedCase(triggerday, startofday, std::move(filt), std::move(chg));  // validates invariants
}

vector<SeedCase> load_seed_cases(const json& jdata,  const ModelParams& mp) {
  vector<SeedCase> seedcases;
  seedcases.reserve(jdata.size());
  for (const auto& sc : jdata) {
    seedcases.push_back(load_seed_case(sc, mp));
  }
  return seedcases;
}



////////////////////////
/*
Social Distancing Cases
*/
////////////////////////


SocialDistancing load_sd_case(const json& sdc) {
  vector<Agegrp> inc_ages;
  for (const auto& str_age : sdc["include_ages"]) {
    inc_ages.push_back(Agegrp(Agegrp::resolve_name(str_age)));
  }
  SocialDistancing sd{sdc["name"], sdc["startday"], sdc["comply"],
                      sdc["contact_delta"], sdc["touch_delta"], std::move(inc_ages)};
  sd.endday = sdc.value("endday", INT_MAX);
  return sd;
}

vector<SocialDistancing> load_sd_cases(const json& jdata, const SocialParams& social) {
  vector<SocialDistancing> sd_cases;
  sd_cases.reserve(jdata.size());
  for (const auto& sdc : jdata) {
    sd_cases.push_back(load_sd_case(sdc));
  }

  for (auto& sdc : sd_cases) {
    if (sdc.contact_delta.size() != 2)
      throw std::runtime_error(fmt::format(
          "SocialDistancing '{}' contact_delta must have 2 elements, got {}",
          sdc.name, sdc.contact_delta.size()));
    if (sdc.touch_delta.size() != 2)
      throw std::runtime_error(fmt::format(
          "SocialDistancing '{}' touch_delta must have 2 elements, got {}",
          sdc.name, sdc.touch_delta.size()));
    if (sdc.comply < 0.0f || sdc.comply > 1.0f)
      throw std::runtime_error(fmt::format(
          "SocialDistancing '{}' comply must be in [0, 1], got {}", sdc.name, sdc.comply));
    // apply_sd_cases_for_day tags on startday and clears on endday (else-if), so the
    // case needs startday >= 1 and endday after startday
    if (sdc.startday < 1 || sdc.endday <= sdc.startday)
      throw std::runtime_error(fmt::format(
          "SocialDistancing '{}' needs 1 <= startday < endday, got startday {} endday {}",
          sdc.name, sdc.startday, sdc.endday));

    sdc.contactfactors = social.contactfactors;
    shifter(sdc.contactfactors, sdc.contact_delta[0], sdc.contact_delta[1]);

    sdc.touchfactors = social.touchfactors;
    shifter(sdc.touchfactors, sdc.touch_delta[0], sdc.touch_delta[1]);
  }

  SDCase::names = {"none"};
  for (const auto& sdc : sd_cases) {
    register_trait_name(SDCase::names, sdc.name);
  }

  return sd_cases;
}

void print_sd_cases(const vector<SocialDistancing>& sd_cases) {
  std::vector<std::string_view> age_names;
  for (auto sdc : sd_cases) {
    fmt::println("\nname = \"{}\"", sdc.name);
    fmt::println("startday = {}", sdc.startday);
    fmt::println("endday = {}", sdc.endday);
    fmt::println("comply = {}", sdc.comply);
    fmt::println("contact_delta = [{}]", fmt::join(sdc.contact_delta, ","));
    fmt::println("touch_delta = [{}]", fmt::join(sdc.touch_delta, ","));

    age_names.reserve(sdc.include_ages.size());
    for (const auto& a : sdc.include_ages) age_names.push_back(a.show());
    fmt::println("include ages = [{}]", fmt::join(age_names , ","));
    age_names.clear();

    fmt::println("contactfactors ({}x{}):", sdc.contactfactors.size(), sdc.contactfactors[0].size());
    for (const auto& row : sdc.contactfactors)
      fmt::println("  [{}]", fmt::join(row, ", "));

    fmt::println("touchfactors ({}x{}):", sdc.touchfactors.size(), sdc.touchfactors[0].size());
    for (const auto& row : sdc.touchfactors)
      fmt::println("  [{}]", fmt::join(row, ", "));
  }
}


void apply_sd_cases_for_day(int day, const vector<SocialDistancing>& sd_cases, PopData& pop) {
  for (size_t k = 0; k < sd_cases.size(); ++k) {
    const auto& sdc = sd_cases[k];
    const SDCase case_value{k + 1};
    const uint8_t case_idx = static_cast<uint8_t>(case_value);

    if (day == sdc.startday) {
      for (size_t i = 1; i <= pop.popn; ++i) {
        const bool eligible_state =
            pop.status[i] == UNEXPOSED || pop.status[i] == RECOVERED ||
            pop.cond[i]   == NIL       || pop.cond[i]   == MILD;
        if (!eligible_state) continue;

        if (!sdc.include_ages.empty()) {
          bool in_ages = false;
          for (const auto& ag : sdc.include_ages) {
            if (pop.agegrp[i] == ag) { in_ages = true; break; }
          }
          if (!in_ages) continue;
        }

        if (xo::bernoulli(sdc.comply)) {
          pop.sdcase[i] = case_value;
        }
      }
    } else if (day == sdc.endday) {
      for (size_t i = 1; i <= pop.popn; ++i) {
        if (pop.sdcase[i].v == case_idx) {
          pop.sdcase[i] = SDCase{};
        }
      }
    }
  }
}
