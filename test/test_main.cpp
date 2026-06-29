#include "test_support.h"

// forward declaration of component test fuctions
// no includes needed for these because the files are added to the test build
void run_pop_serialize_tests(const test_support::TestRunOptions& options);
void run_parameter_tests(const test_support::TestRunOptions& options);
void run_disease_modeling_tests(const test_support::TestRunOptions& options);
void run_vaccination_tests(const test_support::TestRunOptions& options);
void run_traits_tests(const test_support::TestRunOptions& options);
void run_series_tests(const test_support::TestRunOptions& options);
void run_setup_tests(const test_support::TestRunOptions& options);
void run_plot_tests(const test_support::TestRunOptions& options);
void run_runsim_tests(const test_support::TestRunOptions& options);
void run_template_tests(const test_support::TestRunOptions& options);

int main(int argc, char** argv) {
  // "runsim" is intentionally excluded from the no-arg sweep below: it is slow,
  // writes CSV/plot artifacts to disk, and opens browser tabs. Run explicitly with
  // `xmake run test runsim`.
  const std::vector<std::string> groups = {"pop_serialize", "parameters", "disease_modeling", "vaccination", "traits", "series", "setup", "plot", "runsim", "templates"};
  test_support::TestRunOptions options;
  options.artifact_root = test_support::project_dir() / "test_output";
  std::optional<std::string> selected_group;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--artifacts" || arg == "--artifact") {
      options.write_artifacts = true;
      continue;
    }

    if (std::find(groups.begin(), groups.end(), arg) != groups.end()) {
      if (selected_group.has_value()) {
        fmt::println(stderr, "Only one test group may be selected at a time.");
        return 1;
      }
      selected_group = arg;
      continue;
    }

    fmt::println(stderr, "Unknown argument '{}'. Available groups: {}",
                 arg, fmt::join(groups, ", "));
    return 1;
  }

  if (!selected_group.has_value()) {
    run_pop_serialize_tests(options);
    run_parameter_tests(options);
    run_disease_modeling_tests(options);
    run_vaccination_tests(options);
    run_traits_tests(options);
    run_series_tests(options);
    run_setup_tests(options);
    run_plot_tests(options);
  } else {
    const std::string& selected = *selected_group;
    if (selected == "pop_serialize") {
      run_pop_serialize_tests(options);
    } else if (selected == "parameters") {
      run_parameter_tests(options);
    } else if (selected == "disease_modeling") {
      run_disease_modeling_tests(options);
    } else if (selected == "vaccination") {
      run_vaccination_tests(options);
    } else if (selected == "traits") {
      run_traits_tests(options);
    } else if (selected == "series") {
      run_series_tests(options);
    } else if (selected == "setup") {
      run_setup_tests(options);
    } else if (selected == "plot") {
      run_plot_tests(options);
    } else if (selected == "runsim") {
      run_runsim_tests(options);
    } else if (selected == "templates") {
      run_template_tests(options);
    }
  }

  test_summary();
  return test_failure_count() > 0 ? 1 : 0;
}
