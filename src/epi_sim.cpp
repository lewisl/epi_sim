#include "lib_includes.h"
#include "epi_sim.h"
#include "setup.h"
#include "sim.h"
#include "parameters.h"


int main(int argc, char** argv) {
  fmt::println("Deeply Under Construction.");
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

  Config config{config_json["days"], config_json["locale"], config_json["calendar_start"], config_json["dovax"]};

  Model model = setup_sim(config);

  vector<SeedCase> seedcases = load_seed_cases(seed_json, model.pop, model.mp);

  runsim(model, std::move(seedcases));
  
  return 0;
}
