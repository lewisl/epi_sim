#pragma once

#include "population.h"
#include "parameters.h"

void progression(AgentView person, AllSeries& series, const ProgressionSet& progset,
                 vector<InfectParams>& infectparams, bool dovax,
                 const VaxSet& vaxset);
