#pragma once

#include "lib_includes.h"
#include "population.h"

int how_many_contacts(float gammashape, uint8_t spr_agegrp,
                     uint8_t spr_cond, const array<array<float, 5>, 4>& contactfactors);

vector<size_t> get_contacts(PopData pop, float gammashape, uint8_t spr_agegrp,
                            uint8_t spr_cond, const array<array<float, 5>, 4>& contactfactors);