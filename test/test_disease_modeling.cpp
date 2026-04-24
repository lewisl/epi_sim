#include "test_support.h"

#include "../src/disease_modeling.h"
#include "../src/series.h"
#include "../src/sim.h"

namespace {

constexpr std::string_view GROUP = "disease_modeling";

AllSeries make_series(const PopData& pop, size_t day_cnt) {
  return AllSeries(day_cnt, pop, Variant::names.size(), Vax::names.size());
}

void write_disease_modeling_artifact(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;

  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none", "base"};
  Vax::names = {"none", "pfizer"};

  std::ostringstream artifact;
  artifact << "Disease modeling summary\n";
  artifact << "========================\n\n";

  {
    PopData pop(1);
    AllSeries series = make_series(pop, 5);
    sim::reset_day();
    sim::incr_day();
    sim::ds.day = sim::get_day();
    auto person = pop.agent(1);
    person.make_sick(Variant{1}, series, MILD, 4);
    artifact << "make_sick day1:\n";
    artifact << "  status/cond/duration: " << person.status().show() << "/"
             << person.cond().show() << "/" << int(person.duration()) << "\n";
    artifact << "  variant/sickday: " << person.variant().show() << "/"
             << person.sickday() << "\n";
    artifact << "  series new_infectious/new_variant(base): "
             << series.new_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[1] << "/"
             << series.new_variant.at(uint8_t(Variant{1}), AgeBucket::total)[1] << "\n\n";
  }

  {
    PopData pop(1);
    AllSeries series = make_series(pop, 20);
    auto person = pop.agent(1);
    person.variant() = Variant{1};
    sim::reset_day();
    for (int day = 1; day <= 17; ++day) {
      sim::incr_day();
      sim::ds.day = sim::get_day();
      person.status() = INFECTIOUS;
      person.cond() = MILD;
      person.duration() = 3;
      person.make_well(series);
    }
    artifact << "make_well overflow:\n";
    artifact << "  recovday/latest/stored: " << person.recovday() << "/"
             << person.recovday_hist().latest() << "/"
             << person.recovday_hist().stored_count() << "\n";
    artifact << "  recovday_hist: " << person.recovday_hist().show() << "\n\n";
  }

  {
    PopData pop(1);
    AllSeries series = make_series(pop, 5);
    sim::reset_day();
    sim::incr_day();
    sim::ds.day = sim::get_day();
    auto person = pop.agent(1);
    person.make_sick(Variant{1}, series, SICK, 4);
    person.make_dead(series);
    artifact << "make_dead day1:\n";
    artifact << "  status/deadday: " << person.status().show() << "/"
             << person.deadday() << "\n";
    artifact << "  now_dead/now_base_variant: "
             << series.now_status.at(uint8_t(DEAD), AgeBucket::total)[1] << "/"
             << series.now_variant.at(uint8_t(Variant{1}), AgeBucket::total)[1] << "\n\n";
  }

  {
    PopData pop(2);
    pop.variant[1] = Variant{1};
    pop.recovday[1] = 10;
    pop.variant[2] = Variant{1};
    vector<InfectParams> infectparams(2);
    infectparams[1].recovery_immunity = {0.0f, 0.5f};
    infectparams[1].immunehalflife = 120;

    VaxParams pfizer;
    pfizer.halflife = 180;
    pfizer.full_effect_days = 14;
    pfizer.day1_effect = 0.65f;
    pfizer.infectfactor = {{"base", 0.9f}};
    pfizer.effectiveness = {
        {"first", {{"base", 0.7f}}},
        {"full", {{"base", 0.9f}}},
        {"booster", {{"base", 0.95f}}},
    };
    VaxSet vaxset;
    vaxset.params.push_back(VaxParams{});
    vaxset.params.push_back(pfizer);
    pop.vaxstatus[1] = Vaxstat::full;
    pop.vax[1] = Vax{1};
    pop.vaxday[1] = 10;

    artifact << "protection factors at day20:\n";
    artifact << "  recoveffect(recovered): " << recoveffect(pop.agent(1), 20, 1, infectparams) << "\n";
    artifact << "  recoveffect(naive): " << recoveffect(pop.agent(2), 20, 1, infectparams) << "\n";
    artifact << "  vaxeffect(full): " << vaxeffect(20, pop.agent(1), vaxset, 1) << "\n";
    artifact << "  vaxeffect(naive): " << vaxeffect(20, pop.agent(2), vaxset, 1) << "\n";
  }

  test_support::write_artifact_text(options, GROUP, "disease_modeling_summary.txt", artifact.str());
}

void test_make_sick_updates_state_and_series() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none", "base"};
  Vax::names = {"none"};

  PopData pop(1);
  AllSeries series = make_series(pop, 5);

  sim::reset_day();
  sim::incr_day();
  sim::ds.day = sim::get_day();

  auto person = pop.agent(1);
  person.make_sick(Variant{1}, series);

  CHECK(person.status() == INFECTIOUS);
  CHECK(person.cond() == NIL);
  CHECK(person.duration() == 1);
  CHECK(person.variant() == Variant{1});
  CHECK(person.sickday() == 1);
  CHECK(person.variant_hist().count == 1);
  CHECK(person.variant_hist().latest() == Variant{1});
  CHECK(person.sickday_hist().count == 1);
  CHECK(person.sickday_hist().latest() == 1);

  CHECK(series.new_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[1] == 1);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[1] == 1);
  CHECK(series.now_status.at(uint8_t(UNEXPOSED), AgeBucket::total)[1] == 0);
  CHECK(series.new_variant.at(uint8_t(Variant{1}), AgeBucket::total)[1] == 1);
  CHECK(series.now_variant.at(uint8_t(Variant{1}), AgeBucket::total)[1] == 1);
}

void test_make_well_updates_state_and_recovday_history() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none", "base"};
  Vax::names = {"none"};

  PopData pop(1);
  AllSeries series = make_series(pop, 20);
  auto person = pop.agent(1);
  person.variant() = Variant{1};

  sim::reset_day();

  for (int day = 1; day <= 17; ++day) {
    sim::incr_day();
    sim::ds.day = sim::get_day();

    person.status() = INFECTIOUS;
    person.cond() = MILD;
    person.duration() = 3;
    person.make_well(series);
  }

  CHECK(person.status() == RECOVERED);
  CHECK(person.cond() == UNINFECTED);
  CHECK(person.duration() == 0);
  CHECK(person.recovday() == 17);
  CHECK(person.recovday_hist().count == 17);
  CHECK(person.recovday_hist().stored_count() == 16);
  CHECK(person.recovday_hist().arr[0] == 2);
  CHECK(person.recovday_hist().arr[15] == 17);
  CHECK(person.recovday_hist().latest() == 17);
  CHECK(person.recovday_hist().show() == "2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17");
}

void test_make_dead_sets_death_state() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none", "base"};
  Vax::names = {"none"};

  PopData pop(1);
  AllSeries series = make_series(pop, 5);

  sim::reset_day();
  sim::incr_day();
  sim::ds.day = sim::get_day();

  auto person = pop.agent(1);
  person.make_sick(Variant{1}, series, SICK, 4);
  person.make_dead(series);

  CHECK(person.status() == DEAD);
  CHECK(person.deadday() == 1);
  CHECK(person.variant() == Variant{1});

  CHECK(series.new_status.at(uint8_t(DEAD), AgeBucket::total)[1] == 1);
  CHECK(series.now_status.at(uint8_t(DEAD), AgeBucket::total)[1] == 1);
  CHECK(series.now_status.at(uint8_t(INFECTIOUS), AgeBucket::total)[1] == 0);
  CHECK(series.now_variant.at(uint8_t(Variant{1}), AgeBucket::total)[1] == 0);
}

void test_recoveffect_uses_scalar_recovday() {
  PopData pop(2);
  pop.variant[1] = Variant{1};
  pop.recovday[1] = 10;
  pop.recovday_hist[1].count = 0;
  pop.variant[2] = Variant{1};
  pop.recovday[2] = 0;

  vector<InfectParams> infectparams(2);
  infectparams[1].recovery_immunity = {0.0f, 0.5f};
  infectparams[1].immunehalflife = 120;

  const float recovered_factor = recoveffect(pop.agent(1), 20, 1, infectparams);
  const float naive_factor = recoveffect(pop.agent(2), 20, 1, infectparams);

  CHECK(recovered_factor < 1.0f);
  CHECK(recovered_factor >= 0.0f);
  CHECK(approx_equal(naive_factor, 1.0f, 1e-6));
}

void test_vaxeffect_uses_scalar_latest_vax() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none", "base"};
  Vax::names = {"none", "pfizer"};

  PopData pop(2);
  pop.vaxstatus[1] = Vaxstat::full;
  pop.vax[1] = Vax{1};
  pop.vaxday[1] = 10;
  pop.vax_hist[1].count = 0;
  pop.vaxday_hist[1].count = 0;

  VaxParams pfizer;
  pfizer.halflife = 180;
  pfizer.full_effect_days = 14;
  pfizer.day1_effect = 0.65f;
  pfizer.infectfactor = {{"base", 0.9f}};
  pfizer.effectiveness = {
      {"first", {{"base", 0.7f}}},
      {"full", {{"base", 0.9f}}},
      {"booster", {{"base", 0.95f}}},
  };

  VaxSet vaxset;
  vaxset.params.push_back(VaxParams{});
  vaxset.params.push_back(pfizer);

  const float protected_factor = vaxeffect(20, pop.agent(1), vaxset, 1);
  const float naive_factor = vaxeffect(20, pop.agent(2), vaxset, 1);

  CHECK(protected_factor < 1.0f);
  CHECK(protected_factor >= 0.0f);
  CHECK(approx_equal(naive_factor, 1.0f, 1e-6));
}

}  // namespace

void run_disease_modeling_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running disease_modeling tests...");
  test_make_sick_updates_state_and_series();
  test_make_well_updates_state_and_recovday_history();
  test_make_dead_sets_death_state();
  test_recoveffect_uses_scalar_recovday();
  test_vaxeffect_uses_scalar_latest_vax();
  write_disease_modeling_artifact(options);
  if (options.write_artifacts) {
    fmt::println("disease_modeling artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("disease_modeling tests passed.");
}