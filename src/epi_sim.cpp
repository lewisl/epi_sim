#include "epi_sim.h"
#include "setup.h"
#include "sim.h"


int main(int argc, char** argv) {
  fmt::println("Deeply Under Construction.");

  Model model = setup_sim(180, 38015, "2020-01-01", false);
  std::filesystem::path trace_path{};
  std::filesystem::path spread_debug_prefix{};
  if (argc > 1) {
    trace_path = argv[1];
  }
  if (argc > 2) {
    spread_debug_prefix = argv[2];
  }
  runsim(model, trace_path, spread_debug_prefix);
  
  return 0;
}
