// disease_modeling: state transitions, immunity math, infection risk, spread contact
// selection, and progression outcome statistics.
#include "test_support.h"
#include "test_invariants.h"

#include "../src/cases.h"
#include "../src/disease_modeling.h"
#include "../src/progression.h"
#include "../src/random.h"
#include "../src/spread.h"

namespace {

using test_support::atomic;
using test_support::install_names;
using test_support::make_pop;
using test_support::RuntimeNamesGuard;
using test_support::total;

constexpr auto S = TraitID::status;
constexpr auto VAR = TraitID::variant;
constexpr auto NOW = Phase::now;
constexpr auto NEW = Phase::new_;
// gamma_distribution needs scale > 0; this scale always rounds to 0 contacts
constexpr float NO_CONTACT = 1e-6f;

void set_day(int d) {
  sim::reset_day();
  for (int i = 0; i < d; ++i) sim::incr_day();
  sim::ds.day = d;
}

int count_status(const PopData& pop, Status s) {
  int n = 0;
  for (size_t p = 1; p <= pop.popn; ++p) n += pop.status[p] == s;
  return n;
}

// minimal world for calling spread() and progression() directly
struct World {
  PopData pop;
  Histories h;
  SocialParams social{};
  vector<InfectParams> ip = vector<InfectParams>(2);
  VaxSet vaxset{};
  vector<size_t> contacts = vector<size_t>(250);
  vector<float> indoor = vector<float>(30, 1.0f);
  vector<SocialDistancing> sd;
  RingTraits rt{};
  vector<vector<size_t>> members;
  vector<size_t> lengths;

  World(size_t n, size_t rings = 1) : pop(make_pop(n, rings)), h(30, pop) {
    ip[1].sendrisk.assign(DURATIONLIM + 1, 1.0f);
    ip[1].recvrisk.assign(Agegrp::count, 1.0f);
    ip[1].recovery_immunity.assign(2, 0.0f);
    ip[1].immunehalflife = 100;
    social.gammashape = 1.0f;
    for (auto& row : social.contactfactors) row.fill(NO_CONTACT);
    for (auto& row : social.touchfactors) row.fill(1.0f);
  }
  void spread_from(size_t p, int times = 1) {
    for (int i = 0; i < times; ++i)
      spread(pop, h, pop.agent(p), social, ip, vaxset, false, contacts, 1.0f, indoor, sd, rt, members, lengths);
  }
  int infected() const { return count_status(pop, INFECTIOUS); }
};

//
// state transitions: PopData fields and the 5-coordinate history cells (trait, phase,
// value, ring, age) plus totals
//
void test_state_transitions_update_person_and_histories() {
  RuntimeNamesGuard guard;
  install_names({"base", "delta"}, {}, {"r1", "r2"});
  auto pop = make_pop(10, 2);  // person 2: age0_19, ring 2
  Histories h(5, pop);
  auto p = pop.agent(2);
  const Agegrp age = p.agegrp();
  REQUIRE(p.ring().v == 2);

  set_day(1);
  p.make_sick(Variant{2}, h, MILD, 3);
  CHECK_EQ(p.status(), INFECTIOUS);
  CHECK_EQ(p.cond(), MILD);
  CHECK_EQ(p.duration(), Duration{3});
  CHECK_EQ(p.variant(), Variant{2});
  CHECK_EQ(p.sickday(), Sickday{1});
  CHECK_EQ(p.variant_hist().count, uint16_t{1});
  CHECK_EQ(atomic(h, S, NEW, INFECTIOUS.v, age, 2, 1), 1);
  CHECK_EQ(atomic(h, S, NOW, INFECTIOUS.v, age, 2, 1), 1);
  CHECK_EQ(atomic(h, S, NOW, UNEXPOSED.v, age, 2, 1), 0);  // the one ring-2 age0_19 person
  CHECK_EQ(atomic(h, VAR, NOW, 2, age, 2, 1), 1);
  CHECK_EQ(atomic(h, VAR, NEW, 2, age, 2, 1), 1);
  CHECK_EQ(atomic(h, S, NOW, INFECTIOUS.v, age, 1, 1), 0);  // nothing in ring 1

  set_day(3);
  h.init_history_newday(2);
  h.init_history_newday(3);
  p.make_well(h);
  CHECK_EQ(p.status(), RECOVERED);
  CHECK_EQ(p.cond(), UNINFECTED);
  CHECK_EQ(p.duration(), Duration{0});
  CHECK_EQ(p.recovday(), Recovday{3});
  CHECK_EQ(atomic(h, S, NOW, RECOVERED.v, age, 2, 3), 1);
  CHECK_EQ(atomic(h, S, NEW, RECOVERED.v, age, 2, 3), 1);
  CHECK_EQ(atomic(h, S, NOW, INFECTIOUS.v, age, 2, 3), 0);
  CHECK_EQ(atomic(h, VAR, NOW, 2, age, 2, 3), 0);

  // reinfection from recovered moves the person out of now_recovered
  set_day(4);
  h.init_history_newday(4);
  p.make_sick(Variant{1}, h);
  CHECK_EQ(atomic(h, S, NOW, RECOVERED.v, age, 2, 4), 0);
  CHECK_EQ(p.variant_hist().count, uint16_t{2});
  CHECK_EQ(p.cond(), NIL);  // default condition
  CHECK_EQ(p.duration(), Duration{1});

  set_day(5);
  h.init_history_newday(5);
  p.make_dead(h);
  CHECK_EQ(p.status(), DEAD);
  CHECK_EQ(p.deadday(), Deadday{5});
  CHECK_EQ(atomic(h, S, NOW, DEAD.v, age, 2, 5), 1);
  CHECK_EQ(atomic(h, S, NEW, DEAD.v, age, 2, 5), 1);
  CHECK_EQ(atomic(h, VAR, NOW, 1, age, 2, 5), 0);

  h.sum_total_histories();
  CHECK_EQ(total(h, S, NOW, UNEXPOSED.v, 5), 9);
  CHECK_EQ(total(h, S, NOW, DEAD.v, 5), 1);
  CHECK_EQ(total(h, S, NOW, DEAD.v, 5, 2), 1);     // ring-2 split total
  CHECK_EQ(total(h, S, NOW, DEAD.v, 5, 1), 0);     // ring-1 split total
  CHECK_EQ(total(h, S, NEW, INFECTIOUS.v, 1), 1);
  CHECK_EQ(total(h, VAR, NEW, 2, 1, RING_ALL, age.v), 1);  // age split total
  CHECK_EQ(invariants::check_run(pop, h, "transitions"), 0);
}

void test_history_overflow_keeps_latest_16() {
  RuntimeNamesGuard guard;
  install_names({"base"});
  auto pop = make_pop(5);
  Histories h(20, pop);
  auto p = pop.agent(1);
  for (int day = 1; day <= 17; ++day) {
    set_day(day);
    p.make_sick(Variant{1}, h);
    p.make_well(h);
  }
  CHECK_EQ(p.recovday_hist().count, uint16_t{17});
  CHECK_EQ(p.recovday_hist().stored_count(), size_t{16});
  CHECK_EQ(p.recovday_hist().show(), string("2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17"));
  CHECK_EQ(p.sickday_hist().latest(), int16_t{17});
  CHECK_EQ(p.variant_hist().count, uint16_t{17});
}

//
// immunity curves
//
void test_decay_and_rise_functions() {
  CHECK_NEAR(sigdecay(100, 100, 6.0f, 0.15f), 0.5, 1e-6);  // half-life means half at t = h
  CHECK_NEAR(sigdecay(0, 100, 6.0f, 0.15f), 1.0 / (1.0 + std::exp(-6.0)), 1e-6);
  CHECK_NEAR(sigdecay(5000, 100, 6.0f, 0.15f), 0.15, 1e-6);  // floor
  float prev = 2;
  bool mono = true;
  for (size_t t = 0; t <= 400; t += 10) {
    const float y = sigdecay(t, 100, 6.0f, 0.15f);
    mono &= y <= prev;
    prev = y;
  }
  CHECK_MSG(mono, "sigdecay is not non-increasing in t");
  CHECK_NEAR(effect_rise(0, 0.65f, 14), 0.65, 1e-6);
  CHECK_NEAR(effect_rise(7, 0.65f, 14), 0.825, 1e-6);
  CHECK_NEAR(effect_rise(14, 0.65f, 14), 1.0, 1e-6);
  CHECK_NEAR(effect_rise(40, 0.65f, 14), 1.0, 1e-6);
}

// recoveffect = 1 - rise(t) * decay(t) * recovery_immunity[contact variant][spreader variant]
void test_recoveffect() {
  RuntimeNamesGuard guard;
  install_names({"base", "delta"});
  auto pop = make_pop(5);
  vector<InfectParams> ip(3);
  ip[1].recovery_immunity = {0.0f, 0.8f, 0.3f};  // recovered from base: vs base, vs delta
  ip[2].recovery_immunity = {0.0f, 0.5f, 0.9f};  // recovered from delta
  ip[1].immunehalflife = 100;
  ip[2].immunehalflife = 200;
  pop.variant[1] = Variant{1};
  pop.recovday[1] = 10;
  pop.variant[2] = Variant{2};
  pop.recovday[2] = 10;
  auto a = pop.agent(1), b = pop.agent(2), naive = pop.agent(3);

  auto want = [](float imm, size_t t, float h) {
    return 1.0f - imm * effect_rise(t) * sigdecay(t, h, 6.0f, 0.15f);
  };
  CHECK_NEAR(recoveffect(naive, 50, 1, ip), 1.0, 1e-6);
  CHECK_NEAR(recoveffect(a, 10, 1, ip), want(0.8f, 0, 100), 1e-6);   // day of recovery: rise = 0.65
  CHECK_NEAR(recoveffect(a, 24, 1, ip), want(0.8f, 14, 100), 1e-6);  // full rise
  CHECK_NEAR(recoveffect(a, 110, 1, ip), 1.0 - 0.8 * 0.5, 1e-5);   // half-life
  CHECK_NEAR(recoveffect(a, 9000, 1, ip), 1.0 - 0.8 * 0.15, 1e-5); // floor
  CHECK_NEAR(recoveffect(a, 110, 2, ip), 1.0 - 0.3 * 0.5, 1e-5);   // spreader variant selects column
  CHECK_NEAR(recoveffect(b, 210, 1, ip), 1.0 - 0.5 * 0.5, 1e-5);   // contact variant selects row and half-life
  bool mono = true, bounded = true;
  float prev = 0;
  for (size_t d = 24; d < 600; d += 7) {
    const float f = recoveffect(a, d, 1, ip);
    mono &= f >= prev;
    bounded &= f >= 1.0f - 0.8f && f <= 1.0f;
    prev = f;
  }
  CHECK_MSG(mono, "recovery protection does not wane monotonically after full rise");
  CHECK_MSG(bounded, "recoveffect outside [1 - immunity, 1]");
}

// infectrisk = clamp(sendrisk[duration] * recvrisk[agegrp - 1] * min(vax, recov), 0, 1)
void test_infectrisk_indexing_and_clamp() {
  vector<InfectParams> ip(2);
  ip[1].sendrisk.resize(DURATIONLIM + 1);
  for (size_t d = 0; d <= DURATIONLIM; ++d) ip[1].sendrisk[d] = 0.01f * d;
  ip[1].recvrisk = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
  CHECK_NEAR(infectrisk(ip, 1, 7, AGE60_79.v), 0.07 * 0.4, 1e-6);
  CHECK_NEAR(infectrisk(ip, 1, DURATIONLIM, AGE80_UP.v), 0.25 * 0.5, 1e-6);
  CHECK_NEAR(infectrisk(ip, 1, 10, AGE0_19.v, 0.8f, 0.5f), 0.1 * 0.1 * 0.5, 1e-6);
  ip[1].sendrisk[3] = 5.0f;
  ip[1].recvrisk[0] = 1.0f;
  CHECK_NEAR(infectrisk(ip, 1, 3, AGE0_19.v), 1.0, 1e-6);
}

void test_touch_map() {
  CHECK_EQ(touch_map(UNEXPOSED, UNINFECTED), uint8_t{0});
  CHECK_EQ(touch_map(RECOVERED, UNINFECTED), uint8_t{1});
  CHECK_EQ(touch_map(INFECTIOUS, NIL), uint8_t{2});
  CHECK_EQ(touch_map(INFECTIOUS, SEVERE), uint8_t{5});
  CHECK_THROWS(std::runtime_error, (void)touch_map(INFECTIOUS, UNINFECTED), "Invalid condition");
  CHECK_THROWS(std::runtime_error, (void)touch_map(DEAD, SEVERE), "Invalid status");
}

//
// spread: the contact count uses contactfactors[cond - 2][agegrp - 1]. Zero exactly the
// row (or column) the spreader should use; any infection means the wrong cell was read.
//
void test_spread_uses_spreader_condition_row() {
  RuntimeNamesGuard guard;
  install_names({"base"});
  for (Condition c : {NIL, MILD, SICK, SEVERE}) {
    for (bool positive : {false, true}) {
      World w(2000);
      for (size_t r = 0; r < 4; ++r) w.social.contactfactors[r].fill((r == size_t(c.v - 2)) == positive ? 3.0f : NO_CONTACT);
      set_day(1);
      w.pop.agent(1).make_sick(Variant{1}, w.h, c, 2);
      set_day(2);
      xo::seed(11);
      w.spread_from(1, 40);
      const int newly = w.infected() - 1;
      if (positive)
        CHECK_MSG(newly > 20, "spreader cond {}: only {} infections with its own row = 3", c.show(), newly);
      else
        CHECK_MSG(newly == 0, "spreader cond {}: {} infections though its row is 0 (wrong row read)", c.show(),
                  newly);
    }
  }
}

void test_spread_uses_spreader_age_column() {
  RuntimeNamesGuard guard;
  install_names({"base"});
  for (size_t spreader : {size_t{1}, size_t{1999}}) {  // age0_19 and age80_up
    World w(2000);
    const size_t col = zidx(w.pop.agegrp[spreader]);
    for (auto& row : w.social.contactfactors)
      for (size_t a = 0; a < 5; ++a) row[a] = a == col ? NO_CONTACT : 3.0f;
    set_day(1);
    w.pop.agent(spreader).make_sick(Variant{1}, w.h);
    set_day(2);
    w.spread_from(spreader, 40);
    CHECK_MSG(w.infected() == 1, "spreader age {}: {} infections though its column is 0",
              w.pop.agegrp[spreader].show(), w.infected() - 1);
  }
}

// touch factors: contact's row by status (unexposed/recovered) and column by contact age
void test_spread_touch_factors_by_contact() {
  RuntimeNamesGuard guard;
  install_names({"base"});
  World w(2000);
  for (auto& row : w.social.contactfactors) row.fill(4.0f);
  w.social.touchfactors[0].fill(0.0f);  // unexposed contacts are never touched
  set_day(1);
  w.pop.agent(1).make_sick(Variant{1}, w.h);
  set_day(2);
  w.spread_from(1, 40);
  CHECK_MSG(w.infected() == 1, "{} unexposed contacts infected with touch row 0", w.infected() - 1);

  for (size_t p = 2; p <= w.pop.popn; ++p) w.pop.status[p] = RECOVERED;  // now only row 1 applies
  w.spread_from(1, 40);
  CHECK_MSG(w.infected() > 20, "recovered contacts (touch row 1 = 1.0) infected only {}", w.infected() - 1);
}

// SD cases replace the spreader's contactfactors and the contact's touchfactors
void test_spread_uses_social_distancing_factors() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {}, {"population"}, {"lockdown"});
  World w(2000);
  for (auto& row : w.social.contactfactors) row.fill(4.0f);
  SocialDistancing sdc;
  sdc.name = "lockdown";
  for (auto& row : sdc.contactfactors) row.fill(NO_CONTACT);
  for (auto& row : sdc.touchfactors) row.fill(1.0f);
  w.sd.push_back(sdc);
  set_day(1);
  w.pop.agent(1).make_sick(Variant{1}, w.h);
  set_day(2);
  w.pop.sdcase[1] = SDCase{1};
  w.spread_from(1, 40);
  CHECK_MSG(w.infected() == 1, "spreader in SD case (contacts 0) infected {}", w.infected() - 1);

  w.pop.sdcase[1] = SDCase{};
  w.sd[0].contactfactors = w.social.contactfactors;
  for (auto& row : w.sd[0].touchfactors) row.fill(0.0f);
  for (size_t p = 2; p <= w.pop.popn; ++p) w.pop.sdcase[p] = SDCase{1};
  w.spread_from(1, 40);
  CHECK_MSG(w.infected() == 1, "contacts in SD case (touch 0) infected {}", w.infected() - 1);
}

// ring-aware contact selection: out_ring_prob 0 keeps every contact in the spreader's ring
void test_spread_ring_selection() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {}, {"r1", "r2"});
  for (float pout : {0.0f, 1.0f}) {
    World w(2000, 2);
    for (auto& row : w.social.contactfactors) row.fill(4.0f);
    w.rt.out_ring_prob.assign(3, vector<float>(Agegrp::names.size(), pout));
    w.rt.pct_of_population = {0.0f, 0.5f, 0.5f};
    w.members = build_ring_members(w.pop, 2);
    w.lengths = {0, w.members[1].size(), w.members[2].size()};
    set_day(1);
    w.pop.agent(1).make_sick(Variant{1}, w.h);  // ring 1
    set_day(2);
    w.spread_from(1, 40);
    int in = 0, out = 0;
    for (size_t p = 2; p <= w.pop.popn; ++p)
      if (w.pop.status[p] == INFECTIOUS) (w.pop.ring[p].v == 1 ? in : out)++;
    if (pout == 0.0f)
      CHECK_MSG(out == 0 && in > 20, "out_ring_prob 0: {} in-ring, {} out-of-ring infections", in, out);
    else
      CHECK_MSG(in == 0 && out > 20, "out_ring_prob 1: {} in-ring, {} out-of-ring infections", in, out);
  }
}

//
// progression: outcome frequencies follow the tree row; outcome k (1..4) -> Condition k+1
//
void test_progression_outcome_frequencies() {
  RuntimeNamesGuard guard;
  install_names({"base"});
  const size_t N = 20000;
  auto pop = make_pop(N);
  Histories h(3, pop);
  ProgressionSet ps;
  ps.progression.resize(2);
  auto& tree = ps.progression[1].tree;
  OutcomesByCurrentCondition rows{};
  const std::array<float, 6> mild_row{0.1f, 0.2f, 0.3f, 0.2f, 0.1f, 0.1f};
  rows[MILD.v - 2] = mild_row;
  tree.entries.push_back(rows);
  for (auto& age : tree.entry_index) age[5] = 0;
  vector<InfectParams> ip(2);
  ip[1].recovery_immunity.assign(2, 0.0f);
  VaxSet vs;

  set_day(1);
  for (size_t p = 1; p <= N; ++p) pop.agent(p).make_sick(Variant{1}, h, MILD, 5);
  set_day(2);
  h.init_history_newday(2);
  xo::seed(5);
  for (size_t p = 1; p <= N; ++p) progression(pop.agent(p), h, ps, ip, false, vs);

  std::array<int, 6> got{};
  int bad_duration = 0;
  for (size_t p = 1; p <= N; ++p) {
    if (pop.status[p] == RECOVERED) ++got[0];
    else if (pop.status[p] == DEAD) ++got[5];
    else {
      ++got[pop.cond[p].v - 1];  // nil=2 -> outcome 1 ... severe=5 -> outcome 4
      bad_duration += pop.duration[p] != 6;
    }
  }
  for (size_t k = 0; k < 6; ++k)
    CHECK_MSG(std::abs(got[k] / double(N) - mild_row[k]) < 0.015,
              "outcome {} ({}): observed {:.4f}, tree row {:.2f}", k,
              magic_enum::enum_name(static_cast<Progressionmap>(k)), got[k] / double(N), mild_row[k]);
  CHECK_MSG(bad_duration == 0, "{} still-infected people did not advance duration 5 -> 6", bad_duration);
  CHECK_EQ(atomic(h, S, NEW, DEAD.v, AGE40_59, 1, 2) + atomic(h, S, NEW, DEAD.v, AGE0_19, 1, 2) +
               atomic(h, S, NEW, DEAD.v, AGE20_39, 1, 2) + atomic(h, S, NEW, DEAD.v, AGE60_79, 1, 2) +
               atomic(h, S, NEW, DEAD.v, AGE80_UP, 1, 2),
           got[5]);

  // not a breakday: only duration advances
  auto q = pop.agent(1);
  if (q.status() == INFECTIOUS) {
    const auto cond = q.cond();
    const auto dur = q.duration();
    progression(q, h, ps, ip, false, vs);
    CHECK_EQ(q.cond(), cond);
    CHECK_EQ(q.duration(), Duration{static_cast<uint8_t>(dur.v + 1)});
  }
}

// prior recovery redistributes severe/dead probability toward recover/nil/mild;
// on the terminal day the excess goes to recovery only
void test_progression_immunity_redistribution() {
  RuntimeNamesGuard guard;
  install_names({"base"});
  for (uint8_t day : {uint8_t{5}, uint8_t{DURATIONLIM}}) {
    const size_t N = 6000;
    auto pop = make_pop(N);
    Histories h(30, pop);
    ProgressionSet ps;
    ps.progression.resize(2);
    OutcomesByCurrentCondition rows{};
    rows[NIL.v - 2] = {0, 0, 0, 0, 0, 1.0f};  // everyone dies without immunity
    ps.progression[1].tree.entries.push_back(rows);
    for (auto& age : ps.progression[1].tree.entry_index) age[day] = 0;
    vector<InfectParams> ip(2);
    ip[1].recovery_immunity = {0.0f, 1.0f};
    ip[1].immunehalflife = 100000;
    VaxSet vs;
    set_day(15);
    for (size_t p = 1; p <= N; ++p) {
      pop.agent(p).make_sick(Variant{1}, h, NIL, day);
      if (p % 2 == 0) pop.recovday[p] = 1;  // even ids recovered 14 days earlier
    }
    xo::seed(9);
    for (size_t p = 1; p <= N; ++p) progression(pop.agent(p), h, ps, ip, false, vs);
    int naive_dead = 0, immune_dead = 0, immune_recov = 0;
    for (size_t p = 1; p <= N; ++p) {
      if (p % 2) naive_dead += pop.status[p] == DEAD;
      else {
        immune_dead += pop.status[p] == DEAD;
        immune_recov += pop.status[p] == RECOVERED;
      }
    }
    CHECK_MSG(naive_dead == int(N / 2), "day {}: {} of {} without immunity died (want all)", day, naive_dead, N / 2);
    CHECK_MSG(immune_dead < int(N / 100), "day {}: {} immune people died (want ~0.3%)", day, immune_dead);
    const double frac = immune_recov / double(N / 2);
    if (day == DURATIONLIM)
      CHECK_MSG(frac > 0.98, "terminal day: immune recovered fraction {:.3f}, want ~1", frac);
    else
      CHECK_MSG(std::abs(frac - 1.0 / 3) < 0.03, "day 5: immune recovered fraction {:.3f}, want ~1/3", frac);
  }
}

}  // namespace

void run_disease_modeling_tests(const test_support::TestRunOptions&) {
  using tst::run_test;
  run_test("state_transitions_update_person_and_histories", test_state_transitions_update_person_and_histories);
  run_test("history_overflow_keeps_latest_16", test_history_overflow_keeps_latest_16);
  run_test("decay_and_rise_functions", test_decay_and_rise_functions);
  run_test("recoveffect", test_recoveffect);
  run_test("infectrisk_indexing_and_clamp", test_infectrisk_indexing_and_clamp);
  run_test("touch_map", test_touch_map);
  run_test("spread_uses_spreader_condition_row", test_spread_uses_spreader_condition_row);
  run_test("spread_uses_spreader_age_column", test_spread_uses_spreader_age_column);
  run_test("spread_touch_factors_by_contact", test_spread_touch_factors_by_contact);
  run_test("spread_uses_social_distancing_factors", test_spread_uses_social_distancing_factors);
  run_test("spread_ring_selection", test_spread_ring_selection);
  run_test("progression_outcome_frequencies", test_progression_outcome_frequencies);
  run_test("progression_immunity_redistribution", test_progression_immunity_redistribution);
}
