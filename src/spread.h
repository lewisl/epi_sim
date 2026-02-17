#pragma once

#include "lib_includes.h"

#include "population.h"
#include "parameters.h"

void spread(PopData pop, size_t p, SocialParams social, InfectSet infect);
// additional parameters later: dovax, vaxset, density_factor, indoor_seq, sd_cases