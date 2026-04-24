#include "test_support.h"

namespace {

constexpr std::string_view GROUP = "traits";

template <typename Hist>
void check_scalar_history_overflow(Hist& history) {
  for (int day = 1; day <= 17; ++day) history.set(static_cast<int16_t>(day));

  CHECK(history.count == 17);
  CHECK(history.stored_count() == 16);
  CHECK(history.arr[0] == 2);
  CHECK(history.arr[15] == 17);
  CHECK(history.latest() == 17);
  CHECK(history.show() == "2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17");
}

void test_primitive_wrappers() {
  Duration duration;
  Ring ring;
  Sickday sickday;
  Recovday recovday;
  Deadday deadday;
  Testday testday;
  Quarday quarday;
  Vaxday vaxday;

  CHECK(duration == 0);
  CHECK(ring == 0);
  CHECK(sickday == 0);

  duration = 5;
  ring = 3;
  sickday = 12;
  recovday = 14;
  deadday = 21;
  testday = 8;
  quarday = 9;
  vaxday = 17;

  CHECK(duration.show() == "5");
  CHECK(ring.show() == "3");
  CHECK(sickday.show() == "12");
  CHECK(recovday.show() == "14");
  CHECK(deadday.show() == "21");
  CHECK(testday.show() == "8");
  CHECK(quarday.show() == "9");
  CHECK(vaxday.show() == "17");

  CHECK(duration < 6);
  CHECK(ring < 4);
  CHECK(4 < duration);
  Duration next = duration + 2;
  CHECK(next == 7);
  next += 3;
  CHECK(next == 10);
  const Duration before_post = next++;
  CHECK(before_post == 10);
  CHECK(next == 11);
  ++next;
  CHECK(next == 12);
}

void test_compile_time_trait_names_and_lookup() {
  CHECK(UNKNOWN.show() == "unknown");
  CHECK(AGE20_39.show() == "age20_39");
  CHECK(UNEXPOSED.show() == "unexposed");
  CHECK(RECOVERED.show() == "recovered");
  CHECK(UNINFECTED.show() == "uninfected");
  CHECK(SEVERE.show() == "severe");
  CHECK(Vaxstat::booster.show() == "booster");
  CHECK(Progressmap::ToDead.name() == "ToDead");

  CHECK(trait_from_string<Agegrp>("AGE20_39") == AGE20_39);
  CHECK(trait_from_string<Status>("infectious") == INFECTIOUS);
  CHECK(trait_from_string<Condition>("Mild") == MILD);
  CHECK(trait_from_string<Vaxstatus>("full") == Vaxstat::full);
  CHECK(!trait_from_string<Agegrp>("bad_age").has_value());
  CHECK(!trait_from_string<Status>("bad_status").has_value());
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

  CHECK(none_variant.show() == "none");
  CHECK(alpha.show() == "alpha");
  CHECK(delta.show() == "delta");
  CHECK(Variant::names.size() == 3);

  CHECK(none_vax.show() == "none");
  CHECK(pfizer.show() == "Pfizer");
  CHECK(Vax::names.size() == 2);

  CHECK(none_sd.show() == "none");
  CHECK(distancing.show() == "distancing");
  CHECK(SDCase::names.size() == 2);

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
  CHECK(variant_hist.count == 2);
  CHECK(variant_hist.latest() == Variant{2});
  CHECK(variant_hist.show() == "alpha|delta");

  VaxHist vax_hist;
  vax_hist.set(Vax{1});
  vax_hist.set(Vax{2});
  CHECK(vax_hist.count == 2);
  CHECK(vax_hist.latest() == Vax{2});
  CHECK(vax_hist.show() == "pfizer|moderna");
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
