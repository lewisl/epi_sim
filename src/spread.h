#pragma once

#include "lib_includes.h"

#include "population.h"
#include "parameters.h"
#include "sim.h"
#include "series.h"

void spread(PopData &pop, AllSeries & series, AgentView person, SocialParams &social,
            vector<InfectParams> &infectparams, const VaxSet& vaxset, bool dovax,
            vector<size_t> &contacts, float density_factor, vector<float> &indoor_seq);
// additional parameters later: dovax, vaxset,  sd_cases
