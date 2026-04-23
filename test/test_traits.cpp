#include "test_support.h"

namespace {

constexpr std::string_view GROUP = "traits";

template <typename Hist>
void check_scalar_history_overflow(Hist& history) {
  for (int day = 1; day <= 17; ++day) history.set(static_cast<int16_t>(day));

  assert(history.count == 17);
  assert(history.stored_count() == 16);
  assert(history.arr[0] == 2);
  assert(history.arr[15] == 17);
  assert(history.latest() == 17);
  assert(history.show() == "2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17");
}

void test_primitive_wrappers() {
  Duration duration;
  Sickday sickday;
  Recovday recovday;
  Deadday deadday;
  Testday testday;
  Quarday quarday;
  Vaxday vaxday;

  assert(duration == 0);
  assert(sickday == 0);

  duration = 5;
  sickday = 12;
  recovday = 14;
  deadday = 21;
  testday = 8;
  quarday = 9;
  vaxday = 17;

  assert(duration.show() == "5");
  assert(sickday.show() == "12");
  assert(recovday.show() == "14");
  assert(deadday.show() == "21");
  assert(testday.show() == "8");
  assert(quarday.show() == "9");
  assert(vaxday.show() == "17");

  assert(duration < 6);
  assert(4 < duration);
  Duration next = duration + 2;
  assert(next == 7);
  next += 3;
  assert(next == 10);
  const Duration before_post = next++;
  assert(before_post == 10);
  assert(next == 11);
  ++next;
  assert(next == 12);
}

void test_compile_time_trait_names_and_lookup() {
  assert(UNKNOWN.show() == "unknown");
  assert(AGE20_39.show() == "age20_39");
  assert(UNEXPOSED.show() == "unexposed");
  assert(RECOVERED.show() == "recovered");
  assert(UNINFECTED.show() == "uninfected");
  assert(SEVERE.show() == "severe");
  assert(Vaxstat::booster.show() == "booster");
  assert(Progressmap::ToDead.name() == "ToDead");

  assert(trait_from_string<Agegrp>("AGE20_39") == AGE20_39);
  assert(trait_from_string<Status>("infectious") == INFECTIOUS);
  assert(trait_from_string<Condition>("Mild") == MILD);
  assert(trait_from_string<Vaxstatus>("full") == Vaxstat::full);
  assert(!trait_from_string<Agegrp>("bad_age").has_value());
  assert(!trait_from_string<Status>("bad_status").has_value());
}

void test_runtime_traits_register_and_render_names() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  vector<string> saved_sdcase_names = SDCase::names;

  Variant::names.clear();
  Vax::names.clear();
  SDCase::names.clear();

  const Variant none_variant{"none"};
  const Variant alpha{"alpha"};
  const Variant delta{"delta"};

  const Vax none_vax{"none"};
  const Vax pfizer{"Pfizer"};

  const SDCase none_sd{"none"};
  const SDCase distancing{"distancing"};

  assert(none_variant.show() == "none");
  assert(alpha.show() == "alpha");
  assert(delta.show() == "delta");
  assert(Variant::names.size() == 3);

  assert(none_vax.show() == "none");
  assert(pfizer.show() == "Pfizer");
  assert(Vax::names.size() == 2);

  assert(none_sd.show() == "none");
  assert(distancing.show() == "distancing");
  assert(SDCase::names.size() == 2);

  SDCase::names = saved_sdcase_names;
}

void test_runtime_histories_render_values() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names = {"none", "alpha", "delta"};
  Vax::names = {"none", "pfizer", "moderna"};

  VariantHist variant_hist;
  variant_hist.set(Variant{1});
  variant_hist.set(Variant{2});
  assert(variant_hist.count == 2);
  assert(variant_hist.latest() == Variant{2});
  assert(variant_hist.show() == "alpha|delta");

  VaxHist vax_hist;
  vax_hist.set(Vax{1});
  vax_hist.set(Vax{2});
  assert(vax_hist.count == 2);
  assert(vax_hist.latest() == Vax{2});
  assert(vax_hist.show() == "pfizer|moderna");
}

void test_scalar_histories_overflow() {
  SickdayHist sickday_hist;
  RecovdayHist recovday_hist;
  TestdayHist testday_hist;
  VaxdayHist vaxday_hist;

  check_scalar_history_overflow(sickday_hist);
  check_scalar_history_overflow(recovday_hist);
  check_scalar_history_overflow(testday_hist);
  check_scalar_history_overflow(vaxday_hist);
}

void write_traits_artifact(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;

  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  vector<string> saved_sdcase_names = SDCase::names;

  std::ostringstream artifact;
  artifact << "Trait summary\n";
  artifact << "============\n\n";

  artifact << fmt::format("Agegrp names: [{}]\n", fmt::join(Agegrp::names, ", "));
  artifact << fmt::format("Status names: [{}]\n", fmt::join(Status::names, ", "));
  artifact << fmt::format("Condition names: [{}]\n", fmt::join(Condition::names, ", "));
  artifact << fmt::format("Vaxstatus names: [{}]\n\n", fmt::join(Vaxstatus::names, ", "));

  artifact << "trait_from_string examples:\n";
  artifact << "  AGE20_39 -> " << trait_from_string<Agegrp>("AGE20_39")->show() << "\n";
  artifact << "  infectious -> " << trait_from_string<Status>("infectious")->show() << "\n";
  artifact << "  Mild -> " << trait_from_string<Condition>("Mild")->show() << "\n";
  artifact << "  full -> " << trait_from_string<Vaxstatus>("full")->show() << "\n\n";

  Variant::names = {"none", "alpha", "delta"};
  Vax::names = {"none", "pfizer", "moderna"};
  SDCase::names = {"none", "distancing"};
  artifact << fmt::format("Variant names: [{}]\n", fmt::join(Variant::names, ", "));
  artifact << fmt::format("Vax names: [{}]\n", fmt::join(Vax::names, ", "));
  artifact << fmt::format("SDCase names: [{}]\n\n", fmt::join(SDCase::names, ", "));

  VariantHist variant_hist;
  variant_hist.set(Variant{1});
  variant_hist.set(Variant{2});
  VaxHist vax_hist;
  vax_hist.set(Vax{1});
  vax_hist.set(Vax{2});
  artifact << "rendered histories:\n";
  artifact << "  VariantHist: " << variant_hist.show() << "\n";
  artifact << "  VaxHist: " << vax_hist.show() << "\n";

  SickdayHist sickday_hist;
  for (int day = 1; day <= 17; ++day) sickday_hist.set(static_cast<int16_t>(day));
  artifact << "  SickdayHist overflow: " << sickday_hist.show() << "\n";
  artifact << "  SickdayHist latest/stored: " << sickday_hist.latest() << "/"
           << sickday_hist.stored_count() << "\n";

  test_support::write_artifact_text(options, GROUP, "trait_summary.txt", artifact.str());
  SDCase::names = saved_sdcase_names;
}

}  // namespace

void run_traits_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running traits tests...");
  test_primitive_wrappers();
  test_compile_time_trait_names_and_lookup();
  test_runtime_traits_register_and_render_names();
  test_runtime_histories_render_values();
  test_scalar_histories_overflow();
  write_traits_artifact(options);
  if (options.write_artifacts) {
    fmt::println("traits artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("traits tests passed.");
}