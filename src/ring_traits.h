#pragma once

#include "lib_includes.h"

/*
RingTraits: model-level per-ring characteristics.

Storage is virtual 1-indexed to match Ring ids:
  out_ring_prob[ring_id][agegrp]
    outer: index 0 is unused sentinel; valid ring ids are 1..ring_count()
    inner: length 6 (matches Agegrp); index 0 (UNKNOWN) is 0.0f
*/
struct RingTraits {
  std::vector<std::vector<float>> out_ring_prob;   // index as:  [ring][agegrp], both 1-indexed
  std::vector<float> pct_of_population;            // index as:  [ring], 1-indexed; index 0 = 0.0f

  size_t ring_count() const {
    return out_ring_prob.empty() ? 0 : out_ring_prob.size() - 1;
  }
};
