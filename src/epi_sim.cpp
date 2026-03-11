#include "epi_sim.h"
#include "setup.h"
#include "sim.h"


int main() {
  fmt::println("Deeply Under Construction.");

  Model model = setup_sim(180, 38015, "2020-01-01", false);
  runsim(model);
  
  return 0;
}
