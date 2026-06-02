#include "cases.h"
#include "lib_includes.h"
#include "epi_sim.h"
#include "setup.h"
#include "sim.h"
#include <absl/strings/str_split.h>
#include "parameters.h"
#include "param_init.h"

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
  std::string case_name;


  // process cmdline --param and its value
  for (int i = 1; i < argc - 1; i += 2) {
      std::string flag = argv[i];
      std::string val  = argv[i + 1];
      if (flag == "--config") config_path = val;
      else if (flag == "--seed") seed_path = val;
      else if (flag == "--sd_seed") sd_seed_path = val;
      else if (flag == "--case-init") case_name = val;
      else {
          fmt::println(stderr, "Unknown flag: {}", flag);
          return 1;
      }
  }

  // create the scaffolding if present, then quit
  if (!empty(case_name)) {
    create_scaffold();
    return 0;
  }

  // end now if missing any required input
  if (config_path.empty() || seed_path.empty()) {
      fmt::println(stderr, "Usage: epi_sim --config <path> --seed <path> [--sd_seed <path>]");
      return 1;
  }

  // payloads for the required input parameters: load config_json and seed_json
  json config_json = load_json_params(config_path);
  json seed_json   = load_json_params(seed_path);

  const fs::path config_dir = fs::path(config_path).parent_path();

  Config config{
      .days = config_json["days"],
      .locale = config_json["locale"],
      .calendar_start = config_json["calendar_start"],
      .dovax = config_json["dovax"],
      .debug = config_json.value("debug", false),
      .geodata = resolve_config_path(config_dir, config_json, "geodata"),
      .variants = resolve_config_path(config_dir, config_json, "variants"),
      .social = resolve_config_path(config_dir, config_json, "social"),
      .vaccines = resolve_config_path(config_dir, config_json, "vaccines"),
      .vax_sched_dir = resolve_config_path(config_dir, config_json, "vax_sched_dir"),
      .rings = config_json.contains("rings")
                   ? resolve_config_path(config_dir, config_json, "rings")
                   : fs::path{},  // key absent = rings disabled
  };

  fmt::println("Setup simulation...");
  Model model = setup_sim(config);
  vector<SeedCase> seedcases = load_seed_cases(seed_json, model.pop, model.mp);
  vector<SocialDistancing> sd_cases;  // default constructor leaves this empty
  if (!sd_seed_path.empty()) {
    sd_cases = load_sd_cases(load_json_params(sd_seed_path), model.mp.socialdata);
  }
  fmt::println("Setup complete.");

  fmt::println("Starting simulation...");
  runsim(model, seedcases, sd_cases);
  
  return 0;
}
