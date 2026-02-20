#pragma once

#include "lib_includes.h"
#include "population.h"

int how_many_contacts(float gammashape, uint8_t spr_agegrp,
                     uint8_t spr_cond, const array<array<float, 5>, 4>& contactfactors);

void get_contacts(const PopData& pop, float gammashape, uint8_t spr_agegrp,
                  uint8_t spr_cond,
                  const array<array<float, 5>, 4> &contactfactors,
                  vector<size_t> &contacts);

bool istouched(const PopData &pop, size_t contact, const array<array<float, 5>, 6> &touchfactors);

// need to add vaxset, dovax after spreader VaxSet &vaxset, bool dovax,
bool isinfected(const PopData &pop, size_t contact, size_t spreader, vector<InfectParams> &infectparams, int thisday);