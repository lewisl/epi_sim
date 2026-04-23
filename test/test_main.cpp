#include "test_support.h"

void run_pop_serialize_tests(const test_support::TestRunOptions& options);
void run_parameter_tests(const test_support::TestRunOptions& options);
void run_disease_modeling_tests(const test_support::TestRunOptions& options);
void run_vaccination_tests(const test_support::TestRunOptions& options);
void run_traits_tests(const test_support::TestRunOptions& options);
void run_series_tests(const test_support::TestRunOptions& options);
void run_setup_tests(const test_support::TestRunOptions& options);
void run_plot_tests(const test_support::TestRunOptions& options);

int main(int argc, char** argv) {
  const std::vector<std::string> groups = {"pop_serialize", "parameters", "disease_modeling", "vaccination", "traits", "series", "setup", "plot"};
  test_support::TestRunOptions options;
  options.artifact_root = test_support::project_dir() / "test_output";
  std::optional<std::string> selected_group;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--artifacts") {
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
    return 0;
  }

  const std::string& selected = *selected_group;
  if (selected == "pop_serialize") {
    run_pop_serialize_tests(options);
    return 0;
  }

  if (selected == "parameters") {
    run_parameter_tests(options);
    return 0;
  }

  if (selected == "disease_modeling") {
    run_disease_modeling_tests(options);
    return 0;
  }

  if (selected == "vaccination") {
    run_vaccination_tests(options);
    return 0;
  }

  if (selected == "traits") {
    run_traits_tests(options);
    return 0;
  }

  if (selected == "series") {
    run_series_tests(options);
    return 0;
  }

  if (selected == "setup") {
    run_setup_tests(options);
    return 0;
  }

  if (selected == "plot") {
    run_plot_tests(options);
    return 0;
  }

  fmt::println(stderr, "Unknown test group '{}'. Available groups: {}",
               selected, fmt::join(groups, ", "));
  return 1;
}