
#include "lib_includes.h"

#include "parameters.h"
#include "population.h"
#include "random.h"
#include "sim.h"
#include "setup.h"




// TODO later add parameters for runcases, showr0, silent, dovax?, vaxscheds?
void runsim(Model model) {
  ModelParams mp = model.mp;

  // setup before day loop starts
  //    alias names for series columns
  

}





// TODO move this to a new source file disease_modeling.cpp eventually
// make one person sick
void PopData::make_sick(size_t p, uint8_t var, uint8_t condition, uint8_t durationdays) {
  cond[p] = condition;
  duration[p] = durationdays;
  status[p] = Trait::Stat::infectious;
  if (variant_count[p] < 16) {
    variant[p][variant_count[p]] = var;
    sickday[p][variant_count[p]] = sim::get_day();
    variant_count[p]++;
  } else {
      std::cerr << "Variant overflow for person " << p
              << ". Oldest variant lost.\n";
      std::cerr << "variant_count increased to " << variant_count[p] << "\n";  
      std::shift_left(variant[p].begin(), variant[p].end(), 1);
      variant[p].back() = var;
      std::shift_left(sickday[p].begin(), sickday[p].end(), 1);
      sickday[p].back() = sim::get_day();
      variant_count[p]++;
  }
}