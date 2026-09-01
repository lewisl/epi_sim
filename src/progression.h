#pragma once

#include "population.h"
#include "parameters.h"

void progression(AgentView person, Histories& histories, const ProgressionSet& progset,
                 vector<InfectParams>& infectparams, bool dovax,
                 const VaxSet& vaxset);
