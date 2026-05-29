# Session: ring axis for series subsystem

Branch: `ring_by_ai`. Plan: `design/ring_series_plan.md`. Follow-up plan:
`design/cli_ring_selection_prompt.md` (selection-layer ring API,
implemented; see "Selection-layer ring API" section below).

## Status
All 7 plan steps complete. Build + tests green (was 294/294, now 302/302
after adding selection-layer ring tests). No-rings CSV byte-identical to
pre-change baseline. 3-ring sim passes `validate_variant_invariant` for
all 180 days. Selection-layer ring API is now wired through
`serialize_selected_series`, `print_selected_series`, and `seriesplot`.

## What changed
- `src/series.h` / `src/series.cpp`
  - `RING_ALL = 0` constant; ring slot 0 reserved for the all-rings aggregate
  - `SeriesGroup` storage: `subjects[subject][ring][bucket][day]`
  - `SeriesGroup::update(subject, ring, agegrp, day, change)` — four-write
    mirror (per-ring + RING_ALL) with `if (ring != RING_ALL)` guard so the
    no-rings case writes once
  - `SeriesGroup::at(..., bucket, ring = RING_ALL)` — defaulted ring arg
    keeps every existing call site reading the aggregate
  - `AllSeries(day_cnt, pop, n_variants, n_vax, n_rings)` — added n_rings
  - Day-1 UNEXPOSED seeding lands in RING_ALL only (per-ring seeding is a
    documented follow-up — needs `assign_rings` results threaded in)
  - `init_history_series` carry-forward refactored to lambda nesting a
    ring loop
  - `resolve_series(..., ring = RING_ALL)` threads ring into every `.at()`
  - `parse_ring_suffix("name@ring:<name|idx>")` returns
    `{base_name, ring_id}`; nullopt when suffix present but unresolvable
  - `<charconv>` added for `std::from_chars`
- `src/sim.cpp` — `n_ring_slots = max(Ring::names.size(), 1)` at the
  `AllSeries` call site
- `src/disease_modeling.cpp` — `make_sick` / `make_well` / `make_dead`
  grab `auto r = ring().v;` and pass it as the new ring arg (12 sites)
- `src/vaccination.cpp` — 2 sites pass `agent.ring().v`
- `src/plot.cpp` — `seriesplot` calls `parse_ring_suffix` before
  `resolve_series` (mirrors print/serialize)
- `test/test_series.cpp` — `make_series` ctor arity bump; 5 new tests:
  no-rings identity, aggregate-equals-sum-of-rings, no-double-count,
  `resolve_series` ring arg, `parse_ring_suffix`
- `test/test_disease_modeling.cpp` / `test/test_vaccination.cpp` —
  `make_series` ctor arity bump (caught during validation gate, not in
  the original plan)

## Key decisions
- Selection-name parse option (a) from the plan: `name@ring:<X>` suffix
  rather than extending `SeriesSelection` struct. Lowest churn — every
  existing `SeriesColSpec` call site compiles untouched.
- Labels left as `name:bucket`; when name carries `@ring:Jail` the
  emitted label naturally becomes `now_infectious@ring:Jail:total` and
  disambiguates without extra formatting code.
- `at()` ring arg is defaulted (`= RING_ALL`); `update()` ring arg is
  required. Read paths stay unchanged where they don't care; the hot
  loop is forced to pass the ring explicitly.
- `validate_variant_invariant` is unchanged — it reads
  `at(idx, AgeBucket::total)` which now resolves to RING_ALL via the
  default, exactly what it was checking before.

## Known follow-ups (called out in plan, not blockers)
- Per-ring day-1 UNEXPOSED seeding: currently only RING_ALL is seeded.
  Real per-ring initial stocks need ring assignment results threaded
  into the `AllSeries` ctor.
- Apportionment stragglers: persons at ring 0 from `assign_rings`
  rounding will contribute to RING_ALL only, slight under-count in any
  real ring's column.
- `SeriesColSpec("all", ...)` sentinel still emits RING_ALL columns
  only; a sentinel variant to expand across rings is possible later.

## Validation evidence
- `xmake build test` + `xmake build epi_sim` clean
- `xmake run test` → 294 checks, 0 failed
- `epi_sim` with `config.json` + `seed_basic.json` → series CSV is
  byte-identical to a pre-change baseline (`test_series_05_28_2026_15_49_00.csv`,
  21697 bytes)
- `epi_sim` with `ring_experiment/seed_rings.json` + `rings_highout.json`
  (3 rings) → `validate_variant_invariant OK` for all 180 days

## Selection-layer ring API (follow-up, complete)
- `src/series.h`
  - `SeriesSelection` promoted from `pair<string,string>` to
    `struct { name, bucket, ring="" }` with defaulted `operator==`. Existing
    two-field aggregate literals still compile (ring defaults to "").
  - Declared free `ring_id_from_token(tok)` — "" → RING_ALL, decimal →
    literal id, name → `Ring::names` lookup, nullopt if unknown.
  - Doc comment on `SeriesColSpec` shows a ring-qualified row example.
- `src/series.cpp`
  - Added `ring_id_from_token` definition.
  - `print_selected_series` and `serialize_selected_series` loops rewritten
    to read `sel.name/bucket/ring` and build labels conditionally
    (`name:bucket` for bare, `name:bucket:ring` for ring-qualified).
  - Both functions no longer call `parse_ring_suffix`.
- `src/plot.cpp` — `seriesplot` loop rewritten the same way.
- `test/test_series.cpp`
  - `.first`/`.second` access on `SeriesSelection` updated to `.name`/
    `.bucket` (line 265-266).
  - Added `test_ring_qualified_selection_resolves_to_ring` — exercises
    `ring_id_from_token` + drives `serialize_selected_series` with a
    ring-qualified `SeriesSelection` literal and verifies CSV header and
    values.
  - Added `test_bare_selection_resolves_to_aggregate` — confirms bare
    selection writes the RING_ALL column (header has no ring suffix).
- Not touched: `sim.cpp` call sites, `build_for_buckets`, the sentinel
  `SeriesColSpec` constructors, the accumulation layer
  (`SeriesGroup::update`, hot-loop call sites), `parse_ring_suffix`
  (left as dead code — declaration, definition, and test still present).

## Validation evidence (follow-up)
- `xmake build test` + `xmake build epi_sim` clean
- `xmake run test` → 302 checks, 0 failed
- `epi_sim` with `config.json` + `seed_basic.json` →
  `test_series_05_28_2026_19_01_55.csv` byte-identical to
  `test_series_05_28_2026_15_49_00.csv` baseline (21697 bytes both)
- `epi_sim` with `sample_parameters/config_rings.json` +
  `sample_parameters/ring_experiment/seed_rings.json` (3 rings) runs
  clean end-to-end

## Next steps
- Open a PR if the user wants
- Resume notes file `design/RESUME_NOTES_ring_work.md` and the plan file
  `design/ring_series_plan.md` are still present — delete or keep per
  user preference
