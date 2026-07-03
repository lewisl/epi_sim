#include "sim.h"

double r0_sim(Model & model);
void rt_sim(PopData locdat, Model & model);
double run_rt_sim(Model & model, PopData & r0pop, Variant use_variant, int scale, vector<float> & indoor_seq);