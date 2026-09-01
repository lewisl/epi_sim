# Session Notes

## Current State

The atomic-history redesign is implemented and validated.

- `Histories` stores only atomic `(trait, phase, real trait value, ring lane,
  concrete age)` vectors.
- Only `HistoryValue` is a history storage alias. Column vectors use
  `std::vector<HistoryValue>` directly and outer-vector indices use `size_t`;
  the former `HistoryVector` and `HistoryVectorIndex` aliases are removed.
- Physical order remains:

  ```text
  status/now, status/new_, vax/now, vax/new_, variant/now, variant/new_
  ```

- `TraitPhaseLayout`, its six descriptors, and the experimental `test_index`
  calculation are removed.
- The only stored layout widths are the three per-trait phase widths.
- Raw one-based trait, ring, and age values are converted to compact zero-based
  ordinals at the history-vector indexing boundary.
- Trait sentinel value 0, total age, and total ring do not occupy physical
  columns.
- Rings disabled means one implicit whole-population lane. Rings enabled means
  exactly one lane for each real ring.
- Vaccination disabled means zero vaccine-brand columns. Vaccine history is
  brand only; no `Vaxstatus` histories were added.
- `new_unexposed` atomic columns remain as deliberate fixed-stride placeholders.
  They are never written, cannot be selected, and are omitted from `"all"`.

The single index expression is:

```text
trait_phase_base
+ (raw_trait_value - 1) * ring_lane_count * 5
+ ring_ordinal * 5
+ (raw_age - 1)
```

Group bases are derived from the status, vaccine, and variant phase widths;
there is no duplicate stored overlay schema.

## Materialized Totals

Totals are produced post-simulation by `resolve_history_selection`:

- `age == "total"` sums all five concrete ages.
- an empty ring selector sums all real rings, or selects the implicit lane when
  rings are disabled.
- `vaccinated` sums all active real vaccine brands.
- combinations expand to the Cartesian product of their atomic sources.

Each `ResolvedHistoryVector` owns the materialized day vector and records its
`source_history_vectors`. No total vectors are inserted into `Histories` or a
second total-history store.

Printing, CSV serialization, and plotting all consume the same resolved
vectors. Invalid selections are reported and skipped while other valid
selections continue. The status-total printer, variant invariant, and Rt
counting were also moved off physical total slots.

## Initialization And Updates

- One simulation update writes exactly one atomic vector.
- Day-1 `now_unexposed` is seeded by walking persons `1..popn`, so both age and
  ring-specific initial histories are correct.
- `init_history_series(day)` carries only atomic `Phase::now` vectors.
- `Phase::new_` starts each day at zero.

## Introspection

`Histories` now provides:

- `valid_history_coordinates(...)`
- `describe_history_vector(index)`
- `history_column_label(index)`
- `explain_history_vector_index(...)`
- `dump_history_layout(out)`
- `validate_history_layout()`

The constructor runs the exhaustive layout validator. Tests also cover every
combination of 1–3 variants, 0–3 vaccine brands, and 0–3 rings, verifying exact
column counts and forward/reverse index round trips.

## Boundary Decisions

- Variant input must be non-empty and its first variant must be `base`.
  `input_verify.cpp` already enforces both. The empty-input case now has an
  explicit parameters test.
- A valid input simulation therefore has at least one real variant. The
  `Histories` type still permits zero variant columns for isolated unit-test
  fixtures that never update variants.
- With zero vaccine brands, `now_vaccinated` and `new_vaccinated` resolve to
  materialized zero vectors. A named vaccine selector is invalid.
- With one vaccine, one variant, or one ring, no redundant total physical lane
  is created.
- Statuses are never totaled across status values.

## case-1

For `/Users/lewislevin/epi-sim-project/case-1/input/config.json`:

- six real variants;
- `dovax == false`, so zero active vaccine-brand columns even though a vaccine
  parameter file is named in the config;
- `do_rings == false`, so one implicit population ring lane.

The physical count is:

```text
2 * (4 statuses + 0 vaccines + 6 variants) * 1 ring lane * 5 ages = 100
```

The only intentionally unused outer vectors are the five concrete-age
`new_unexposed` placeholders.

## Documentation

- `design/series_hierarchy.md` describes storage, indexing, materialization,
  query behavior, and introspection.
- `design/series_column_order.md` gives the exact group bases, a worked
  two-ring example, and the case-1 ranges.

## Validation

- `xmake build epi_sim`: passed.
- `xmake run test series`: 430 checks passed.
- `xmake run test disease_modeling`: 44 checks passed.
- `xmake run test vaccination`: 97 checks passed.
- `xmake run test plot`: 29 checks passed, including exact embedded Plotly JSON
  for an age-and-ring total.
- `xmake run test parameters`: 172 checks passed, including empty variants.
- `xmake run test`: 1,037 checks passed.
- `xmake run test runsim --artifacts`: 34 checks passed.
- Comprehensive runsim CSV: 180 data rows, 486 selected aggregate/per-ring
  columns, `new_unexposed` absent, `now_vaccinated` present.
- `git diff --check`: passed.

## Working Constraints

- Use xmake only and always name the target.
- `xmake run test` intentionally excludes `runsim`; run it explicitly.
- Preserve compact raw `PopData` trait IDs and direct indexed access in hot
  loops.
- Preserve unrelated worktree changes.
