#include "test_support.h"

#include "../src/random.h"
#include "../src/series.h"
#include "../src/vaccination.h"

namespace {

constexpr std::string_view GROUP = "vaccination";

AllSeries make_series(const PopData& pop, size_t day_cnt) {
  size_t n_ring_slots = std::max<size_t>(Ring::names.size(), 1);
  return AllSeries(day_cnt, pop, Variant::names.size(), Vax::names.size(), n_ring_slots);
}

VaxSet make_pfizer_set(int reqdshots, int delay2ndshot, int delaybooster) {
  VaxParams pfizer;
  pfizer.reqdshots = reqdshots;
  pfizer.delay2ndshot = delay2ndshot;
  pfizer.delaybooster = delaybooster;

  VaxSet vaxset;
  vaxset.params.push_back(VaxParams{});
  vaxset.params.push_back(pfizer);
  return vaxset;
}

VaxSchedSet make_pfizer_schedset(int doses, std::pair<int, int> dayrange = {1, 30}) {
  PerVaxSpec spec;
  spec.vax = Vax{1};
  spec.mix = 1.0f;
  spec.starting_doses = doses;
  spec.doses = doses;
  spec.pct2ndshot = 1.0f;
  spec.pctboost = 1.0f;

  VaxSched sched;
  sched.vaxesincluded.push_back(spec);
  sched.dayrange = dayrange;
  sched.targetpct = 1.0f;
  sched.filtervec = {AGE20_39};
  sched.shotmode = "all";
  sched.pattern = {1.0f, 1.0f};
  sched.spreadfunc = [](int) { return 1.0f; };

  VaxSchedSet schedset;
  schedset.schedules.push_back({"unit_test_sched", sched});
  return schedset;
}

int vaccinated_count(const PopData& pop) {
  int total = 0;
  for (size_t p = 1; p <= pop.popn; ++p) {
    if (pop.vaxstatus[p] != Vaxstat::none) ++total;
  }
  return total;
}

void set_agegrp(PopData& pop, Agegrp agegrp) {
  for (size_t p = 1; p <= pop.popn; ++p) pop.agegrp[p] = agegrp;
}

void set_only_people_in_agegrp(PopData& pop, std::initializer_list<size_t> people,
                               Agegrp agegrp) {
  set_agegrp(pop, AGE40_59);
  for (const size_t p : people) pop.agegrp[p] = agegrp;
}

void test_vaccinate_first_shot_respects_supply_limit_and_updates_series() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none"};
  Vax::names = {"none", "pfizer"};

  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
  set_agegrp(pop, AGE20_39);
  AllSeries series = make_series(pop, 20);
  VaxSet vaxset = make_pfizer_set(1, 0, 999);
  VaxSchedSet schedset = make_pfizer_schedset(1);

  xo::seed(1);
  vaccinate(10, schedset, vaxset, pop, series);

  CHECK(vaccinated_count(pop) == 1);
  CHECK(schedset.schedules[0].second.vaxesincluded[0].doses == 0);
  CHECK(series.new_vax.at(uint8_t(Vax{1}), AgeBucket::total)[10] == 1);
  CHECK(series.now_vax.at(uint8_t(Vax{1}), AgeBucket::total)[10] == 1);
}

void test_vaccinate_uses_scalar_recovday_eligibility() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none"};
  Vax::names = {"none", "pfizer"};

  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
  set_only_people_in_agegrp(pop, {1, 2, 3}, AGE20_39);
  pop.status[2] = RECOVERED;
  pop.recovday[2] = 5;
  pop.status[3] = RECOVERED;
  pop.recovday[3] = 10;

  AllSeries series = make_series(pop, 30);
  VaxSet vaxset = make_pfizer_set(1, 0, 999);
  VaxSchedSet schedset = make_pfizer_schedset(10);

  xo::seed(2);
  vaccinate(20, schedset, vaxset, pop, series);

  CHECK(pop.vaxstatus[1] == Vaxstat::full);
  CHECK(pop.vaxstatus[2] == Vaxstat::full);
  CHECK(pop.vaxstatus[3] == Vaxstat::none);
  CHECK(pop.vax_hist[1].count == 1);
  CHECK(pop.vax_hist[2].count == 1);
  CHECK(pop.vax_hist[3].count == 0);
  CHECK(series.new_vax.at(uint8_t(Vax{1}), AgeBucket::total)[20] == 2);
}

void test_vaccinate_second_shot_after_delay() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none"};
  Vax::names = {"none", "pfizer"};

  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
  set_only_people_in_agegrp(pop, {1}, AGE20_39);
  pop.vaxstatus[1] = Vaxstat::first;
  pop.vax[1] = Vax{1};
  pop.vaxday[1] = 1;
  pop.vax_hist[1].set(Vax{1});
  pop.vaxday_hist[1].set(1);

  AllSeries series = make_series(pop, 20);
  VaxSet vaxset = make_pfizer_set(2, 7, 999);
  VaxSchedSet schedset = make_pfizer_schedset(2, {1, 5});

  vaccinate(10, schedset, vaxset, pop, series);

  CHECK(pop.vaxstatus[1] == Vaxstat::full);
  CHECK(pop.vaxday[1] == 10);
  CHECK(pop.vax_hist[1].count == 2);
  CHECK(pop.vax_hist[1].latest() == Vax{1});
  CHECK(pop.vaxday_hist[1].latest() == 10);
  CHECK(series.new_vax.at(uint8_t(Vax{1}), AgeBucket::total)[10] == 0);
}

void test_vaccinate_booster_after_delay() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none"};
  Vax::names = {"none", "pfizer"};

  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
  set_only_people_in_agegrp(pop, {1}, AGE20_39);
  pop.vaxstatus[1] = Vaxstat::full;
  pop.vax[1] = Vax{1};
  pop.vaxday[1] = 1;
  pop.vax_hist[1].set(Vax{1});
  pop.vaxday_hist[1].set(1);

  AllSeries series = make_series(pop, 30);
  VaxSet vaxset = make_pfizer_set(2, 7, 10);
  VaxSchedSet schedset = make_pfizer_schedset(2);

  vaccinate(20, schedset, vaxset, pop, series);

  CHECK(pop.vaxstatus[1] == Vaxstat::booster);
  CHECK(pop.vaxday[1] == 20);
  CHECK(pop.vax_hist[1].count == 2);
  CHECK(pop.vaxday_hist[1].latest() == 20);
  CHECK(series.new_vax.at(uint8_t(Vax{1}), AgeBucket::total)[20] == 0);
}

void test_vax_history_overflow_retains_latest_days() {
  test_support::VaxNamesGuard vax_guard;
  Vax::names = {"none", "pfizer"};

  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
  for (int day = 1; day <= 17; ++day) {
    pop.vax[1] = Vax{1};
    pop.vaxday[1] = static_cast<int16_t>(day);
    pop.vax_hist[1].set(Vax{1});
    pop.vaxday_hist[1].set(static_cast<int16_t>(day));
  }

  CHECK(pop.vax_hist[1].count == 17);
  CHECK(pop.vax_hist[1].stored_count() == 16);
  CHECK(pop.vaxday_hist[1].count == 17);
  CHECK(pop.vaxday_hist[1].stored_count() == 16);
  CHECK(pop.vaxday_hist[1].arr[0] == 2);
  CHECK(pop.vaxday_hist[1].arr[15] == 17);
}

void write_vaccination_artifact(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;

  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none"};
  Vax::names = {"none", "pfizer"};

  std::ostringstream artifact;
  artifact << "Vaccination summary\n";
  artifact << "===================\n\n";

  {
    PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
    set_agegrp(pop, AGE20_39);
    AllSeries series = make_series(pop, 20);
    VaxSet vaxset = make_pfizer_set(1, 0, 999);
    VaxSchedSet schedset = make_pfizer_schedset(1);
    xo::seed(1);
    vaccinate(10, schedset, vaxset, pop, series);
    artifact << "first shot at day10:\n";
    artifact << "  vaccinated_count: " << vaccinated_count(pop) << "\n";
    artifact << "  remaining doses: " << schedset.schedules[0].second.vaxesincluded[0].doses << "\n";
    artifact << "  new_vax/now_vax day10: "
             << series.new_vax.at(uint8_t(Vax{1}), AgeBucket::total)[10] << "/"
             << series.now_vax.at(uint8_t(Vax{1}), AgeBucket::total)[10] << "\n\n";
  }

  {
    PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
    set_only_people_in_agegrp(pop, {1, 2, 3}, AGE20_39);
    pop.status[2] = RECOVERED;
    pop.recovday[2] = 5;
    pop.status[3] = RECOVERED;
    pop.recovday[3] = 10;
    AllSeries series = make_series(pop, 30);
    VaxSet vaxset = make_pfizer_set(1, 0, 999);
    VaxSchedSet schedset = make_pfizer_schedset(10);
    xo::seed(2);
    vaccinate(20, schedset, vaxset, pop, series);
    artifact << "recovered eligibility at day20:\n";
    artifact << "  statuses: " << pop.vaxstatus[1].show() << ", "
             << pop.vaxstatus[2].show() << ", " << pop.vaxstatus[3].show() << "\n";
    artifact << "  new_vax day20: "
             << series.new_vax.at(uint8_t(Vax{1}), AgeBucket::total)[20] << "\n\n";
  }

  {
    PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
    set_only_people_in_agegrp(pop, {1}, AGE20_39);
    pop.vaxstatus[1] = Vaxstat::first;
    pop.vax[1] = Vax{1};
    pop.vaxday[1] = 1;
    pop.vax_hist[1].set(Vax{1});
    pop.vaxday_hist[1].set(1);
    AllSeries series = make_series(pop, 20);
    VaxSet vaxset = make_pfizer_set(2, 7, 999);
    VaxSchedSet schedset = make_pfizer_schedset(2, {1, 5});
    vaccinate(10, schedset, vaxset, pop, series);
    artifact << "second shot upgrade:\n";
    artifact << "  status/vaxday/history_count: " << pop.vaxstatus[1].show() << "/"
             << pop.vaxday[1] << "/" << int(pop.vax_hist[1].count) << "\n\n";
  }

  {
    PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
    set_only_people_in_agegrp(pop, {1}, AGE20_39);
    pop.vaxstatus[1] = Vaxstat::full;
    pop.vax[1] = Vax{1};
    pop.vaxday[1] = 1;
    pop.vax_hist[1].set(Vax{1});
    pop.vaxday_hist[1].set(1);
    AllSeries series = make_series(pop, 30);
    VaxSet vaxset = make_pfizer_set(2, 7, 10);
    VaxSchedSet schedset = make_pfizer_schedset(2);
    vaccinate(20, schedset, vaxset, pop, series);
    artifact << "booster upgrade:\n";
    artifact << "  status/vaxday/latest_hist_day: " << pop.vaxstatus[1].show() << "/"
             << pop.vaxday[1] << "/" << pop.vaxday_hist[1].latest() << "\n\n";
  }

  {
    PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});
    for (int day = 1; day <= 17; ++day) {
      pop.vax[1] = Vax{1};
      pop.vaxday[1] = static_cast<int16_t>(day);
      pop.vax_hist[1].set(Vax{1});
      pop.vaxday_hist[1].set(static_cast<int16_t>(day));
    }
    artifact << "history overflow:\n";
    artifact << "  vaxday_hist latest/stored: " << pop.vaxday_hist[1].latest() << "/"
             << pop.vaxday_hist[1].stored_count() << "\n";
    artifact << "  vaxday_hist range: " << pop.vaxday_hist[1].arr[0] << " -> "
             << pop.vaxday_hist[1].arr[15] << "\n";
  }

  test_support::write_artifact_text(options, GROUP, "vaccination_summary.txt", artifact.str());
}

}  // namespace

void run_vaccination_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running vaccination tests...");
  test_vaccinate_first_shot_respects_supply_limit_and_updates_series();
  test_vaccinate_uses_scalar_recovday_eligibility();
  test_vaccinate_second_shot_after_delay();
  test_vaccinate_booster_after_delay();
  test_vax_history_overflow_retains_latest_days();
  write_vaccination_artifact(options);
  if (options.write_artifacts) {
    fmt::println("vaccination artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("vaccination tests passed.");
}
