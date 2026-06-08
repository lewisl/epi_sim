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
      .geodata = resolve_against(dir, cfg["geodata"].get<string>()).string(),
      .variants = resolve_against(dir, cfg["variants"].get<string>()).string(),
      .social_params = resolve_against(dir, cfg["social_params"].get<string>()).string(),
      .vaccines = resolve_against(dir, cfg["vaccines"].get<string>()).string(),
      .vax_sched_dir = resolve_against(dir, cfg["vax_sched_dir"].get<string>()).string(),
  };
}

std::string current_minute_prefix() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t t = std::chrono::system_clock::to_time_t(now);
  const std::tm lt = *std::localtime(&t);
  return fmt::format("{:02}_{:02}_{:04}_{:02}_{:02}",
                     lt.tm_mon + 1, lt.tm_mday, lt.tm_year + 1900,
                     lt.tm_hour, lt.tm_min);
}

// TODO need to get rid of hardcoded file paths
// Move any .html files in plot_output/ whose filename contains one of the
// given minute-prefixes (MM_DD_YYYY_HH_MM) into dest_dir. Captures the plots
// produced by this test run without needing to know their base titles.
void move_plot_htmls_for_minutes(const fs::path& dest_dir,
                                 const std::vector<std::string>& minute_prefixes) {
  const fs::path plot_dir = fs::path(std::getenv("HOME")) / "code" / "epi_sim" / "plot_output";
  if (!fs::exists(plot_dir)) return;
  for (const auto& entry : fs::directory_iterator(plot_dir)) {
    if (!entry.is_regular_file()) continue;
    if (entry.path().extension() != ".html") continue;
    const std::string name = entry.path().filename().string();
    for (const auto& p : minute_prefixes) {
      if (name.find(p) != std::string::npos) {
        fs::rename(entry.path(), dest_dir / entry.path().filename());  // risky way to do a move
        break;
      }
    }
  }
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

  Config config = build_test_config(config_path);
  Model model = setup_sim(config);

  CHECK(model.seedcases.size() == 2);
  CHECK(model.sd_cases.empty());

  const int seeded = 6;  // 3 Age20_39 + 3 Age40_59 from seed_test.json

  const std::string start_minute = current_minute_prefix();
  runsim(model);
  const std::string end_minute = current_minute_prefix();

  const RunsimResult r = tally(model.pop);

  CHECK(r.popn == 1000);
  CHECK(r.ever_infectious > seeded);
  CHECK(r.n_unexposed + r.n_infectious + r.n_recovered + r.n_dead == static_cast<int>(r.popn));
  CHECK(r.n_recovered > 0);
  CHECK(r.n_recovered < r.ever_infectious);

  if (options.write_artifacts) {
    const fs::path dest = test_support::artifact_group_dir(options, GROUP);
    std::vector<std::string> minutes{start_minute};
    if (end_minute != start_minute) minutes.push_back(end_minute);
    move_plot_htmls_for_minutes(dest, minutes);

    for (const auto& src : {config_path, fs::path(config.seed), fixtures_dir / "testgeo.csv"}) {
      fs::copy_file(src, dest / src.filename(), fs::copy_options::overwrite_existing);
    }

    std::ostringstream artifact;
    artifact << "Runsim end-to-end summary\n";
    artifact << "=========================\n\n";
    artifact << "config: " << config_path.string() << "\n";
    artifact << "seed:   " << config.seed << "\n";
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
