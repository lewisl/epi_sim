#include "test_support.h"

#include "../src/setup.h"

namespace {

constexpr std::string_view GROUP = "setup";

Config sample_config() {
  const auto paths = test_support::sample_paths();
  Config config;
  config.days = 366;
  config.locale = 38015;
  config.calendar_start = "2020-01-01";
  config.dovax = false;
  config.geodata = paths.geodata;
  config.variants = paths.variants;
  config.social = paths.social;
  config.vaccines = paths.vaccines;
  config.vax_sched_dir = paths.vax_sched_dir;
  return config;
}

Model make_sample_model() {
  Variant::names.clear();
  Vax::names.clear();
  return setup_sim(sample_config());
}

void test_sendrisk_indexing() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Model model = make_sample_model();
  REQUIRE(model.mp.variants.size() > 1);
  REQUIRE(model.mp.infectparams.size() > 1);

  const auto& sendrisk = model.mp.infectparams[1].sendrisk;
  CHECK(model.mp.variants[1].show() == "base");
  REQUIRE(sendrisk.size() == 25);
  CHECK(approx_equal(sendrisk[0], 0.0, 1e-6));
  CHECK(approx_equal(sendrisk[1], 0.3, 1e-6));
  CHECK(approx_equal(sendrisk[2], 0.65, 1e-6));
  CHECK(approx_equal(sendrisk[5], 0.85, 1e-6));
  CHECK(approx_equal(sendrisk[24], 0.0, 1e-6));
  CHECK(model.mp.trvec.size() == 6);
}

void test_setup_sim_builds_expected_model_shape() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Model model = make_sample_model();

  CHECK(model.ndays == 366);
  CHECK(model.locale == 38015);
  CHECK(model.caldays.size() == size_t(model.ndays));
  CHECK(model.indoor_seq.size() == size_t(model.ndays));
  CHECK(model.day1 == absl::CivilDay(2020, 1, 1));
  CHECK(model.caldays.front() == absl::CivilDay(2020, 1, 1));
  CHECK(model.caldays.back() == absl::CivilDay(2020, 12, 31));

  const auto locale_it = std::find(model.mp.geodata.fips.begin(), model.mp.geodata.fips.end(), model.locale);
  REQUIRE(locale_it != model.mp.geodata.fips.end());
  const size_t locale_idx = static_cast<size_t>(std::distance(model.mp.geodata.fips.begin(), locale_it));
  CHECK(model.pop.popn == size_t(model.mp.geodata.pop[locale_idx]));
  CHECK(model.pop.popz == model.pop.popn + 1);
  CHECK(model.mp.variants[1].show() == "base");
}

void write_setup_artifacts(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;

  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Model model = make_sample_model();
  const auto& sendrisk = model.mp.infectparams[1].sendrisk;

  std::ostringstream artifact;
  artifact << "Setup summary\n";
  artifact << "=============\n\n";
  artifact << "setup_sim sendrisk:\n";
  artifact << "  variants/infectparams: " << model.mp.variants.size() << "/"
           << model.mp.infectparams.size() << "\n";
  artifact << "  base sendrisk size: " << sendrisk.size() << "\n";
  artifact << "  base sendrisk[0,1,2,5,24]: " << sendrisk[0] << ", "
           << sendrisk[1] << ", " << sendrisk[2] << ", " << sendrisk[5]
           << ", " << sendrisk[24] << "\n";
  artifact << "  progression trvec size: " << model.mp.trvec.size() << "\n\n";
  artifact << "setup_sim fixture:\n";
  artifact << "  ndays/locale: " << model.ndays << "/" << model.locale << "\n";
  artifact << "  first/last calday: " << absl::FormatCivilTime(model.caldays.front()) << " -> "
           << absl::FormatCivilTime(model.caldays.back()) << "\n";
  artifact << "  popn/popz: " << model.pop.popn << "/" << model.pop.popz << "\n";
  artifact << "  first variant: " << model.mp.variants[1].show() << "\n";

  test_support::write_artifact_text(options, GROUP, "setup_summary.txt", artifact.str());
}

}  // namespace

void run_setup_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running setup tests...");
  test_sendrisk_indexing();
  test_setup_sim_builds_expected_model_shape();
  write_setup_artifacts(options);
  if (options.write_artifacts) {
    fmt::println("setup artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("setup tests passed.");
}
