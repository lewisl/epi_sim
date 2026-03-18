#pragma once

#include "lib_includes.h"

#include "population.h"
#include "parameters.h"
#include "sim.h"

void spread(PopData &pop, DayData & series, size_t p, SocialParams &social, vector<InfectParams> &infectparams, vector<size_t> &contacts, float density_factor, vector<float> &indoor_seq);
// additional parameters later: dovax, vaxset,  sd_cases