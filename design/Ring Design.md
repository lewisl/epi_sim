# Ring-Based Contact Structure Spec

## Summary

Specify rings as static community-cluster contact layers for `epi_sim`. `PopData::ring` remains the per-person membership trait, while a separate ring member index provides fast lookup into `PopData` vectors during spread. Ring membership does not change during a simulation run.

Research pass should compare this design against layered contact approaches in Covasim/SynthPops, Starsim networks, POLYMOD/Prem contact matrices, and OpenABM-Covid19.

## Key Changes

- Add a model-owned ring index, outside `PopData`, shaped like `vector<vector<size_t>>`, where each inner vector stores 1-based person indices for one ring.
- Add a `PopData` column for individual out-of-ring contact probability, likely `vector<OutRingProb> out_ring_prob`, defaulting to `1.0f` to preserve current global-mixing behavior when rings are not configured.
- Support both ring assignment paths:
  - generated from parameters: ring count, size distribution, optional age/geography weighting
  - loaded explicit assignment: one ring ID per person, overriding generated assignment
- Use rings only in contact candidate selection:
  - for each contact draw, use the spreader's `out_ring_prob`
  - same-ring draw samples from that spreader's ring member list
  - out-of-ring draw samples from the full population
  - touch probability and infection risk remain separate gates

## Spread Semantics

- Rings affect "who may be contacted," not transmissibility once contact occurs.
- Social distancing still controls contact count and touch factors.
- Receiver traits still control susceptibility through existing infection-risk logic.
- Do not model inbound/outbound asymmetry in v1 beyond the spreader's `out_ring_prob`.
- Avoid full adjacency lists in v1; recurrent per-person networks can be a later feature if research shows they matter.

## Research Anchors

Use these as the first literature/reference set:

- Covasim PLOS paper: multilayer household, school, workplace, community contact networks: https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1009149
- Starsim: dynamic transmission network design: https://starsim.org/
- POLYMOD contact survey: empirical contact mixing by age/location/duration: https://journals.plos.org/plosmedicine/article?id=10.1371/journal.pmed.0050074
- Prem/Cook/Jit projected contact matrices: home/work/school/other contact structure: https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1005697
- OpenABM-Covid19: realistic social networks and contact tracing context: https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1009146

## Test Plan

- Unit test ring assignment preserves 1-based indexing and leaves index 0 unused.
- Unit test ring index construction maps every person `1..popn` into exactly one member list.
- Unit test explicit assignment overrides generated assignment.
- Spread test with `out_ring_prob = 0.0f`: contacts come only from same ring when possible.
- Spread test with `out_ring_prob = 1.0f`: behavior matches current global contact selection.
- Serialization test for the new out-of-ring probability column.

## Assumptions

- Ring membership is static for one simulation run.
- Ring IDs are small numeric trait values, consistent with the new `Ring` wrapper.
- The ring member index belongs to `Model` or another model-level runtime structure, not inside `PopData`.
- v1 should be a conservative contact-selection enhancement, not a full social-network rewrite.
