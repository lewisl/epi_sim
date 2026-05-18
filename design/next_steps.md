# Ring work — resume notes

Last touched: 2026-05-18. Branch `ring_by_ai`, head commit `d975adc`
("Add ring contact-structure foundations"). All tests passing (270/270).

## What's done

Foundations for ring-based contact structure are in place. The simulation
runs unchanged when no rings file is supplied; rings are opt-in via
`Config.rings`.

- **JSON schema and sample**: `sample_parameters/rings.json`.
- **`Ring` trait** (`src/traits.h:211-238`): named-registry shape with
  optional names, defaulting to `"ring_<n>"` at the loader. Index 0 is the
  unused sentinel. `show()` returns the registered name when present;
  numeric fallback otherwise.
- **`RingTraits` struct** (`src/ring_traits.h`): holds
  `out_ring_prob[ring][agegrp]` and `pct_of_population[ring]`, both
  virtual 1-indexed. Field added to `ModelParams`
  (`src/parameters.h:515`).
- **Loader** (`src/parameters.cpp` `load_ring_traits`): validates pct sum,
  agegrp coverage, name uniqueness, bounds. Declared in
  `src/parameters.h:550`.
- **Stratified assignment** (`src/setup.cpp` `assign_rings`): per-agegrp
  apportionment + `std::shuffle` using the project RNG (`xo::get_gen`).
  Called from `setup_sim` after `PopData(popn)`.
- **Ring member index** (`src/setup.cpp` `build_ring_members`):
  `Model::ring_members[ring][i] -> 1-based person id`. Built once after
  `assign_rings`. Recomputed every run; not persisted.
- **Config**: `Config.rings` path field (`src/sim.h:27`); empty default
  means rings disabled. Threaded through `setup_model_params`.
- **Tests harness**: `RingNamesGuard` in `test/test_support.h:39-42`
  matching `VariantNamesGuard` / `VaxNamesGuard` pattern.
- **Design doc**: `design/Ring Design.md` rewritten to match implemented
  spec.

## What's left

Two tasks were deferred during the foundations push.

### Task 7 — Wire rings into spread contact selection

Goal: in `src/spread.cpp`, for each contact draw, use the spreader's
out-of-ring probability to choose between same-ring and full-population
sampling. Rings affect *who may be contacted*, not transmissibility.

Sketch:

- For spreader `s` with `r = pop.ring[s]` and `g = pop.agegrp[s]`:
  - If `model.ring_members` is empty (rings disabled) or `r == 0`, fall
    back to current global contact selection unchanged.
  - Otherwise, `p_out = model.mp.ringtraits.out_ring_prob[r][g]`.
    With `bernoulli(p_out)` (from `random.h`), draw an out-of-ring
    contact (current global path); else sample uniformly from
    `model.ring_members[r]` (excluding `s` itself).
- Hot path: avoid allocations. The `bernoulli` and indexing calls are
  cheap; the same-ring sample is one uniform-int draw plus a vector
  lookup.

Spread sits on the hot path (see `CLAUDE.md` "Hot Paths"). Do not add
helper calls or bounds checks inside loops unless justified by a
measured bug. Mirror the existing inline style in `spread.cpp`.

Verification approach (before task 9 is built out):

- Run the suite end-to-end with `Config.rings` empty and confirm
  series outputs match prior runs (no behavioral drift when rings are
  disabled — this is the critical regression bar).
- Hand-check infection patterns with a small population (~1000) and a
  rings file where `out_ring_prob = 0` everywhere: infections should
  stay within their seeded ring.

Open question: should the same-ring sample exclude the spreader, or is
self-contact already filtered downstream? Read `spread.cpp` end-to-end
before implementing.

### Task 9 — Tests for ring assignment and spread behavior

Belongs in a new `test/test_rings.cpp` (or fold into
`test/test_setup.cpp`). Register a new group in `test/test_main.cpp`'s
`groups` vector if a new file is used.

Cases to cover:

1. **Assignment basics**
   - `pop.ring[0] == Ring{0}` (index 0 untouched).
   - Every person `1..popn` has a non-zero ring id in `1..nrings`.
   - `ring_members[r].size()` summed across rings equals `popn`; no
     person id appears twice.

2. **Distribution vs. input pcts** (user explicitly asked for this)
   - Build a population of, say, 10_000 with rings `pct_of_population =
     [0.4, 0.6]`. Assert per-ring counts are within rounding tolerance
     of `popn * pct` (e.g., ±5 or `±popn * 0.01` — pick a margin that
     accounts for stratification rounding across 5 agegrps).
   - Run with three rings of unequal sizes and verify the same.

3. **Age mix per ring**
   - For each ring, the fraction in each agegrp should match the
     population's fraction within rounding. Bound at e.g. ±1% absolute
     for a 10_000-person test.

4. **Loader validation**
   - Pct sum ≠ 1.0 throws.
   - Missing agegrp in `out_ring_prob_by_agegrp` throws.
   - Duplicate ring names throw.
   - Out-of-range probabilities throw.
   - Missing `"rings"` key returns empty `RingTraits` (no throw).

5. **`Ring::show()` round-trip**
   - With `Ring::names` populated, `pop.ring[i].show()` returns the
     registered name.
   - With registry empty, `show()` returns the integer.
   - Use `RingNamesGuard` (`test/test_support.h:39`) to isolate.

6. **Spread behavior** (after task 7)
   - `out_ring_prob = 0` everywhere with a seed in one ring: all
     infections stay in that ring across N days.
   - `out_ring_prob = 1` everywhere: spread series matches the
     no-rings baseline (within RNG noise — seed both runs).

Use `xo::seed(...)` from `src/random.h` for deterministic test runs.

## Open design questions

- **`pop.apportion` assertion**: `apportion` asserts `parts.back() > 0`.
  With small per-agegrp counts and a small last-ring pct, this could
  trip. Not a concern for the sample rings; revisit if test populations
  shrink or ring configs grow long.
- **Self-contact in same-ring sample**: needs decision when implementing
  task 7.
- **Multi-locale support**: rings are currently single-locale (one
  `rings.json`, one population). Multi-locale models would need either
  per-locale ring config or a way to scope ring ids — out of scope for
  v1.

## Pointers

- Spec: `design/Ring Design.md`
- Sample input: `sample_parameters/rings.json`
- Trait: `src/traits.h:211-238`
- Container: `src/ring_traits.h`
- Loader: `src/parameters.cpp` (search for `load_ring_traits`)
- Setup wiring: `src/setup.cpp` (search for `assign_rings`,
  `build_ring_members`)
- Model fields: `src/setup.h` (`Model::ring_members`,
  `ModelParams::ringtraits`)
- Test guard: `test/test_support.h:39`
- Branch: `ring_by_ai`, head `d975adc`

## Project conventions to keep in mind

- xmake only; never cmake/make. Build file `xmake.lua`.
- Don't use git worktrees (vcpkg/clang paths are not reconstructable).
- PopData vectors are 1-indexed; index 0 is unused.
- Read `AGENTS.md`, `CONTRIBUTING.md`, `design/data-structures.md`,
  `design/code_organization.md` at session start (per `CLAUDE.md`).
- Spread is a hot path — minimize allocations and helper calls.
