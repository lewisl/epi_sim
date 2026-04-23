#include "test_support.h"

#include "../src/setup.h"

namespace {

constexpr std::string_view GROUP = "setup";

void test_sendrisk_indexing() {
  test_support::VariantNamesGuard variant_guard;
  Variant::names.clear();

  nlohmann::ordered_json jdata = {
      {"base", {{"spread", {{"sendrisk", std::vector<float>{0.0f, 0.0f, 0.3f, 0.65f, 0.75f, 0.85f}}, {"recvrisk", std::vector<float>{0.0f, 0.1f, 0.39f, 0.44f, 0.54f, 0.56f}}, {"basemultiplier", 1.0}}}, {"immunity", {{"recovery_immunity", {{"base", 0.8f}}}, {"immunehalflife", 360}}}}},
  };

  const auto [variants, infectparams] = load_variants_data(jdata);
  assert(variants.size() == infectparams.size());
  assert(variants.size() == 2);
  assert(variants[1].show() == "base");
  assert(infectparams[1].sendrisk.size() == 6);
  assert(approx_equal(infectparams[1].sendrisk[0], 0.0, 1e-9));
  assert(approx_equal(infectparams[1].sendrisk[2], 0.3, 1e-9));
  assert(approx_equal(infectparams[1].sendrisk[5], 0.85, 1e-9));
}

void test_setup_sim_builds_expected_model_shape() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names.clear();
  Vax::names.clear();

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

  Model model = setup_sim(config);

  assert(model.ndays == 366);
  assert(model.locale == 38015);
  assert(model.caldays.size() == size_t(model.ndays));
  assert(model.indoor_seq.size() == size_t(model.ndays));
  assert(model.day1 == absl::CivilDay(2020, 1, 1));
  assert(model.caldays.front() == absl::CivilDay(2020, 1, 1));
  assert(model.caldays.back() == absl::CivilDay(2020, 12, 31));

  const auto locale_it = std::find(model.mp.geodata.fips.begin(), model.mp.geodata.fips.end(), model.locale);
  assert(locale_it != model.mp.geodata.fips.end());
  const size_t locale_idx = static_cast<size_t>(std::distance(model.mp.geodata.fips.begin(), locale_it));
  assert(model.pop.popn == size_t(model.mp.geodata.pop[locale_idx]));
  assert(model.pop.popz == model.pop.popn + 1);
  assert(model.mp.variants[1].show() == "base");
}

void write_setup_artifacts(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;

  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names.clear();
  Vax::names.clear();

  nlohmann::ordered_json jdata = {
      {"base", {{"spread", {{"sendrisk", std::vector<float>{0.0f, 0.0f, 0.3f, 0.65f, 0.75f, 0.85f}}, {"recvrisk", std::vector<float>{0.0f, 0.1f, 0.39f, 0.44f, 0.54f, 0.56f}}, {"basemultiplier", 1.0}}}, {"immunity", {{"recovery_immunity", {{"base", 0.8f}}}, {"immunehalflife", 360}}}}},
  };
  const auto [variants, infectparams] = load_variants_data(jdata);

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
  Model model = setup_sim(config);

  std::ostringstream artifact;
  artifact << "Setup summary\n";
  artifact << "=============\n\n";
  artifact << "sendrisk fixture:\n";
  artifact << "  variants/infectparams: " << variants.size() << "/" << infectparams.size() << "\n";
  artifact << "  base sendrisk[0,2,5]: " << infectparams[1].sendrisk[0] << ", "
           << infectparams[1].sendrisk[2] << ", " << infectparams[1].sendrisk[5] << "\n\n";
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