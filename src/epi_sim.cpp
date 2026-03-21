#include "epi_sim.h"
#include "setup.h"
#include "sim.h"


int main(int argc, char** argv) {
  fmt::println("Deeply Under Construction.");

  Model model = setup_sim(180, 27053, "2020-01-01", false);

  runsim(model);
  
  return 0;
}
