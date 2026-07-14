#include "helpers.h"
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <string>
#include "setup.h"
#include "sim.h"
#include <absl/strings/str_split.h>
#include <toml++/toml.hpp>
#include "parameters.h"
#include <fmt/args.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include "param_init.h"
#include "template.h"
#include "input_verify.h"

namespace fs = std::filesystem;

void print_cli_block(std::string_view message, FILE* stream = stdout) {
  // fmt::println(stream, "---");

  for (std::string_view line : absl::StrSplit(message, '\n')) {
    fmt::println(stream, "  {}", line);
  }
}

//
// build the simulation model
//
Model build_model(fs::path case_dir) {
  fs::path config_path = config_path_for_case_dir(case_dir);
  if (!fs::exists(config_path)) {
    throw std::runtime_error(fmt::format("Path to config.json for case {} does not exist.\n", case_dir.string()));
    // fmt::println(stderr, "Path to config.json for case {} does not exist.\n", case_dir.string());
    // std::exit(EXIT_FAILURE);
  }

  fs::path input_dir = config_path.parent_path();
  input_verify(input_dir);
  json config_json = load_json_params(config_path.string());

  bool dovax = config_json["dovax"];

  Config config{
      .days = config_json["days"],
      .locale = config_json["locale"],
      .calendar_start = config_json["calendar_start"],
      .seed = resolve_config_path(input_dir, config_json, "seed").string(),
      .social_dist = resolve_optional_config_path(input_dir, config_json, "social_dist"),
      .dovax = dovax,
      .do_social_distancing = config_json.value("do_social_distancing", false),
      .do_rings = config_json.value("do_rings", false),
      .debug = config_json.value("debug", false),
      .rt_sim_interval = config_json["rt_sim_interval"],
      .age_dist = config_json["age_dist"],
      .geodata = resolve_config_path(input_dir, config_json, "geodata").string(),
      .variants = resolve_config_path(input_dir, config_json, "variants").string(),
      .social_params = resolve_config_path(input_dir, config_json, "social_params").string(),
      .vaccines = dovax ? resolve_config_path(input_dir, config_json, "vaccines").string()
                        : std::string{},
      .vax_sched_dir = dovax ? resolve_config_path(input_dir, config_json, "vax_sched_dir").string()
                             : std::string{},
      .rings = resolve_optional_config_path(input_dir, config_json, "rings"),
      .output_dir = config_json.contains("output")
          ? resolve_config_path(case_dir, config_json, "output").string()
          : (case_dir / "output").string(),
      .case_label = sanitize_filename_component(case_dir.lexically_normal().filename().string())
    };

  fmt::println("Setup simulation...");
  Model model = setup_sim(config);
  fmt::println("Setup complete.");
  return model;
}

//
// helper files for navigating project directories
//
std::optional<fs::path> resolve_home_path(const std::string& path_str) {
  const char* home_c = std::getenv("HOME");
  if (!home_c) {
    print_cli_block("HOME not set", stderr);
    std::exit(EXIT_FAILURE);
  }
  fs::path home{home_c};
  fs::path p;
  if (path_str == "~") return home;
  if (path_str.starts_with("~/")) {
    p = home / fs::path{path_str.substr(2)};
  } else if (!path_str.empty() && path_str[0] == '~') {
    print_cli_block("Only ~ and ~/ paths are supported.", stderr);
    return std::nullopt;
  } else {
    p = fs::path{path_str};
    if (p.is_relative()) p = home / p;
  }

  fs::path rel = fs::weakly_canonical(p).lexically_relative(fs::weakly_canonical(home));
  if (rel.empty() || rel.begin()->string() == "..") {
    print_cli_block("Absolute path input does not start at user's home directory.", stderr);
    std::exit(EXIT_FAILURE);
    return std::nullopt;
  }
  return p;
}

fs::path resolve_config_path(const fs::path& config_dir, const json& config_json, const char* key) {
  fs::path path = config_json[key].get<string>();
  if (path.is_absolute()) return path;
  return config_dir / path;
}

std::string resolve_optional_config_path(const fs::path& config_dir, const json& config_json, const char* key) {
  if (!config_json.contains(key) || config_json[key].is_null()) return "";
  std::string path_str = config_json[key].get<string>();
  if (path_str.empty()) return "";
  return resolve_config_path(config_dir, config_json, key).string();
}

fs::path read_project_dir() {
  const auto& config_file_path = resolve_home_path(".config/epi_sim/project-dir.toml");
  if (!config_file_path.has_value() || !fs::exists(*config_file_path)) {
    print_cli_block("project-dir.toml doesn't exist.\n"
        "Create it with epi_sim --set-project-dir <valid path for project dir>.", stderr);
    std::exit(EXIT_FAILURE);
  }
  if (!fs::is_regular_file(*config_file_path)) {
    print_cli_block("project-dir.toml is not a regular file.", stderr);
    std::exit(EXIT_FAILURE);
  }

  auto cfg = toml::parse_file(config_file_path->string());
  std::string project_dir_str = cfg["project-dir"].value_or(std::string{});
  if (project_dir_str.empty()) {
    print_cli_block("project-dir.toml does not contain a project-dir value.", stderr);
    std::exit(EXIT_FAILURE);
  }
  const auto & home_path = resolve_home_path(project_dir_str);
  if (!home_path.has_value()) {
    print_cli_block("Project directory path is invalid.", stderr);
    std::exit(EXIT_FAILURE);
  }
  if (!fs::exists(*home_path)) {
    print_cli_block(fmt::format("Configured project directory {} does not exist.", home_path->string()), stderr);
    std::exit(EXIT_FAILURE);
  }
  if (!fs::is_directory(*home_path)) {
    print_cli_block(fmt::format("Configured project directory {} is not a directory.", home_path->string()), stderr);
    std::exit(EXIT_FAILURE);
  }
  return *home_path;
}


void write_file(const std::string& content, std::string filename, std::string extension,
  fs::path path_name) {

    filename.append("." + extension);
    std::ofstream out(path_name / filename);
    if (!out) {
        throw std::runtime_error(
            fmt::format("Could not file to '{}'", path_name.string()));
    }

    out << content;
}

fs::path config_path_for_case_dir(const fs::path& case_dir) {
  return case_dir / "input" / "config.json";
}

fs::path resolve_explicit_case_dir(std::string path_arg) {
  auto const & p = resolve_home_path(path_arg);
  if (p.has_value()) return *p;
  else {print_cli_block(fmt::format("Invalid case directory path: {}.", path_arg), stderr);
        exit(1);}
}

void ensure_case_dirs(const fs::path& case_dir) {
  fs::create_directories(case_dir / "input");
  fs::create_directories(case_dir / "output");
}

//
// writes templates for parameter files in the project case directories
//
void create_scaffold(fs::path case_dir) {
  try {
  
    if (fs::exists(case_dir)) {
      if (!fs::is_directory(case_dir)) {
        print_cli_block(fmt::format("Case directory path {} exists but is not a directory.", case_dir.string()), stderr);
        std::exit(EXIT_FAILURE);
      }
      ensure_case_dirs(case_dir);
      print_cli_block(fmt::format("Using existing case directory {}; ensured input and output directories.", case_dir.string()));
      return;
    }

    fs::create_directories(case_dir);
    ensure_case_dirs(case_dir);

    // write input files to inputs subdir
      auto in = case_dir / "input";
      write_file(config_json,      "config",        "json", in);
      write_file(seed_json,        "seed",          "json", in);
      write_file(social_dist_json, "soc_dist",      "json", in);
      write_file(socialparams_json,"socialparams",  "json", in);
      write_file(variants_json,    "variants",      "json", in);
      write_file(vaccines_json,    "vaccines",      "json", in);
      write_file(geodata_csv,      "geodata",       "csv",  in);
      write_file(rings_json,       "rings",         "json", in);

      // read directory name for vax sched files
      json cfg = json::parse(config_json, nullptr, true, true);
      fs::path vax_sched = in / cfg["vax_sched_dir"].get<std::string>();
      //create directory and files
      fs::create_directories(vax_sched);
      write_file(vaxsched::loc38015_old_json,   "loc38015_old",   "json", vax_sched);
      write_file(vaxsched::loc38015_young_json, "loc38015_young", "json", vax_sched);
  }
    catch (const fs::filesystem_error& e) {
        print_cli_block(fmt::format("filesystem error: {} (path: {})", e.what(), e.path1().string()), stderr);
        exit(1);
    }
    catch (const std::exception& e) {
        print_cli_block(fmt::format("error creating scaffold: {}", e.what()), stderr);
        exit(1);
    }
}


//
// implement actions for cli flags
//
void set_project_dir(std::string val) {
    // write the toml file with the proposed value in the canonical location: ~/.config/epi_sim/project-dir.toml
    auto const & config_path = resolve_home_path(".config");
    if (!config_path) {print_cli_block("Could not resolve ~/.config.", stderr); std::exit(EXIT_FAILURE);}
    if (!fs::exists(*config_path)) { fs::create_directory(*config_path); 
      } else if (!fs::is_directory(*config_path)) {
        print_cli_block("~/.config is not a directory", stderr);
        std::exit(EXIT_FAILURE);          
      }
    // check for app dir epi_sim
    fs::path epi_sim_config_dir = *config_path / "epi_sim";
    if (!fs::exists(epi_sim_config_dir)) {
      fs::create_directory(epi_sim_config_dir);
    } else if (!fs::is_directory(epi_sim_config_dir)) {
        print_cli_block("~/.config/epi_sim is not a directory", stderr);
        std::exit(EXIT_FAILURE);          
      }
    auto const & p = resolve_home_path(val);
    if (!p) {
      print_cli_block(fmt::format("Invalid project directory path {}.", val), stderr);
      std::exit(EXIT_FAILURE);
    }

    std::optional<fs::path> current_project_dir;
    fs::path config_file_path = epi_sim_config_dir / "project-dir.toml";
    if (fs::exists(config_file_path) && fs::is_regular_file(config_file_path)) {
      try {
        auto cfg = toml::parse_file(config_file_path.string());
        std::string current_project_dir_str = cfg["project-dir"].value_or(std::string{});
        if (!current_project_dir_str.empty()) {
          current_project_dir = resolve_home_path(current_project_dir_str);
        }
      } catch (const std::exception& e) {
        print_cli_block(fmt::format("Replacing unreadable project config {}: {}",
                                    config_file_path.string(), e.what()), stderr);
      }
    }

    const bool already_active = current_project_dir.has_value()
        && fs::weakly_canonical(*current_project_dir) == fs::weakly_canonical(*p);

    if (fs::exists(*p)) {
      if (!fs::is_directory(*p)) {
        print_cli_block(fmt::format("Project directory path {} exists but is not a directory.", p->string()), stderr);
        std::exit(EXIT_FAILURE);
      }
      if (already_active) {
        print_cli_block(fmt::format("Project directory {} is already active.", p->string()));
      } else {
        print_cli_block(fmt::format("Activating existing project directory {}.", p->string()));
      }
    } else {
      fs::create_directories(*p);
      if (already_active) {
        print_cli_block(fmt::format("Created missing active project directory {}.", p->string()));
      } else {
        print_cli_block(fmt::format("Created and activated project directory {}.", p->string()));
      }
    }

    toml::table t{{"project-dir", p->string()}};
    std::ofstream out(config_file_path);
    if (!out) {
      print_cli_block(fmt::format("Could not write project config {}.", config_file_path), stderr);
      std::exit(EXIT_FAILURE);
    }
    out << t;
    out.close();

    show_project_dir();
}


void show_project_dir() {
    // check for canonical location of config file
    // show the dir, file name, and file contents
    auto const & config_file_path = resolve_home_path(".config/epi_sim/project-dir.toml");
    if (!config_file_path.has_value() || !fs::exists(*config_file_path)) {
      print_cli_block("project-dir.toml doesn't exist.\n"
          "Create it with epi_sim --set-project-dir <valid path for project dir>.", stderr);
    } else if (!fs::is_regular_file(*config_file_path)) {
      print_cli_block("project-dir.toml is not a regular file.", stderr);
      std::exit(EXIT_FAILURE);
    } else {
      // read and print the toml file
      std::ifstream f(*config_file_path);
      std::ostringstream ss;
      ss << f.rdbuf();
      print_cli_block(fmt::format("Contents of {}:\n  {}", config_file_path->string(), ss.str()));

      auto cfg = toml::parse_file(config_file_path->string());
      std::string project_dir_str = cfg["project-dir"].value_or(std::string{});
      if (project_dir_str.empty()) {
        print_cli_block("project-dir.toml does not contain a project-dir value.", stderr);
        std::exit(EXIT_FAILURE);
      }
      auto actual_project_dir = resolve_home_path(project_dir_str);
      if (!actual_project_dir.has_value()) {
        print_cli_block("Project directory path is invalid.", stderr);
        std::exit(EXIT_FAILURE);
      }

      if (!fs::exists(*actual_project_dir)) {
        print_cli_block(fmt::format(
            "Configured project directory {} does not exist.\n"
            "Recreate it with:\n"
            "  epi_sim --set-project-dir {}\n"
            "Or choose another project with:\n"
            "  epi_sim --set-project-dir <project-dir>",
            actual_project_dir->string(), project_dir_str));
      } else if (!fs::is_directory(*actual_project_dir)) {
        print_cli_block(fmt::format("Configured project directory {} is not a directory.",
                                    actual_project_dir->string()), stderr);
        std::exit(EXIT_FAILURE);
      } else {
        print_cli_block(fmt::format("Project directory {} contains the project-dir.toml file.",
                                    actual_project_dir->string()));
      }
    }
  }

void init_case(std::string case_label) {

  fs::path project_dir = read_project_dir();
  fs::path case_dir = project_dir / case_label;
  print_cli_block(fmt::format("Using {}", project_dir.string()));
  create_scaffold(case_dir);

}

void setup_dir(std::string path_arg) {
  fs::path case_dir = resolve_explicit_case_dir(path_arg);
  create_scaffold(case_dir);
}

Model use_managed_case(std::string case_label) {
  fs::path project_dir = read_project_dir();
  fs::path case_dir = project_dir / case_label;
  return build_model(case_dir);
}

void show_cases() {
  fs::path project_dir = read_project_dir();


  fs::directory_iterator Start{project_dir.string()};  
  fs::directory_iterator End{};                

  std::ostringstream cases;
  cases << fmt::format("Cases in {}:", project_dir.string()) << '\n';
  int cnt = 0;
  for (auto Iter{Start}; Iter != End; ++Iter) {
    cases << "  " << Iter->path().filename().string() << '\n';
    cnt++;
  }
  if (cnt == 0) cases << "  No cases initialized yet.";
  return print_cli_block(cases.str());
}

Model use_dir(std::string path_arg) {
  fs::path case_dir = resolve_explicit_case_dir(path_arg);
  return build_model(case_dir);
}

std::optional<Model> r0_sim_setup(std::string path_arg) {
  try {
    Model model = use_managed_case(path_arg);
    return model;
  } catch (const std::exception& e) {
    print_cli_block("Input case path is not a case-label in standard project location.\n  Trying to use input as a case directory...");
  }

  try {
    Model model = use_dir(path_arg);
    return model;
  } catch (const std::exception& e) {
      print_cli_block("Input case path is not a case-dir in the home directory.", stderr);
  }

  return {};  // must return valid object of type Model or null object
}
