#pragma once

#include "lib_includes.h"

#include "population.h"
#include "parameters.h"
#include "cases.h"
#include "sim.h"
#include "series.h"

int spread(PopData &pop, AllSeries & series, AgentView person, SocialParams &social,
            vector<InfectParams> &infectparams, const VaxSet& vaxset, bool dovax,
            vector<size_t> &contacts, float density_factor, vector<float> &indoor_seq,
            const vector<SocialDistancing>& sd_cases,
            const RingTraits& ringtraits,
            const std::vector<std::vector<size_t>>& ring_members,
            const std::vector<size_t>& ring_lengths);
