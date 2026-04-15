#include "lib_includes.h"
#include "epi_sim.h"
#include "setup.h"
#include "sim.h"
#include "parameters.h"

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

  for (int i = 1; i < argc - 1; i += 2) {
      std::string flag = argv[i];
      std::string val  = argv[i + 1];
      if (flag == "--config") config_path = val;
      else if (flag == "--seed") seed_path = val;
      else {
          fmt::println(stderr, "Unknown flag: {}", flag);
          return 1;
      }
  }

  if (config_path.empty() || seed_path.empty()) {
      fmt::println(stderr, "Usage: epi_sim --config <path> --seed <path>");
      return 1;
  }

  // load config and seed json
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
  };

  fmt::println("Setup simulation...");
  Model model = setup_sim(config);
  fmt::println("Setup complete.");

  vector<SeedCase> seedcases = load_seed_cases(seed_json, model.pop, model.mp);

  fmt::println("Starting simulation...");
  runsim(model, std::move(seedcases));
  
  return 0;
}
