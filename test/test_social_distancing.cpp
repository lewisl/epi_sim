// social_distancing: SD case tagging on start/end days, SD effect on a whole run, and
// seed cases at runtime (counting, eligibility, beginning- and end-of-day passes).
#include "test_support.h"
#include "test_invariants.h"

#include "../src/cases.h"
#include "../src/random.h"

namespace {

using test_support::install_names;
using test_support::make_pop;
using test_support::RuntimeNamesGuard;
using test_support::total;

constexpr auto S = TraitID::status;
constexpr auto NOW = Phase::now;
constexpr auto NEW = Phase::new_;

void set_day(int d) {
  sim::reset_day();
  for (int i = 0; i < d; ++i) sim::incr_day();
  sim::ds.day = d;
}

//
// social distancing
//
void test_sd_tagging_on_start_and_end_days() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {}, {"population"}, {"a", "b"});
  auto pop = make_pop(20000);
  // ineligible: infectious sick/severe and dead-with-severe
  for (size_t p = 1; p <= 1000; ++p) {
    pop.status[p] = INFECTIOUS;
    pop.cond[p] = p % 2 ? SICK : SEVERE;
  }
  for (size_t p = 1001; p <= 1500; ++p) { pop.status[p] = INFECTIOUS; pop.cond[p] = NIL; }  // eligible
  for (size_t p = 1501; p <= 1600; ++p) { pop.status[p] = DEAD; pop.cond[p] = SEVERE; }
  SocialDistancing a, b;
  a.name = "a"; a.startday = 5; a.endday = 10; a.comply = 0.5f;
  b.name = "b"; b.startday = 7; b.comply = 1.0f; b.include_ages = {AGE80_UP};
  const vector<SocialDistancing> cases{a, b};
  auto tagged = [&](uint8_t v) {
    int n = 0;
    for (size_t p = 1; p <= pop.popn; ++p) n += pop.sdcase[p].v == v;
    return n;
  };
  xo::seed(3);
  apply_sd_cases_for_day(4, cases, pop);
  CHECK_EQ(tagged(0), int(pop.popn));
  apply_sd_cases_for_day(5, cases, pop);
  int bad = 0;
  for (size_t p = 1; p <= 1000; ++p) bad += pop.sdcase[p].v != 0;
  for (size_t p = 1501; p <= 1600; ++p) bad += pop.sdcase[p].v != 0;
  CHECK_MSG(bad == 0, "{} ineligible (sick/severe/dead) people tagged", bad);
  const double eligible = pop.popn - 1100;
  CHECK_MSG(std::abs(tagged(1) / eligible - 0.5) < 0.02, "comply 0.5: tagged fraction {:.3f}", tagged(1) / eligible);
  const auto snapshot = pop.sdcase;
  apply_sd_cases_for_day(6, cases, pop);
  CHECK_MSG(pop.sdcase == snapshot, "tags changed on a day that is neither start nor end");
  apply_sd_cases_for_day(7, cases, pop);
  int old_untagged = 0, young_b = 0;
  for (size_t p = 1601; p <= pop.popn; ++p) {
    old_untagged += pop.agegrp[p] == AGE80_UP && pop.sdcase[p].v != 2;
    young_b += pop.agegrp[p] != AGE80_UP && pop.sdcase[p].v == 2;
  }
  CHECK_MSG(old_untagged == 0 && young_b == 0, "case b (age80_up, comply 1): {} old missed, {} young tagged",
            old_untagged, young_b);
  const int b_count = tagged(2);
  apply_sd_cases_for_day(10, cases, pop);
  CHECK_EQ(tagged(1), 0);           // case a cleared at endday
  CHECK_EQ(tagged(2), b_count);     // case b untouched (endday defaults to never)
  apply_sd_cases_for_day(100000, cases, pop);
  CHECK_EQ(tagged(2), b_count);
}

// whole run: identical to the control before the SD start day, fewer infections after
void test_sd_effect_on_simulation(const test_support::TestRunOptions&) {
  RuntimeNamesGuard guard;
  auto make = [](bool sd) {
    return test_support::build_small_case("sd_effect", 10000, 120, [sd](const test_support::fs::path& dir) {
      test_support::edit_config(dir, [sd](json& c) { c["do_social_distancing"] = sd; });
      json cases = json::array({{{"name", "all"}, {"startday", 30}, {"endday", 1000}, {"comply", 1.0},
                                 {"contact_delta", {0.0, 0.05}}, {"touch_delta", {0.0, 0.05}},
                                 {"include_ages", json::array()}}});
      test_support::write_case_json(dir, "soc_dist.json", cases);
    });
  };
  Model control = make(false);
  Histories hc = test_support::run_quiet(control);
  Model distanced = make(true);
  Histories hd = test_support::run_quiet(distanced);
  CHECK_EQ(invariants::check_run(control.pop, hc, "sd control"), 0);
  CHECK_EQ(invariants::check_run(distanced.pop, hd, "sd distanced"), 0);
  int differ_before = 0;
  for (size_t d = 1; d < 30; ++d)
    for (uint8_t v = 1; v <= Status::count; ++v)
      differ_before += total(hc, S, NOW, v, d) != total(hd, S, NOW, v, d);
  CHECK_MSG(differ_before == 0, "{} status totals differ before the SD start day", differ_before);
  long inf_c = 0, inf_d = 0;
  for (size_t d = 30; d <= 120; ++d) {
    inf_c += total(hc, S, NEW, INFECTIOUS.v, d);
    inf_d += total(hd, S, NEW, INFECTIOUS.v, d);
  }
  CHECK_MSG(inf_d < inf_c / 2, "new infections after day 30: distanced {} vs control {}", inf_d, inf_c);
  int tagged = 0;
  for (size_t p = 1; p <= distanced.pop.popn; ++p) tagged += distanced.pop.sdcase[p].v == 1;
  CHECK_MSG(tagged > 0, "no one tagged by the SD case");
}

//
// seed cases at runtime
//
SeedCase make_seed(int day, int count, vector<Term> filter, Condition cond = NIL, int duration = 1) {
  return SeedCase(day, true, Filter{std::move(filter)},
                  Change{{{"status", INFECTIOUS.v}, {"variant", 1}, {"cond", cond.v}, {"duration", duration}}, count});
}

void test_seed_case_counts_only_people_it_infects() {
  RuntimeNamesGuard guard;
  install_names({"base"}, {}, {"r1", "r2"});
  auto pop = make_pop(100, 2);  // ages in blocks of 20; rings alternate
  Histories h(5, pop);
  set_day(1);
  const auto first = make_seed(1, 3, {{"agegrp", AGE20_39.v}})(pop, h);
  REQUIRE(first.size() == 3);
  // a second identical seed must pick 3 *other* people
  const auto second = make_seed(1, 3, {{"agegrp", AGE20_39.v}})(pop, h);
  REQUIRE(second.size() == 3);
  CHECK_MSG(std::ranges::find_first_of(first, second) == first.end(), "second seed reused people {} / {}",
            first, second);
  h.sum_total_histories();
  CHECK_EQ(total(h, S, NEW, INFECTIOUS.v, 1), 6);

  // recovered people are eligible (reinfection), dead are not; filter terms are ANDed
  for (size_t p : first) pop.status[p] = DEAD;
  for (size_t p : second) { pop.status[p] = RECOVERED; pop.cond[p] = UNINFECTED; }
  test_support::StdoutCapture quiet;
  set_day(2);
  const auto third = make_seed(2, 50, {{"agegrp", AGE20_39.v}, {"ring", 2}}, SICK, 4)(pop, h);
  (void)quiet.finish();
  int bad = 0;
  for (size_t p : third)
    bad += pop.agegrp[p] != AGE20_39 || pop.ring[p].v != 2 || pop.status[p] != INFECTIOUS ||
           pop.cond[p] != SICK || pop.duration[p] != 4 || pop.sickday[p] != 2;
  CHECK_MSG(bad == 0, "{} seeded people violate the filter or the change terms", bad);
  // age20_39 = ids 21..40, ring 2 = even ids -> 10 people; 22 is dead (skipped),
  // 24 and 26 are recovered (eligible) -> 9 seeded, fewer than the 50 requested
  CHECK_EQ(third.size(), size_t{9});
  CHECK(std::ranges::find(third, size_t{22}) == third.end());
  CHECK(std::ranges::find(third, size_t{24}) != third.end());
}

// beginning-of-day and end-of-day seeds both run on their trigger day
void test_end_of_day_seeds_run() {
  RuntimeNamesGuard guard;
  Model m = test_support::build_small_case("eod_seeds", 5000, 6, [](const test_support::fs::path& dir) {
    json seeds = test_support::read_case_json(dir, "seed.json");
    json start = seeds[0], end = seeds[0];
    for (json* s : {&start, &end}) {
      (*s)["triggerday"] = 5;
      (*s)["filter"] = json::array({{{"trait", "agegrp"}, {"val", "age80_up"}}});
      (*s)["change"]["count"] = 10;
    }
    end["startofday"] = false;
    seeds.push_back(start);
    seeds.push_back(end);
    test_support::write_case_json(dir, "seed.json", seeds);
  });
  test_support::StdoutCapture cap;
  Histories h = runsim(m);
  const string out = cap.finish();
  CHECK_MSG(out.contains("Seed day 5 count: 10") && out.contains("Seed day 5 (end of day) count: 10"),
            "missing seed report lines:\n{}", out.substr(0, 2000));
  int seeded_day5 = 0;
  for (size_t p = 1; p <= m.pop.popn; ++p)
    seeded_day5 += m.pop.agegrp[p] == AGE80_UP && m.pop.sickday[p] == 5 && m.pop.status[p] == INFECTIOUS;
  CHECK_MSG(seeded_day5 >= 20, "{} age80_up people infected on day 5, want >= 20 (10 + 10 seeds)", seeded_day5);
  CHECK_EQ(invariants::check_run(m.pop, h, "eod_seeds"), 0);
}

}  // namespace

void run_social_distancing_tests(const test_support::TestRunOptions& options) {
  using tst::run_test;
  run_test("sd_tagging_on_start_and_end_days", test_sd_tagging_on_start_and_end_days);
  run_test("sd_effect_on_simulation", [&] { test_sd_effect_on_simulation(options); });
  run_test("seed_case_counts_only_people_it_infects", test_seed_case_counts_only_people_it_infects);
  run_test("end_of_day_seeds_run", test_end_of_day_seeds_run);
}
