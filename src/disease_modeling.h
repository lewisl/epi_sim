#pragma once

#include "lib_includes.h"
#include "population.h"

struct InfectRiskComponents {
  uint8_t spr_variant{};
  float sendrisk{};
  float recvrisk{};
  float recovfactor{};
  float vaxfactor{};
  float risk{};
};

float contact_factor(const array<array<float, 5>, 4>& contactfactors,
                     uint8_t spr_agegrp, uint8_t spr_cond);
float contact_scale(float density_factor, float indoor_factor,
                    uint8_t spr_agegrp, uint8_t spr_cond,
                    const array<array<float, 5>, 4>& contactfactors);
int how_many_contacts(float density_factor, float indoor_factor, float gammashape, uint8_t spr_agegrp,
                     uint8_t spr_cond, const array<array<float, 5>, 4>& contactfactors);

void get_contacts(const PopData& pop, float density_factor, float indoor_factor, float gammashape, uint8_t spr_agegrp,
                  uint8_t spr_cond,
                  const array<array<float, 5>, 4> &contactfactors,
                  vector<size_t> &contacts);

uint8_t touch_map(Status target_status, Condition target_cond);

float touch_probability(const PopData &pop, size_t contact,
                        const array<array<float, 5>, 6> &touchfactors,
                        float indoor_factor);
bool istouched(const PopData &pop, size_t contact, const array<array<float, 5>, 6> &touchfactors, float indoor_factor);

float infectrisk(vector<InfectParams> &infectparams, uint8_t spr_variant,
                 uint8_t spr_duration, uint8_t contact_agegrp, float recovfactor= 1.0, float vaxfactor= 1.0);

InfectRiskComponents infectrisk_components(const PopData &pop, size_t contact, size_t spreader,
                                           vector<InfectParams> &infectparams, int thisday);
// need to add vaxset, dovax after spreader VaxSet &vaxset, bool dovax,
bool isinfected(const PopData &pop, size_t contact, size_t spreader, vector<InfectParams> &infectparams, int thisday);

float recoveffect(const PopData &pop, size_t thisday, size_t contact,
                  uint8_t spr_variant, vector<InfectParams> &infectparams,
                  float csig = 6.0, float decay_lower = 0.15);

// decay and rise functions for vaccination effectiveness and partial immunity after recovery

float effect_rise(size_t days_since, float mineff = 0.65, float delay_days = 14);

float lindecay(size_t t, float h, float lower);

float expdecay(size_t t, float h);

float sigdecay(size_t t, float h, float csig = 5.0, float decay_lower = 0.1);

float tbrk(float h, float lower);

float intercept(size_t t, float hl);
