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
  config.seed = (test_support::project_dir() / "sample_parameters" / "seed_basic.json").string();
  config.geodata = paths.geodata;
  config.variants = paths.variants;
  config.social_params = paths.social;
  config.social_dist = (test_support::project_dir() / "sample_parameters" / "soc_dist.json").string();
  config.vaccines = paths.vaccines;
  config.vax_sched_dir = paths.vax_sched_dir;
  config.rings = (test_support::project_dir() / "sample_parameters" / "rings.json").string();
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

void test_social_distancing_file_is_not_loaded_when_flag_is_false() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Config config = sample_config();
  config.do_social_distancing = false;
  REQUIRE(!config.social_dist.empty());

  Model model = setup_sim(config);

  CHECK(!model.do_social_distancing);
  CHECK(model.sd_cases.empty());
}

void test_social_distancing_file_loads_when_flag_is_true() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Config config = sample_config();
  config.do_social_distancing = true;

  Model model = setup_sim(config);

  CHECK(model.do_social_distancing);
  CHECK(!model.sd_cases.empty());
}

void test_social_distancing_enabled_requires_path() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Config config = sample_config();
  config.do_social_distancing = true;
  config.social_dist = "";

  bool threw = false;
  try {
    (void)setup_sim(config);
  } catch (const std::runtime_error&) {
    threw = true;
  }

  CHECK(threw);
}

void test_rings_file_is_not_loaded_when_flag_is_false() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Config config = sample_config();
  config.do_rings = false;
  REQUIRE(!config.rings.empty());

  Model model = setup_sim(config);

  CHECK(!model.do_rings);
  CHECK(model.mp.ringtraits.ring_count() == 0);
  CHECK(model.ring_members.empty());
  CHECK(Ring::names.empty());
}

void test_rings_file_loads_when_flag_is_true() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Config config = sample_config();
  config.do_rings = true;

  Model model = setup_sim(config);

  CHECK(model.do_rings);
  CHECK(model.mp.ringtraits.ring_count() == 2);
  CHECK(model.ring_members.size() == 3);
  CHECK(Ring::names.size() == 3);
}

void test_rings_enabled_requires_path() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  Config config = sample_config();
  config.do_rings = true;
  config.rings = "";

  bool threw = false;
  try {
    (void)setup_sim(config);
  } catch (const std::runtime_error&) {
    threw = true;
  }

  CHECK(threw);
}

void test_rings_enabled_requires_top_level_rings_key() {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  test_support::RingNamesGuard ring_guard;
  const auto tmp_dir = test_support::fs::temp_directory_path() /
                       fmt::format("epi_sim_bad_rings_{}", std::random_device{}());
  test_support::fs::create_directories(tmp_dir);
  const auto bad_rings_path = tmp_dir / "bad_rings.json";
  {
    std::ofstream out(bad_rings_path);
    out << R"({"not_rings":[]})";
  }

  Config config = sample_config();
  config.do_rings = true;
  config.rings = bad_rings_path.string();

  bool threw = false;
  try {
    (void)setup_sim(config);
  } catch (const std::runtime_error&) {
    threw = true;
  }

  CHECK(threw);
  test_support::fs::remove_all(tmp_dir);
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
  test_social_distancing_file_is_not_loaded_when_flag_is_false();
  test_social_distancing_file_loads_when_flag_is_true();
  test_social_distancing_enabled_requires_path();
  test_rings_file_is_not_loaded_when_flag_is_false();
  test_rings_file_loads_when_flag_is_true();
  test_rings_enabled_requires_path();
  test_rings_enabled_requires_top_level_rings_key();
  write_setup_artifacts(options);
  if (options.write_artifacts) {
    fmt::println("setup artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("setup tests passed.");
}
