#include "parameters.h"
#include "population.h"
#include "r0_simulation.h"
#include "setup.h"


/*
    r0_sim(popsize, age_dist, progressionset, trvec, infectset, vaxset, 
                socialparams, density_factor=1.0, scale=3)

Simulates r0 or rt. The first method creates a population and tracks how many infections
are caused by first generation spreaders and NOT spreaders who were infected by the
first generation (or later). 
*/

/*.    mp -- modelparams
      .geodata = std::move(geodata),
      .variants = std::move(variants),
      .infectparams = std::move(infectparams),
      .progressionset = std::move(progressionset),
      .trvec = std::move(trvec),
      .socialdata = std::move(socialdata),
      .vaxset = std::move(vaxdata),
      .vaxschedset = std::move(vaxschedset),
      .ringtraits = std::move(ringtraits),
*/
void r0_sim(Model & model) {
  // extract required inputs
  int ndays = model.ndays;
  int locale = model.locale;
  bool do_rings = model.do_rings;
  bool dovax = model.dovax;
  int popn = 200'000;          // julia popsize
  PopData & pop = model.pop;          // julia locdat
  VaxSet & vaxset = model.mp.vaxset;  
  float density_factor = 1.0;
  int scale = 3;
  vector<double> age_dist = AGE_DIST;
  ProgressionSet & progressionset = model.mp.progressionset;
  vector<InfectParams> & infectparams = model.mp.infectparams;  // julia infectset
  SocialParams & socialparams = model.mp.socialdata;
  array<float, 6> & trvec = model.mp.trvec;

  Variant use_variant = 1;  // julia variant
  PopData r0_pop = PopData(popn, age_dist);

  fmt::println("ndays: {}  |  locale: {}  |  do_rings: {}  |", ndays, locale, do_rings);

}

void _run_r0_sim(PopData & r0pop, ProgressionSet & progressionset, array<float, 6> trvec,vector<InfectParams> & infectset, 
  VaxSet & vaxset,SocialParams & socialparams, bool dovax=false, Variant variant=1, float density_factor=1.0, int scale=3) {

}