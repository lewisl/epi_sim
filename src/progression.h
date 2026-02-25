#pragma once


#include "lib_includes.h"

#include "population.h"
#include "parameters.h"

// before probvec  InfectParams &infpar, 
void progression(PopData &pop, size_t p, ProgressionSet &progset,
                 vector<float> probvec, bool dovax=false, VaxSet vaxset={});

void redistribute_probability(vector<float> &probvec, float riskfactor,
                         uint8_t duration);

void do_progression(PopData &pop, size_t p, const vector<float> &probvec);

float riskfactor(float recoveff, float vaxeff);