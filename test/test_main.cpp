#include "test_support.h"

// group entry points (defined in the test_*.cpp files added to the test build)
void run_pop_serialize_tests(const test_support::TestRunOptions& options);
void run_parameter_tests(const test_support::TestRunOptions& options);
void run_disease_modeling_tests(const test_support::TestRunOptions& options);
void run_social_distancing_tests(const test_support::TestRunOptions& options);
void run_vaccination_tests(const test_support::TestRunOptions& options);
void run_traits_tests(const test_support::TestRunOptions& options);
void run_series_tests(const test_support::TestRunOptions& options);
void run_setup_tests(const test_support::TestRunOptions& options);
void run_plot_tests(const test_support::TestRunOptions& options);
void run_runsim_tests(const test_support::TestRunOptions& options);
void run_template_tests(const test_support::TestRunOptions& options);

namespace {

using GroupFn = void (*)(const test_support::TestRunOptions&);

struct Group {
  const char* name;
  GroupFn fn;
  bool in_sweep;  // part of the quick no-argument "sniff" run
};

// Ordered from foundations to whole-flow tests so the first failure is usually the root cause.
const std::vector<Group> GROUPS = {
    {"traits", run_traits_tests, true},
    {"parameters", run_parameter_tests, true},
    {"series", run_series_tests, true},
    {"disease_modeling", run_disease_modeling_tests, true},
    {"social_distancing", run_social_distancing_tests, true},
    {"vaccination", run_vaccination_tests, true},
    {"setup", run_setup_tests, true},
    {"plot", run_plot_tests, true},
    {"pop_serialize", run_pop_serialize_tests, true},
    // Deep tier, run explicitly: full scenarios through the TUI case-dir flow with goldens.
    {"runsim", run_runsim_tests, false},
    // Touches ~/.config/epi_sim (restored afterwards); run explicitly.
    {"templates", run_template_tests, false},
};

}  // namespace

int main(int argc, char** argv) {
  tst::install_crash_handler();
  test_support::suppress_browser();  // plots call `open`; never launch a browser from tests

  test_support::TestRunOptions options;
  options.artifact_root = test_support::project_dir() / "test_output";
  std::vector<std::string> selected;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--artifacts" || arg == "--artifact") {
      options.write_artifacts = true;
      continue;
    }
    const bool known = std::ranges::any_of(GROUPS, [&](const Group& g) { return arg == g.name; });
    if (!known) {
      std::vector<std::string> names;
      for (const auto& g : GROUPS) names.emplace_back(g.name);
      fmt::println(stderr, "Unknown argument '{}'. Groups: {}. Flag: --artifacts", arg,
                   fmt::join(names, ", "));
      return 2;
    }
    selected.push_back(arg);
  }

  for (const auto& g : GROUPS) {
    const bool run = selected.empty() ? g.in_sweep : std::ranges::find(selected, g.name) != selected.end();
    if (!run) continue;
    tst::begin_group(g.name);
    try {
      g.fn(options);
    } catch (const std::exception& e) {  // group setup outside run_test
      tst::check(false, "group completed", __FILE__, __LINE__, fmt::format("group threw: {}", e.what()));
    }
    tst::end_group();
  }

  return tst::summary();
}
