#include "test_support.h"

#include "../src/cases.h"
#include "../src/param_init.h"
#include "../src/parameters.h"
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

  Model model = build_model(case_dir);
  // Exercises sanitize_filename_component's path-safety stripping (src/helpers.cpp) --
  // the only test in the suite that checks it.
  model.case_label = "runsim.test/../case";

  CHECK(model.seedcases.size() == 2);
  CHECK(model.sd_cases.empty());
  CHECK(model.ndays == 180);

  const int seeded = 6;  // 3 Age20_39 + 3 Age40_59 from the scaffolded seed.json

  runsim(model);

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
