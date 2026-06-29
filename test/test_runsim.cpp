#include "test_support.h"

#include "../src/cases.h"
#include "../src/parameters.h"
#include "../src/setup.h"
#include "../src/sim.h"

namespace {

constexpr std::string_view GROUP = "runsim";

namespace fs = std::filesystem;

fs::path resolve_against(const fs::path& dir, const std::string& s) {
  fs::path p(s);
  return p.is_absolute() ? p : dir / p;
}

Config build_test_config(const fs::path& config_path) {
  json cfg = load_json_params(config_path.string());
  const fs::path dir = config_path.parent_path();
  return Config{
      .days = cfg["days"],
      .locale = cfg["locale"],
      .calendar_start = cfg["calendar_start"],
      .seed = resolve_against(dir, cfg["seed"].get<string>()).string(),
      .social_dist = cfg.contains("social_dist")
                         ? resolve_against(dir, cfg["social_dist"].get<string>()).string()
                         : "",
      .dovax = cfg["dovax"],
      .debug = cfg.value("debug", false),
      .age_dist = {0.251, 0.271, 0.255, 0.184, 0.039},
      .geodata = resolve_against(dir, cfg["geodata"].get<string>()).string(),
      .variants = resolve_against(dir, cfg["variants"].get<string>()).string(),
      .social_params = resolve_against(dir, cfg["social_params"].get<string>()).string(),
      .vaccines = resolve_against(dir, cfg["vaccines"].get<string>()).string(),
      .vax_sched_dir = resolve_against(dir, cfg["vax_sched_dir"].get<string>()).string(),
  };
}

struct RunsimResult {
  size_t popn;
  int ever_infectious;
  int n_unexposed;
  int n_infectious;
  int n_recovered;
  int n_dead;
};

RunsimResult tally(const PopData& pop) {
  RunsimResult r{pop.popn, 0, 0, 0, 0, 0};
  for (size_t i = 1; i <= pop.popn; ++i) {
    if (pop.sickday[i] > 0) ++r.ever_infectious;
    switch (pop.status[i]) {
      case UNEXPOSED:  ++r.n_unexposed;  break;
      case INFECTIOUS: ++r.n_infectious; break;
      case RECOVERED:  ++r.n_recovered;  break;
      case DEAD:       ++r.n_dead;       break;
      default: break;
    }
  }
  return r;
}

void test_runsim_end_to_end(const test_support::TestRunOptions& options) {
  test_support::VariantNamesGuard variant_guard;
  test_support::VaxNamesGuard vax_guard;
  Variant::names.clear();
  Vax::names.clear();

  const fs::path root = test_support::project_dir();
  const fs::path fixtures_dir = root / "test" / "fixtures" / "runsim";
  const fs::path config_path = fixtures_dir / "config_test.json";
  const fs::path output_dir = options.write_artifacts
      ? test_support::artifact_group_dir(options, GROUP) / "case_output"
      : fs::temp_directory_path() / fmt::format("epi_sim_runsim_out_{}", std::random_device{}());
  fs::remove_all(output_dir);
  fs::create_directories(output_dir);

  Config config = build_test_config(config_path);
  config.output_dir = output_dir.string();
  config.case_label = "runsim.test/../case";
  Model model = setup_sim(config);

  CHECK(model.seedcases.size() == 2);
  CHECK(model.sd_cases.empty());

  const int seeded = 6;  // 3 Age20_39 + 3 Age40_59 from seed_test.json

  runsim(model);

  const RunsimResult r = tally(model.pop);

  CHECK(r.popn == 1000);
  CHECK(r.ever_infectious > seeded);
  CHECK(r.n_unexposed + r.n_infectious + r.n_recovered + r.n_dead == static_cast<int>(r.popn));
  CHECK(r.n_recovered > 0);
  CHECK(r.n_recovered < r.ever_infectious);

  int series_count = 0;
  int pop_count = 0;
  int plot_count = 0;
  for (const auto& entry : fs::directory_iterator(output_dir)) {
    if (!entry.is_regular_file()) continue;
    const std::string name = entry.path().filename().string();
    CHECK(name.find('/') == std::string::npos);
    CHECK(name.find("..") == std::string::npos);
    CHECK(name.rfind("runsimtestcase_", 0) == 0);
    if (name.find("_series_") != std::string::npos && entry.path().extension() == ".csv") ++series_count;
    if (name.find("_pop_") != std::string::npos && entry.path().extension() == ".csv") ++pop_count;
    if (entry.path().extension() == ".html") ++plot_count;
  }
  CHECK(series_count == 1);
  CHECK(pop_count == 1);
  CHECK(plot_count == 4);

  if (options.write_artifacts) {
    const fs::path dest = test_support::artifact_group_dir(options, GROUP);

    for (const auto& src : {config_path, fs::path(config.seed), fixtures_dir / "testgeo.csv"}) {
      fs::copy_file(src, dest / src.filename(), fs::copy_options::overwrite_existing);
    }

    std::ostringstream artifact;
    artifact << "Runsim end-to-end summary\n";
    artifact << "=========================\n\n";
    artifact << "config: " << config_path.string() << "\n";
    artifact << "seed:   " << config.seed << "\n";
    artifact << "output: " << output_dir.string() << "\n";
    artifact << "days:   " << model.ndays << "\n";
    artifact << "locale: " << model.locale << "\n\n";
    artifact << "popn:            " << r.popn << "\n";
    artifact << "seeded:          " << seeded << "\n";
    artifact << "ever_infectious: " << r.ever_infectious << "\n";
    artifact << "unexposed:       " << r.n_unexposed << "\n";
    artifact << "infectious:      " << r.n_infectious << "\n";
    artifact << "recovered:       " << r.n_recovered << "\n";
    artifact << "dead:            " << r.n_dead << "\n";
    test_support::write_artifact_text(options, GROUP, "runsim_summary.txt", artifact.str());
  }

  if (!options.write_artifacts) fs::remove_all(output_dir);
}

}  // namespace

void run_runsim_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running runsim end-to-end test...");
  test_runsim_end_to_end(options);
  if (options.write_artifacts) {
    fmt::println("runsim artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("runsim tests passed.");
}
