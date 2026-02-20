#pragma once


#include "lib_includes.h"

#include "population.h"
#include "parameters.h"


void progression(PopData &pop, size_t p) {
  if (pop.duration[p] == 9) pop.make_well(p);
  else pop.incr_duration(p);
}