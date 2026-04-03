#pragma once


#include "lib_includes.h"

#include "population.h"
#include "parameters.h"

// before probvec  InfectParams &infpar, 
void progression(PopData::AgentView person, HistorySeries & series, ProgressionSet &progset,
                 vector<InfectParams> &infectparams,
                 array<float, 6> &probvec, bool dovax, VaxSet &vaxset);

void redistribute_probability(array<float, 6> &probvec, float riskfactor,
                         uint8_t duration);

// void do_progression(PopData &pop, size_t p, const array<float, 6> &probvec);
void do_progression(PopData::AgentView person, HistorySeries & series, const array<float,6> &probvec, size_t today);

float riskfactor(float recoveff, float vaxeff);