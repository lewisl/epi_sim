# Ring-Based Contact Structure Spec

## Summary

Rings are static community-cluster contact layers for `epi_sim`. `PopData::ring` is the per-person membership trait. A model-owned ring member index supports fast same-ring lookups during spread. Ring-level characteristics (currently the out-of-ring contact probability by agegrp) live in `RingTraits`, also owned at model level. Ring membership and ring characteristics are fixed for the duration of a simulation run.

Subsequent design passes can compare this approach against layered contact networks in Covasim/SynthPops, Starsim, POLYMOD/Prem contact matrices, and OpenABM-Covid19.

## Components

### Per-person membership column

- `PopData::ring` is a `vector<Ring>` sized `popn+1`, 1-indexed; index 0 stays `Ring{0}` (unused, consistent with other PopData columns).

### `Ring` trait (`src/traits.h`)

- Numeric `uint8_t v`, mirroring Variant/Vax.
- `inline static std::vector<std::string> names` registers ring names at load time. The first registration lazily seeds `names[0] = ""` as the unused sentinel, so user-registered ring ids start at 1.
- `Ring(std::string_view name)` registers a name and assigns the next id. Empty names are rejected.
- `Ring(uint8_t)` preserves cheap numeric construction for column default-fill and lookups.
- `show()` returns the registered name when present; falls back to the integer formatted as a string when the registry is empty or the slot has no name.

### `RingTraits` (`src/ring_traits.h`)

Model-level container for per-ring characteristics. v1 fields:

- `vector<vector<float>> out_ring_prob` — `[ring_id][agegrp]`, both dims virtual 1-indexed. Outer size `nrings + 1`; inner size 6 (`Agegrp::names.size()`). Index 0 on either dim is the unused sentinel.
- `vector<float> pct_of_population` — `[ring_id]`, 1-indexed; used at setup time to apportion agents into rings and retained for inspection/round-trip.
- `ring_count()` returns `out_ring_prob.size() - 1` (or 0 when empty).

A default-constructed `RingTraits` represents the "rings disabled" state: `ring_count() == 0`, `pop.ring` stays all zero, and contact selection falls back to current global mixing.

### Ring member index (`Model::ring_members`)

- `vector<vector<size_t>> ring_members` keyed 1-based by ring id. `ring_members[r]` holds the 1-based person ids in ring `r`; index 0 is the unused sentinel.
- Built once in `setup_sim` via `build_ring_members(pop, ring_count)` after `assign_rings`. Recomputed on every run; not serialized to disk.

## Inputs

Rings are configured by a JSON file (path supplied via `Config.rings`; empty path = rings disabled). Top-level key `"rings"` holds an array; each entry has:

- `name` (optional) — string. When omitted, the loader substitutes `"ring_<n>"` where `n` is the 1-based position.
- `pct_of_population` — float in `[0, 1]`. Values across entries must sum to 1.0 (1e-6 tolerance).
- `out_ring_prob_by_agegrp` — object keyed by agegrp name (`age0_19`, `age20_39`, `age40_59`, `age60_79`, `age80_up`). All five agegrps are required. Each value in `[0, 1]`.

Sample: `sample_parameters/rings.json`.

The loader (`load_ring_traits(fpath)` in `src/parameters.cpp`):

- Clears `Ring::names` before registration (matching `load_variants_data` / `load_vax_data` semantics).
- Validates name uniqueness, pct bounds and sum, agegrp coverage, and agegrp prob bounds.
- Returns a populated `RingTraits`. Missing or empty `"rings"` key returns an empty `RingTraits`.

## Setup-time assignment

`assign_rings(pop, pct_of_population)` in `src/setup.cpp` writes `pop.ring[1..popn]`:

- For each agegrp `g` in 1..5: collect indices, `pop.apportion` that agegrp's count across rings using `pct_of_population[1..]`, `std::shuffle` the indices using `xo::get_gen()`, and slice them into ring buckets. Index 0 is left as `Ring{0}`.
- Effect: each ring's age composition matches the population's age composition within rounding. No spatial or social structure beyond agegrp stratification in v1.
- No-op when `pct_of_population` is empty or sentinel-only.

Called from `setup_sim` immediately after `PopData(popn)`, before `build_ring_members`.

## Spread semantics (deferred — task #7)

Designed but not yet implemented. The intended behavior:

- For each contact draw, use `RingTraits::out_ring_prob[spreader.ring()][spreader.agegrp()]` as the probability of an out-of-ring contact.
- Out-of-ring draw samples from the full population (current global selection).
- Same-ring draw samples from `ring_members[spreader.ring()]`.
- Touch probability and infection risk remain separate gates; rings affect *who may be contacted*, not transmissibility once contact occurs.
- Social distancing still controls contact count and touch factors. Receiver traits still control susceptibility.
- No inbound/outbound asymmetry in v1 beyond the spreader's `out_ring_prob`.
- No full adjacency lists in v1; recurrent per-person networks are deferred until research justifies them.

## Persistence

There is no model-snapshot layer in the codebase. Like `Variant::names` and `Vax::names`, `Ring::names` and `RingTraits` are rebuilt from JSON on every run. `ring_members` is recomputed from `pop.ring` on every run. The serialized `pop.ring` column round-trips through `Ring::show()` and produces the registered name when names are loaded.

For tests that register Ring names, `RingNamesGuard` in `test/test_support.h` saves and restores the registry around a test body (matching `VariantNamesGuard` / `VaxNamesGuard`).

## Test Plan (task #9 — deferred)

- 1-based indexing preserved (`pop.ring[0]` stays `Ring{0}`).
- Every person `1..popn` is in exactly one ring.
- Distribution: per-ring person counts match input `pct_of_population` within rounding.
- Per-ring age mix matches the population age mix within rounding.
- Loader: rejects pct sums far from 1.0, missing agegrp keys, duplicate names, out-of-range values.
- Spread: `out_ring_prob = 0` keeps contacts in-ring when possible; `out_ring_prob = 1` matches current global behavior.
- Ring column renders registered names when `Ring::names` is populated and falls back to integers otherwise.

## Research Anchors

- Covasim PLOS paper — multilayer household/school/workplace/community contact networks: https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1009149
- Starsim — dynamic transmission network design: https://starsim.org/
- POLYMOD contact survey — empirical contact mixing by age/location/duration: https://journals.plos.org/plosmedicine/article?id=10.1371/journal.pmed.0050074
- Prem/Cook/Jit projected contact matrices — home/work/school/other contact structure: https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1005697
- OpenABM-Covid19 — realistic social networks and contact tracing context: https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1009146

## Assumptions

- Ring membership is static for one simulation run.
- Ring ids are small numeric trait values (`uint8_t`); optional names are for display, IO, and registration order.
- `RingTraits` and `ring_members` are owned by the model, not by `PopData`.
- v1 is a conservative contact-selection enhancement, not a full social-network rewrite.
