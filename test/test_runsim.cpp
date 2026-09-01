#include "test_support.h"

#include "../src/cases.h"
#include "../src/param_init.h"
#include "../src/parameters.h"
#include "../src/random.h"
#include "../src/setup.h"
#include "../src/sim.h"

namespace {

constexpr std::string_view GROUP = "runsim";

namespace fs = std::filesystem;

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
  test_support::RingNamesGuard ring_guard;
  Variant::names.clear();
  Vax::names.clear();

  const fs::path case_dir =
      test_support::home_dir() / test_support::unique_name("epi_sim_test_runsim_case_");
  setup_dir(case_dir.string());

  const fs::path config_path = case_dir / "input" / "config.json";
  json config = load_json_params(config_path.string());
  config["dovax"] = true;
  config["do_rings"] = true;
  {
    std::ofstream out(config_path);
    REQUIRE(out.good());
    out << config.dump(2);
  }

  // Model construction shuffles realized age groups and ring assignments.
  // Seed before build_model as well as relying on runsim's own fixed seed so
  // this fixture is reproducible across separate old/new binaries.
  xo::seed(424242);
  Model model = build_model(case_dir);
  // Exercises sanitize_filename_component's path-safety stripping (src/helpers.cpp) --
  // the only test in the suite that checks it.
  model.case_label = "runsim.test/../case";

  CHECK(model.seedcases.size() == 2);
  CHECK(model.sd_cases.empty());
  CHECK(model.ndays == 180);
  CHECK(model.dovax);
  CHECK(model.do_rings);
  CHECK(Vax::names.size() > 1);
  CHECK(Ring::names.size() > 1);

  const int seeded = 6;  // 3 Age20_39 + 3 Age40_59 from the scaffolded seed.json

  Histories series = runsim(model);

  const RunsimResult r = tally(model.pop);

  CHECK(r.ever_infectious > seeded);
  CHECK(r.n_unexposed + r.n_infectious + r.n_recovered + r.n_dead == static_cast<int>(r.popn));
  CHECK(r.n_recovered > 0);
  CHECK(r.n_dead > 0);
  CHECK(r.n_infectious <= seeded);
  CHECK(r.n_recovered + r.n_dead + r.n_infectious == r.ever_infectious);

  int series_count = 0;
  int pop_count = 0;
  int plot_count = 0;
  for (const auto& entry : fs::directory_iterator(model.output_dir)) {
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
    HistorySelectionSpec comprehensive("all");
    const auto aggregate_selections = comprehensive.selections;
    for (size_t ring = 1; ring < Ring::names.size(); ++ring) {
      for (auto selection : aggregate_selections) {
        selection.ring = Ring::names[ring];
        comprehensive.selections.push_back(std::move(selection));
      }
    }
    serialize_selected_histories(
        std::move(comprehensive), series,
        test_support::artifact_group_dir(options, GROUP) / "history_full.csv");

    std::ostringstream artifact;
    artifact << "Runsim end-to-end summary\n";
    artifact << "=========================\n\n";
    artifact << "case dir: " << case_dir.string() << "\n";
    artifact << "output:   " << model.output_dir.string() << "\n";
    artifact << "days:     " << model.ndays << "\n";
    artifact << "locale:   " << model.locale << "\n\n";
    artifact << "popn:            " << r.popn << "\n";
    artifact << "seeded:          " << seeded << "\n";
    artifact << "ever_infectious: " << r.ever_infectious << "\n";
    artifact << "unexposed:       " << r.n_unexposed << "\n";
    artifact << "infectious:      " << r.n_infectious << "\n";
    artifact << "recovered:       " << r.n_recovered << "\n";
    artifact << "dead:            " << r.n_dead << "\n";
    test_support::write_artifact_text(options, GROUP, "runsim_summary.txt", artifact.str());
  }

  if (!options.write_artifacts) fs::remove_all(case_dir);
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
