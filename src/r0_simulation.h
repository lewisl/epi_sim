#include "sim.h"

void r0_sim(Model & model);
void _run_r0_sim(PopData & r0pop, ProgressionSet & progressionset, array<float, 6> trvec,vector<InfectParams> & infectset, 
  VaxSet & vaxset,SocialParams & socialparams, bool dovax=false, Variant variant=1, float density_factor=1.0, int scale=3);