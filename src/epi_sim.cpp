#include "cases.h"
#include "lib_includes.h"
#include "epi_sim.h"
#include "setup.h"
#include "sim.h"
#include <absl/strings/str_split.h>
#include "parameters.h"
#include "param_init.h"
#include "show_help.h"
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

fs::path resolve_config_path(const fs::path& config_dir, const json& config_json, const char* key) {
  fs::path path = config_json[key].get<string>();
  if (path.is_absolute()) return path;
  return config_dir / path;
}

} // namespace


int main(int argc, char** argv) {

  std::string config_path;
  std::string seed_path;
  std::string sd_seed_path;
  
  std::string flag;
  std::string val;


  if (argc == 1) show_help(); 

  // process cmdline --param and its value
  
  // if (argc < 3 ) { std::fprintf(stderr, "Must provide 1 flag and 1 value\n"); std::exit(EXIT_FAILURE); }
  // TODO:  move val testing into loop because it always applies.
  // if (argv[2][0] == '-') { std::fprintf(stderr, "Must provide 1 flag and 1 value: you input 2 flags.\n"); std::exit(EXIT_FAILURE); }

  for (int i = 1; i < argc; i += 2) {
      flag = argv[i];
      val  = (i + 1) < argc ? argv[i + 1] : "";

      if (flag == "--set-project-dir") {
        if (val.empty()) { std::fprintf(stderr, "No value for project dir provided.\n"); std::exit(EXIT_FAILURE); }
        set_project_dir(val);
        exit(0);
      } 

      else if (flag == "--show-project-dir") {
        show_project_dir();
        exit(0);
      }

      else if (flag == "--init-case") {
        if (val.empty()) { std::fprintf(stderr, "No value for case dir provided.\n"); std::exit(EXIT_FAILURE); }
        init_case(val);
        exit(0);
      }

      else if (flag == "--use-case") {
        if (val.empty()) { std::fprintf(stderr, "No value for case dir provided.\n"); std::exit(EXIT_FAILURE); }
        
        fs::path config_path = use_case(val);
        fs::path input_dir = config_path.parent_path();
        fs::path case_dir = input_dir.parent_path();

        json config_json = load_json_params(config_path.string());

        Config config{
            .days = config_json["days"],
            .locale = config_json["locale"],
            .calendar_start = config_json["calendar_start"],
            .seed = resolve_config_path(input_dir, config_json, "seed").string(),
            .social_dist = config_json.contains("social_dist")
                ? resolve_config_path(input_dir, config_json, "social_dist").string()
                : "",
            .dovax = config_json["dovax"],
            .debug = config_json.value("debug", false),
            .geodata = resolve_config_path(input_dir, config_json, "geodata").string(),
            .variants = resolve_config_path(input_dir, config_json, "variants").string(),
            .social_params = resolve_config_path(input_dir, config_json, "social_params").string(),
            .vaccines = resolve_config_path(input_dir, config_json, "vaccines").string(),
            .vax_sched_dir = resolve_config_path(input_dir, config_json, "vax_sched_dir").string(),
            .rings = config_json.contains("rings")
                ? resolve_config_path(input_dir, config_json, "rings").string()
                : "",      // key absent = rings disabled
            .output_dir = config_json.contains("output")
                ? resolve_config_path(case_dir, config_json, "output").string()
                : (case_dir / "output").string()
          };

        fmt::println("Setup simulation...");
        Model model = setup_sim(config);
        fmt::println("Setup complete.");
        fmt::println("Starting simulation...");
        runsim(model);
      }

      else if (flag == "--setup-dir") {
        exit(0);
      }

      else if (flag == "--use-dir") {
        
      }

      else if (flag == "--help") {show_help(); exit(0);}

      else {
          fmt::println(stderr, "Unknown flag: {}", flag);
          exit(1);
      }
  }
  return 0;
}

  
