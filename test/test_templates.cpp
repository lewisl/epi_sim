#include "test_support.h"

#include "../src/param_init.h"

#include <sys/wait.h>
#include <unistd.h>

namespace {

constexpr std::string_view GROUP = "templates";

using test_support::home_dir;
using test_support::unique_name;

test_support::fs::path project_config_path() {
  return home_dir() / ".config" / "epi_sim" / "project-dir.toml";
}

struct ProjectConfigGuard {
  test_support::fs::path path = project_config_path();
  bool existed = test_support::fs::exists(path);
  std::string saved_text = existed ? test_support::read_file_text(path) : std::string{};

  ~ProjectConfigGuard() {
    if (existed) {
      test_support::fs::create_directories(path.parent_path());
      std::ofstream out(path);
      out << saved_text;
    } else {
      std::error_code ec;
      test_support::fs::remove(path, ec);
    }
  }
};

template <typename Fn>
int child_exit_code(Fn fn) {
  std::cout.flush();
  std::cerr.flush();
  std::fflush(stdout);
  std::fflush(stderr);

  const pid_t pid = fork();
  REQUIRE(pid >= 0);
  if (pid == 0) {
    fn();
    _exit(EXIT_SUCCESS);
  }

  int status = 0;
  REQUIRE(waitpid(pid, &status, 0) == pid);
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
  return 255;
}

void write_project_config_for(const test_support::fs::path& project_dir) {
  const auto config_path = project_config_path();
  test_support::fs::create_directories(config_path.parent_path());
  std::ofstream out(config_path);
  REQUIRE(out.good());
  out << toml::table{{"project-dir", project_dir.string()}};
}

void require_regular_file(const test_support::fs::path& path) {
  CHECK(test_support::fs::exists(path));
  CHECK(test_support::fs::is_regular_file(path));
}

json load_template_json(const test_support::fs::path& path) {
  return load_json_params(path.string());
}

void verify_scaffolded_case(const test_support::fs::path& case_dir) {
  const auto input_dir = case_dir / "input";
  const auto output_dir = case_dir / "output";
  const auto vax_sched_dir = input_dir / "vaccine_100k";

  CHECK(test_support::fs::exists(case_dir));
  CHECK(test_support::fs::is_directory(case_dir));
  CHECK(test_support::fs::is_directory(input_dir));
  CHECK(test_support::fs::is_directory(output_dir));
  CHECK(test_support::fs::is_directory(vax_sched_dir));

  require_regular_file(input_dir / "config.json");
  require_regular_file(input_dir / "seed.json");
  require_regular_file(input_dir / "soc_dist.json");
  require_regular_file(input_dir / "socialparams.json");
  require_regular_file(input_dir / "variants.json");
  require_regular_file(input_dir / "vaccines.json");
  require_regular_file(input_dir / "geodata.csv");
  require_regular_file(input_dir / "rings.json");
  require_regular_file(vax_sched_dir / "loc38015_old.json");
  require_regular_file(vax_sched_dir / "loc38015_young.json");

  const json config = load_template_json(input_dir / "config.json");
  CHECK(config.contains("age_dist"));
  CHECK(config["age_dist"].is_array());
  CHECK(config["age_dist"].size() == 5);
  CHECK(config.contains("do_social_distancing"));
  CHECK(config.contains("do_rings"));
  CHECK(config.value("vax_sched_dir", "") == "vaccine_100k");
  CHECK(config.value("output", "") == "output");

  const json seed = load_template_json(input_dir / "seed.json");
  const json social_dist = load_template_json(input_dir / "soc_dist.json");
  const json socialparams = load_template_json(input_dir / "socialparams.json");
  const json variants = load_template_json(input_dir / "variants.json");
  const json vaccines = load_template_json(input_dir / "vaccines.json");
  const json rings = load_template_json(input_dir / "rings.json");
  const json old_sched = load_template_json(vax_sched_dir / "loc38015_old.json");
  const json young_sched = load_template_json(vax_sched_dir / "loc38015_young.json");

  CHECK(seed.is_array());
  CHECK(!seed.empty());
  CHECK(seed.front().contains("triggerday"));
  CHECK(seed.front().contains("change"));
  CHECK(social_dist.is_array());
  CHECK(!social_dist.empty());
  CHECK(social_dist.front().contains("name"));
  CHECK(social_dist.front().contains("contact_delta"));
  CHECK(socialparams.contains("contactfactors"));
  CHECK(variants.contains("base"));
  CHECK(variants["base"].contains("progression_tree"));
  CHECK(vaccines.contains("Pfizer"));
  CHECK(vaccines["Pfizer"].contains("effectiveness"));
  CHECK(rings.contains("rings"));
  CHECK(old_sched.contains("vaxesincluded"));
  CHECK(young_sched.contains("vaxesincluded"));
  CHECK(test_support::read_file_text(input_dir / "geodata.csv").find("fips") != std::string::npos);
}

void write_templates_artifact(const test_support::TestRunOptions& options,
                              const std::vector<test_support::fs::path>& dirs) {
  if (!options.write_artifacts) return;

  std::ostringstream artifact;
  artifact << "Template scaffold directories\n";
  artifact << "=============================\n\n";
  for (const auto& dir : dirs) artifact << dir.string() << "\n";

  test_support::write_artifact_text(options, GROUP, "scaffold_paths.txt", artifact.str());
}

void test_project_and_explicit_case_scaffolding(const test_support::TestRunOptions& options) {
  ProjectConfigGuard project_config_guard;

  const auto home = home_dir();
  const auto project_dir = home / unique_name("epi_sim_test_project_");
  const auto explicit_case_dir = home / unique_name("epi_sim_test_explicit_case_");
  const std::string case_a = unique_name("case_a_");
  const std::string case_b = unique_name("case_b_");

  std::vector<test_support::fs::path> created_dirs = {
      project_dir,
      project_dir / case_a,
      project_dir / case_b,
      explicit_case_dir,
  };

  set_project_dir(project_dir.string());
  init_case(case_a);
  init_case(case_b);
  setup_dir(explicit_case_dir.string());

  CHECK(test_support::fs::is_directory(project_dir));
  verify_scaffolded_case(project_dir / case_a);
  verify_scaffolded_case(project_dir / case_b);
  verify_scaffolded_case(explicit_case_dir);

  fmt::println("template scaffold project dir: {}", project_dir.string());
  fmt::println("template scaffold project case: {}", (project_dir / case_a).string());
  fmt::println("template scaffold project case: {}", (project_dir / case_b).string());
  fmt::println("template scaffold explicit case: {}", explicit_case_dir.string());

  write_templates_artifact(options, created_dirs);

  if (!options.write_artifacts) {
    std::error_code ec;
    test_support::fs::remove_all(project_dir, ec);
    test_support::fs::remove_all(explicit_case_dir, ec);
  }
}

void test_existing_active_project_dir_is_used_without_overwriting() {
  ProjectConfigGuard project_config_guard;

  const auto project_dir = home_dir() / unique_name("epi_sim_existing_project_");
  const auto marker_path = project_dir / "keep.txt";
  test_support::fs::create_directories(project_dir);
  {
    std::ofstream out(marker_path);
    REQUIRE(out.good());
    out << "existing project contents";
  }
  write_project_config_for(project_dir);

  set_project_dir(project_dir.string());

  CHECK(read_project_dir() == project_dir);
  CHECK(test_support::fs::exists(marker_path));
  CHECK(test_support::read_file_text(marker_path) == "existing project contents");

  std::error_code ec;
  test_support::fs::remove_all(project_dir, ec);
}

void test_missing_active_project_dir_is_created() {
  ProjectConfigGuard project_config_guard;

  const auto project_dir = home_dir() / unique_name("epi_sim_missing_active_project_");
  write_project_config_for(project_dir);

  set_project_dir(project_dir.string());

  CHECK(read_project_dir() == project_dir);
  CHECK(test_support::fs::is_directory(project_dir));

  std::error_code ec;
  test_support::fs::remove_all(project_dir, ec);
}

void test_different_existing_project_dir_is_activated_without_overwriting() {
  ProjectConfigGuard project_config_guard;

  const auto old_project_dir = home_dir() / unique_name("epi_sim_old_project_");
  const auto new_project_dir = home_dir() / unique_name("epi_sim_new_project_");
  const auto marker_path = new_project_dir / "keep.txt";
  test_support::fs::create_directories(old_project_dir);
  test_support::fs::create_directories(new_project_dir);
  {
    std::ofstream out(marker_path);
    REQUIRE(out.good());
    out << "existing project contents";
  }
  write_project_config_for(old_project_dir);

  set_project_dir(new_project_dir.string());

  CHECK(read_project_dir() == new_project_dir);
  CHECK(test_support::fs::exists(marker_path));
  CHECK(test_support::read_file_text(marker_path) == "existing project contents");

  std::error_code ec;
  test_support::fs::remove_all(old_project_dir, ec);
  test_support::fs::remove_all(new_project_dir, ec);
}

void test_different_missing_project_dir_is_created_and_activated() {
  ProjectConfigGuard project_config_guard;

  const auto old_project_dir = home_dir() / unique_name("epi_sim_old_project_");
  const auto new_project_dir = home_dir() / unique_name("epi_sim_new_project_");
  test_support::fs::create_directories(old_project_dir);
  write_project_config_for(old_project_dir);

  set_project_dir(new_project_dir.string());

  CHECK(read_project_dir() == new_project_dir);
  CHECK(test_support::fs::is_directory(new_project_dir));

  std::error_code ec;
  test_support::fs::remove_all(old_project_dir, ec);
  test_support::fs::remove_all(new_project_dir, ec);
}

void test_invalid_existing_project_path_does_not_repoint_config() {
  ProjectConfigGuard project_config_guard;

  const auto valid_project_dir = home_dir() / unique_name("epi_sim_valid_project_");
  const auto file_path = home_dir() / unique_name("epi_sim_project_path_file_");
  test_support::fs::create_directories(valid_project_dir);
  {
    std::ofstream out(file_path);
    REQUIRE(out.good());
    out << "not a directory";
  }
  write_project_config_for(valid_project_dir);

  const int exit_code = child_exit_code([&]() { set_project_dir(file_path.string()); });

  CHECK(exit_code != 0);
  CHECK(read_project_dir() == valid_project_dir);

  std::error_code ec;
  test_support::fs::remove_all(valid_project_dir, ec);
  test_support::fs::remove(file_path, ec);
}

void test_read_project_dir_rejects_missing_configured_directory() {
  ProjectConfigGuard project_config_guard;

  const auto missing_project_dir = home_dir() / unique_name("epi_sim_missing_project_");
  write_project_config_for(missing_project_dir);

  const int exit_code = child_exit_code([]() { read_project_dir(); });

  CHECK(exit_code != 0);
}

void test_show_project_dir_reports_missing_configured_directory_without_exit() {
  ProjectConfigGuard project_config_guard;

  const auto missing_project_dir = home_dir() / unique_name("epi_sim_missing_project_");
  write_project_config_for(missing_project_dir);

  show_project_dir();

  CHECK(!test_support::fs::exists(missing_project_dir));
  CHECK(test_support::read_file_text(project_config_path()).find(missing_project_dir.string()) != std::string::npos);
}

void test_existing_case_dir_gets_structural_dirs_without_template_overwrite() {
  const auto case_dir = home_dir() / unique_name("epi_sim_existing_case_");
  const auto input_dir = case_dir / "input";
  const auto marker_path = input_dir / "custom.json";

  test_support::fs::create_directories(input_dir);
  {
    std::ofstream out(marker_path);
    REQUIRE(out.good());
    out << R"({"custom": true})";
  }

  setup_dir(case_dir.string());

  CHECK(test_support::fs::is_directory(case_dir));
  CHECK(test_support::fs::is_directory(input_dir));
  CHECK(test_support::fs::is_directory(case_dir / "output"));
  CHECK(test_support::read_file_text(marker_path) == R"({"custom": true})");
  CHECK(!test_support::fs::exists(input_dir / "config.json"));

  std::error_code ec;
  test_support::fs::remove_all(case_dir, ec);
}

void test_existing_managed_case_dir_gets_structural_dirs_without_template_overwrite() {
  ProjectConfigGuard project_config_guard;

  const auto project_dir = home_dir() / unique_name("epi_sim_existing_case_project_");
  const std::string case_label = unique_name("case_");
  const auto case_dir = project_dir / case_label;
  const auto input_dir = case_dir / "input";
  const auto marker_path = input_dir / "custom.json";

  test_support::fs::create_directories(input_dir);
  {
    std::ofstream out(marker_path);
    REQUIRE(out.good());
    out << R"({"custom": true})";
  }
  set_project_dir(project_dir.string());

  init_case(case_label);

  CHECK(test_support::fs::is_directory(case_dir));
  CHECK(test_support::fs::is_directory(input_dir));
  CHECK(test_support::fs::is_directory(case_dir / "output"));
  CHECK(test_support::read_file_text(marker_path) == R"({"custom": true})");
  CHECK(!test_support::fs::exists(input_dir / "config.json"));

  std::error_code ec;
  test_support::fs::remove_all(project_dir, ec);
}

}  // namespace

void run_template_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running template scaffold tests...");
  test_project_and_explicit_case_scaffolding(options);
  test_existing_active_project_dir_is_used_without_overwriting();
  test_missing_active_project_dir_is_created();
  test_different_existing_project_dir_is_activated_without_overwriting();
  test_different_missing_project_dir_is_created_and_activated();
  test_invalid_existing_project_path_does_not_repoint_config();
  test_read_project_dir_rejects_missing_configured_directory();
  test_show_project_dir_reports_missing_configured_directory_without_exit();
  test_existing_case_dir_gets_structural_dirs_without_template_overwrite();
  test_existing_managed_case_dir_gets_structural_dirs_without_template_overwrite();
  if (options.write_artifacts) {
    fmt::println("template scaffold artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
  }
  fmt::println("template scaffold tests passed.");
}
