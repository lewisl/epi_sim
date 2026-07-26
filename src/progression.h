#pragma once

#include "population.h"
#include "parameters.h"

// before probvec  InfectParams &infpar, 
void progression(AgentView person, AllSeries & series, ProgressionSet &progset,
                 vector<InfectParams> &infectparams,
                 array<float, 6> &probvec, bool dovax, VaxSet &vaxset);
