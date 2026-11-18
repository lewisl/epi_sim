// vaccination: (1) shots delivered follow supply, schedule, eligibility and timing;
// (2) vaccine effects follow their parameters in infection and progression.
#include "test_support.h"

#include "../src/disease_modeling.h"
#include "../src/progression.h"
#include "../src/random.h"
#include "../src/vaccination.h"

namespace {

constexpr std::string_view GROUP = "vaccination";
using test_support::install_names;
using test_support::make_pop;
using test_support::RuntimeNamesGuard;

void set_day(int d) {
  sim::reset_day();
  for (int i = 0; i < d; ++i) sim::incr_day();
  sim::ds.day = d;
}

// sum of a vax history over rings and ages for one day
HistoryValue vax_hist(const Histories& h, Phase ph, uint8_t v, size_t day) {
  HistoryValue s = 0;
  for (uint8_t r = 1; r <= Ring::count; ++r)
    for (uint8_t a = 1; a <= Agegrp::count; ++a) s += h.at(TraitID::vax, ph, v, Agegrp{a}, r)[day];
  return s;
}

VaxSet make_vaxset(int reqdshots, int delay2ndshot, int delaybooster, int brands = 1) {
  VaxSet vs;
  vs.params.push_back(Vaxparam{});
  for (int b = 0; b < brands; ++b) {
    Vaxparam p;
    p.reqdshots = reqdshots;
    p.delay2ndshot = delay2ndshot;
    p.delaybooster = delaybooster;
    vs.params.push_back(p);
  }
  return vs;
}

PerVaxSpec make_spec(uint8_t vax, int doses, float mix = 1.0f, float pct2nd = 1.0f, float pctboost = 1.0f) {
  PerVaxSpec s;
  s.vax = Vax{vax};
  s.mix = mix;
  s.starting_doses = s.doses = doses;
  s.pct2ndshot = pct2nd;
  s.pctboost = pctboost;
  return s;
}

// Flat unit-area pattern distributes the starting stock across the window.
VaxSchedSet make_schedset(vector<PerVaxSpec> specs, std::pair<int, int> dayrange) {
  VaxSched sched;
  sched.vaxesincluded = std::move(specs);
  sched.dayrange = dayrange;
  sched.targetpct = 1.0f;
  sched.filtervec = {AGE20_39};
  sched.shotmode = "all";
  sched.pattern = {1.0f, 1.0f};
  VaxSchedSet ss;
  ss.schedules.push_back({"unit_test_sched", sched});
  return ss;
}

void all_ages(PopData& pop, Agegrp a) {
  for (size_t p = 1; p <= pop.popn; ++p) pop.agegrp[p] = a;
}

int vaccinated(const PopData& pop) {
  int n = 0;
  for (size_t p = 1; p <= pop.popn; ++p) n += pop.vaxstatus[p] != Vaxstat::none;
  return n;
}

//
// delivery
//
void test_first_shot_respects_daily_supply_and_updates_histories() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  auto pop = make_pop(5);
  all_ages(pop, AGE20_39);
  Histories h(20, pop);
  VaxSet vs = make_vaxset(1, 0, 999);
  VaxSchedSet ss = make_schedset({make_spec(1, 1)}, {10, 10});
  xo::seed(1);
  vaccinate(10, ss, vs, pop, h);
  CHECK_EQ(vaccinated(pop), 1);
  CHECK_EQ(ss.schedules[0].second.vaxesincluded[0].doses, 0);
  CHECK_EQ(vax_hist(h, Phase::new_, 1, 10), 1);
  CHECK_EQ(vax_hist(h, Phase::now, 1, 10), 1);
}

void test_eligibility_filter_status_and_recent_recovery() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  auto pop = make_pop(10);
  all_ages(pop, AGE40_59);
  for (size_t p = 1; p <= 6; ++p) pop.agegrp[p] = AGE20_39;  // only 1..6 pass the filter
  pop.status[2] = RECOVERED; pop.recovday[2] = 5;    // recovered > 14 days ago: eligible
  pop.status[3] = RECOVERED; pop.recovday[3] = 10;   // recovered 10 days ago: not yet
  pop.status[4] = INFECTIOUS; pop.cond[4] = MILD;    // infectious: not eligible
  pop.status[5] = DEAD;                              // dead: not eligible
  Histories h(30, pop);
  VaxSet vs = make_vaxset(1, 0, 999);
  VaxSchedSet ss = make_schedset({make_spec(1, 100)}, {20, 20});
  xo::seed(2);
  vaccinate(20, ss, vs, pop, h);
  const std::array<bool, 11> want{false, true, true, false, false, false, true, false, false, false, false};
  for (size_t p = 1; p <= 10; ++p)
    CHECK_MSG((pop.vaxstatus[p] != Vaxstat::none) == want[p], "person {} ({}, {}): vaxstatus {}", p,
              pop.status[p].show(), pop.agegrp[p].show(), pop.vaxstatus[p].show());
  CHECK_EQ(vax_hist(h, Phase::new_, 1, 20), 3);
}

void test_second_shot_waits_for_delay() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  auto pop = make_pop(5);
  all_ages(pop, AGE40_59);
  pop.agegrp[1] = AGE20_39;
  Histories h(20, pop);
  VaxSet vs = make_vaxset(2, 7, 999);
  VaxSchedSet ss = make_schedset({make_spec(1, 8)}, {1, 8});
  vaccinate(1, ss, vs, pop, h);
  REQUIRE(pop.vaxstatus[1] == Vaxstat::first);
  vaccinate(7, ss, vs, pop, h);  // 6 days later: too early
  CHECK_EQ(pop.vaxstatus[1], Vaxstat::first);
  vaccinate(8, ss, vs, pop, h);  // 7 days later
  CHECK_EQ(pop.vaxstatus[1], Vaxstat::full);
  CHECK_EQ(pop.vaxday[1], Vaxday{8});
  CHECK_EQ(pop.vax_hist[1].count, uint16_t{2});
  CHECK_EQ(vax_hist(h, Phase::new_, 1, 8), 0);  // histories count people, not shots
}

void test_no_new_allotment_after_schedule_end() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  auto pop = make_pop(5);
  all_ages(pop, AGE20_39);
  Histories h(40, pop);
  VaxSet vs = make_vaxset(2, 10, 999);
  VaxSchedSet ss = make_schedset({make_spec(1, 10)}, {5, 5});
  vaccinate(5, ss, vs, pop, h);
  REQUIRE(vaccinated(pop) == 5);
  vaccinate(15, ss, vs, pop, h);  // follow-up is due, but the schedule has ended
  for (size_t p = 1; p <= pop.popn; ++p)
    CHECK_EQ(pop.vaxstatus[p], Vaxstat::first);
  CHECK_EQ(ss.schedules[0].second.vaxesincluded[0].doses, 5);
}

void test_booster_after_delay() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  auto pop = make_pop(5);
  all_ages(pop, AGE40_59);
  pop.agegrp[1] = AGE20_39;
  pop.vaxstatus[1] = Vaxstat::full;
  pop.vax[1] = Vax{1};
  pop.vaxday[1] = 1;
  pop.vax_hist[1].set(Vax{1});
  pop.vaxday_hist[1].set(1);
  Histories h(30, pop);
  VaxSet vs = make_vaxset(2, 7, 10);
  VaxSchedSet ss = make_schedset({make_spec(1, 16)}, {5, 20});
  vaccinate(10, ss, vs, pop, h);
  CHECK_EQ(pop.vaxstatus[1], Vaxstat::full);  // 9 days: too early
  vaccinate(20, ss, vs, pop, h);
  CHECK_EQ(pop.vaxstatus[1], Vaxstat::booster);
  CHECK_EQ(pop.vaxday_hist[1].latest(), int16_t{20});
}

void test_mixed_brands_follow_mix_and_track_per_brand() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer", "moderna"});
  const int n = 4000;
  auto pop = make_pop(n);
  all_ages(pop, AGE20_39);
  Histories h(20, pop);
  VaxSet vs = make_vaxset(1, 0, 999, 2);
  VaxSchedSet ss = make_schedset({make_spec(1, n, 0.7f), make_spec(2, n, 0.3f)}, {10, 10});
  xo::seed(7);
  vaccinate(10, ss, vs, pop, h);
  int pf = 0, mo = 0;
  for (size_t p = 1; p <= pop.popn; ++p) {
    pf += pop.vax[p] == Vax{1};
    mo += pop.vax[p] == Vax{2};
  }
  CHECK_EQ(pf + mo, n);
  CHECK_MSG(std::abs(pf / double(n) - 0.7) < 0.03, "pfizer share {:.3f}, mix 0.7", pf / double(n));
  const auto& specs = ss.schedules[0].second.vaxesincluded;
  CHECK_EQ(specs[0].doses, n - pf);
  CHECK_EQ(specs[1].doses, n - mo);
  CHECK_EQ(vax_hist(h, Phase::new_, 1, 10), pf);
  CHECK_EQ(vax_hist(h, Phase::now, 2, 10), mo);
}

// each brand's daily allotment is a separate limit: a scarce brand stops at its allotment
// while the plentiful one keeps going
void test_per_brand_daily_allotment_respected() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer", "moderna"});
  auto pop = make_pop(200);
  all_ages(pop, AGE20_39);
  Histories h(20, pop);
  VaxSet vs = make_vaxset(1, 0, 999, 2);
  VaxSchedSet ss = make_schedset({make_spec(1, 5, 0.5f), make_spec(2, 100, 0.5f)}, {10, 10});
  xo::seed(8);
  vaccinate(10, ss, vs, pop, h);
  int pf = 0, mo = 0;
  for (size_t p = 1; p <= pop.popn; ++p) {
    pf += pop.vax[p] == Vax{1};
    mo += pop.vax[p] == Vax{2};
  }
  CHECK_MSG(pf == 5, "pfizer shots {} with a daily allotment of 5", pf);
  CHECK_MSG(mo > 50 && mo <= 100, "moderna shots {} with a daily allotment of 100", mo);
}

void test_alternate_brand_used_when_first_choice_is_out() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer", "moderna"});
  auto pop = make_pop(50);
  all_ages(pop, AGE20_39);
  Histories h(20, pop);
  VaxSet vs = make_vaxset(1, 0, 999, 2);
  auto pf = make_spec(1, 0, 1.0f);  // every draw picks pfizer, which has no doses
  pf.alternate = {Vax{2}};
  VaxSchedSet ss = make_schedset({pf, make_spec(2, 10, 0.0f)}, {10, 10});
  vaccinate(10, ss, vs, pop, h);
  int mo = 0;
  for (size_t p = 1; p <= pop.popn; ++p) mo += pop.vax[p] == Vax{2};
  CHECK_EQ(mo, 10);
  CHECK_EQ(vaccinated(pop), 10);
}

// With a unit-area pattern and enough eligible people, the full target is delivered.
void test_delivery_over_schedule_matches_supply_and_pattern() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  auto pop = make_pop(20000);
  Histories h(70, pop);
  VaxSet vs = make_vaxset(1, 0, 999);
  const json young = json::parse(vaxsched::loc38015_young_json);
  VaxSched sched;
  sched.vaxesincluded = {make_spec(1, 3000, 1.0f, 0.0f, 0.0f)};
  sched.dayrange = {1, 60};
  sched.targetpct = 0.9;
  sched.filtervec = {AGE0_19, AGE20_39, AGE40_59, AGE60_79, AGE80_UP};
  sched.pattern = young["pattern"].get<vector<float>>();
  VaxSchedSet ss;
  ss.schedules.push_back({"young_pattern", sched});
  for (int d = 1; d <= 70; ++d)
    vaccinate(d, ss, vs, pop, h);
  CHECK_EQ(vaccinated(pop), 2700);
  CHECK_EQ(ss.schedules[0].second.vaxesincluded[0].doses, 300);
}

void test_short_pattern_uses_full_supply() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  auto pop = make_pop(1000);
  all_ages(pop, AGE20_39);
  Histories h(2, pop);
  VaxSet vs = make_vaxset(1, 0, 999);
  auto ss = make_schedset({make_spec(1, 1000)}, {1, 2});
  auto& sched = ss.schedules[0].second;
  const json young = json::parse(vaxsched::loc38015_young_json);
  sched.pattern = young["pattern"].get<vector<float>>();

  vaccinate(1, ss, vs, pop, h);
  CHECK_EQ(vaccinated(pop), 512);
  vaccinate(2, ss, vs, pop, h);
  CHECK_EQ(vaccinated(pop), 1000);
  CHECK_EQ(sched.vaxesincluded[0].doses, 0);
}

// The pattern offers 3,000 shots over 30 days; the stock limit is 1,000 doses.
void test_supply_is_never_exceeded() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  auto pop = make_pop(20000);
  Histories h(40, pop);
  VaxSet vs = make_vaxset(1, 0, 999);
  VaxSched sched;
  sched.vaxesincluded = {make_spec(1, 1000, 1.0f, 0.0f, 0.0f)};
  sched.dayrange = {1, 30};
  sched.targetpct = 1.0f;
  sched.filtervec = {AGE0_19, AGE20_39, AGE40_59, AGE60_79, AGE80_UP};
  sched.pattern = {1.0f, 1.0f, 1.0f, 1.0f};  // area 3: un-normalized
  VaxSchedSet ss;
  ss.schedules.push_back({"overlong", sched});
  for (int d = 1; d <= 30; ++d) vaccinate(d, ss, vs, pop, h);
  const int delivered = vaccinated(pop);
  CHECK_EQ(delivered, 1000);
  CHECK_EQ(ss.schedules[0].second.vaxesincluded[0].doses, 0);
}

void test_vax_history_overflow_retains_latest() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  auto pop = make_pop(5);
  for (int day = 1; day <= 17; ++day) {
    pop.vax_hist[1].set(Vax{1});
    pop.vaxday_hist[1].set(static_cast<int16_t>(day));
  }
  CHECK_EQ(pop.vax_hist[1].count, uint16_t{17});
  CHECK_EQ(pop.vaxday_hist[1].stored_count(), size_t{16});
  CHECK_EQ(pop.vaxday_hist[1].arr[0], int16_t{2});
  CHECK_EQ(pop.vaxday_hist[1].latest(), int16_t{17});
}

//
// effects
//
VaxSet make_effect_vaxset() {
  Vaxparam p;
  p.halflife = 100;
  p.full_effect_days = 10;
  p.day1_effect = 0.5f;
  // [variant] and [vaxstatus][variant]; index 0 unused, exhausted row all zeros
  p.infectfactor = {0.0f, 0.9f, 0.6f};
  p.effectiveness = {{0.0f, 0.0f, 0.0f},     // none
                     {0.0f, 0.5f, 0.4f},     // first
                     {0.0f, 0.8f, 0.7f},     // full
                     {0.0f, 0.95f, 0.9f},    // booster
                     {0.0f, 0.0f, 0.0f}};    // exhausted
  VaxSet vs;
  vs.names = {"none", "pfizer"};
  vs.params = {Vaxparam{}, p};
  return vs;
}

// vaxeffect = 1 - rise(t, day1_effect, full_effect_days) * decay(t - full_effect_days, halflife)
//               * effectiveness[shot][variant] * infectfactor[variant]
void test_vaxeffect_follows_parameters() {
  RuntimeNamesGuard guard;
  install_names({"base", "delta"}, {"pfizer"});
  const VaxSet vs = make_effect_vaxset();
  auto pop = make_pop(5);
  pop.vax[1] = Vax{1};
  pop.vaxday[1] = 20;
  auto a = pop.agent(1);
  auto at = [&](Vaxstatus st, size_t day, uint8_t variant) {
    pop.vaxstatus[1] = st;
    return vaxeffect(day, a, vs, variant);
  };
  const float d0 = sigdecay(0, 100, 6.0f, 0.15f);
  CHECK_NEAR(at(Vaxstat::full, 20, 1), 1 - 0.5 * d0 * 0.8 * 0.9, 1e-5);   // day of shot: day1_effect
  CHECK_NEAR(at(Vaxstat::full, 15, 1), 1 - 0.5 * d0 * 0.8 * 0.9, 1e-5);   // before the shot: clamped
  CHECK_NEAR(at(Vaxstat::full, 25, 1), 1 - 0.75 * d0 * 0.8 * 0.9, 1e-5);  // halfway through the rise
  CHECK_NEAR(at(Vaxstat::full, 30, 1), 1 - d0 * 0.8 * 0.9, 1e-5);         // full effect
  CHECK_NEAR(at(Vaxstat::full, 130, 1), 1 - 0.5 * 0.8 * 0.9, 1e-5);       // full effect + half-life
  CHECK_NEAR(at(Vaxstat::full, 130, 2), 1 - 0.5 * 0.7 * 0.6, 1e-5);       // variant selects both factors
  CHECK_NEAR(at(Vaxstat::first, 130, 1), 1 - 0.5 * 0.5 * 0.9, 1e-5);      // shot status selects row
  CHECK_NEAR(at(Vaxstat::booster, 130, 2), 1 - 0.5 * 0.9 * 0.6, 1e-5);
  CHECK_NEAR(at(Vaxstat::none, 130, 1), 1.0, 1e-6);
  pop.vaxday[1] = 0;
  CHECK_NEAR(at(Vaxstat::full, 130, 1), 1.0, 1e-6);  // no vaxday recorded
}

// vaccinated contacts at full effect (effectiveness 1) are almost never infected;
// dovax=false turns the effect off
void test_vaccine_protects_against_infection() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  VaxSet vs = make_effect_vaxset();
  vs.params[1].infectfactor = {0.0f, 1.0f};
  vs.params[1].effectiveness[Vaxstat::full] = {0.0f, 1.0f};
  vs.params[1].halflife = 100000;
  const size_t N = 4000;
  auto pop = make_pop(N + 1);
  vector<InfectParams> ip(2);
  ip[1].sendrisk.assign(DURATIONLIM + 1, 0.5f);
  ip[1].recvrisk.assign(Agegrp::count, 1.0f);
  ip[1].recovery_immunity.assign(2, 0.0f);
  pop.status[N + 1] = INFECTIOUS;
  pop.variant[N + 1] = Variant{1};
  pop.duration[N + 1] = 3;
  for (size_t p = 1; p <= N; ++p)
    if (p % 2 == 0) { pop.vaxstatus[p] = Vaxstat::full; pop.vax[p] = Vax{1}; pop.vaxday[p] = 1; }
  xo::seed(4);
  auto rate = [&](bool dovax, int parity) {
    int inf = 0;
    for (size_t p = 1; p <= N; ++p)
      if (int(p % 2) == parity) inf += isinfected(pop.agent(p), pop.agent(N + 1), ip, vs, dovax, 40);
    return inf / double(N / 2);
  };
  const double unvax = rate(true, 1), vax = rate(true, 0), vax_off = rate(false, 0);
  CHECK_MSG(std::abs(unvax - 0.5) < 0.04, "unvaccinated infection rate {:.3f}, want 0.5", unvax);
  CHECK_MSG(vax < 0.01, "vaccinated (effect 1) infection rate {:.3f}, want ~0", vax);
  CHECK_MSG(std::abs(vax_off - 0.5) < 0.04, "dovax=false: vaccinated rate {:.3f}, want 0.5", vax_off);
}

// vaccination also reduces severity through progression's risk redistribution
void test_vaccine_reduces_progression_severity() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {"pfizer"});
  VaxSet vs = make_effect_vaxset();
  vs.params[1].infectfactor = {0.0f, 1.0f};
  vs.params[1].effectiveness[Vaxstat::full] = {0.0f, 1.0f};
  vs.params[1].halflife = 100000;
  ProgressionSet ps;
  ps.progression.resize(2);
  OutcomesByCurrentCondition rows{};
  rows[NIL.v - 2] = {0, 0, 0, 0, 0, 1.0f};
  ps.progression[1].tree.entries.push_back(rows);
  for (auto& age : ps.progression[1].tree.entry_index) age[5] = 0;
  vector<InfectParams> ip(2);
  ip[1].recovery_immunity.assign(2, 0.0f);
  for (bool dovax : {true, false}) {
    const size_t N = 3000;
    auto pop = make_pop(N);
    Histories h(60, pop);
    set_day(40);
    for (size_t p = 1; p <= N; ++p) {
      pop.agent(p).make_sick(Variant{1}, h, NIL, 5);
      pop.vaxstatus[p] = Vaxstat::full;
      pop.vax[p] = Vax{1};
      pop.vaxday[p] = 1;
    }
    xo::seed(6);
    for (size_t p = 1; p <= N; ++p) progression(pop.agent(p), h, ps, ip, dovax, vs);
    int dead = 0;
    for (size_t p = 1; p <= N; ++p) dead += pop.status[p] == DEAD;
    if (dovax)
      CHECK_MSG(dead < int(N / 100), "dovax: {} of {} fully protected people died", dead, N);
    else
      CHECK_MSG(dead == int(N), "dovax=false: {} of {} died, want all (row is 100% dead)", dead, N);
  }
}

void write_vaccination_artifact(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;
  RuntimeNamesGuard guard;
  install_names({"base", "delta"}, {"pfizer"});
  const VaxSet vs = make_effect_vaxset();
  auto pop = make_pop(5);
  pop.vax[1] = Vax{1};
  pop.vaxday[1] = 1;
  std::ostringstream out;
  out << "day,first_base,full_base,booster_base,full_delta\n";
  for (size_t d = 1; d <= 400; d += 7) {
    out << d;
    const std::array<std::pair<Vaxstatus, int>, 4> combos{
        {{Vaxstat::first, 1}, {Vaxstat::full, 1}, {Vaxstat::booster, 1}, {Vaxstat::full, 2}}};
    for (const auto& [st, v] : combos) {
      pop.vaxstatus[1] = st;
      out << "," << vaxeffect(d, pop.agent(1), vs, static_cast<uint8_t>(v));
    }
    out << "\n";
  }
  test_support::write_artifact_text(options, GROUP, "vaxeffect_curves.csv", out.str());
}

}  // namespace

void run_vaccination_tests(const test_support::TestRunOptions& options) {
  using tst::run_test;
  run_test("first_shot_respects_daily_supply", test_first_shot_respects_daily_supply_and_updates_histories);
  run_test("eligibility_filter_status_recovery", test_eligibility_filter_status_and_recent_recovery);
  run_test("second_shot_waits_for_delay", test_second_shot_waits_for_delay);
  run_test("no_new_allotment_after_schedule_end", test_no_new_allotment_after_schedule_end);
  run_test("booster_after_delay", test_booster_after_delay);
  run_test("mixed_brands_follow_mix", test_mixed_brands_follow_mix_and_track_per_brand);
  run_test("per_brand_daily_allotment", test_per_brand_daily_allotment_respected);
  run_test("alternate_brand_when_out", test_alternate_brand_used_when_first_choice_is_out);
  run_test("delivery_matches_supply_and_pattern", test_delivery_over_schedule_matches_supply_and_pattern);
  run_test("short_pattern_uses_full_supply", test_short_pattern_uses_full_supply);
  run_test("supply_is_never_exceeded", test_supply_is_never_exceeded);
  run_test("vax_history_overflow", test_vax_history_overflow_retains_latest);
  run_test("vaxeffect_follows_parameters", test_vaxeffect_follows_parameters);
  run_test("vaccine_protects_against_infection", test_vaccine_protects_against_infection);
  run_test("vaccine_reduces_progression_severity", test_vaccine_reduces_progression_severity);
  run_test("artifact", [&] { write_vaccination_artifact(options); });
}
