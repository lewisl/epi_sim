# AllSeries Column-Table Refactor

> **Superseded:** This plan describes the former eager-aggregate table. See
> `series_hierarchy.md` and `series_column_order.md` for the current
> atomic-only `Histories` implementation.

## Summary

Replace the six `SeriesGroup` hierarchies with one `columns × days` table while preserving the complete current history space:

- `now/new × status/vaccine/variant × ring × age bucket`
- Full ring-by-age intersections remain available.
- Day vectors remain 1-based.
- String specifications remain post-run output selectors; they do not control collection.
- Condition history is deferred.

## Data Model and Interfaces

- Add six typed `SeriesBlock` identifiers and block descriptors containing base column, subject count, strides, column count, and stock/flow classification.
- Use physical order `block → subject → ring → age bucket`, with age bucket fastest:
  `base + subject * (ring_count * bucket_count) + ring * bucket_count + bucket`.
- Store private `std::vector<std::vector<std::int32_t>>` data, with every inner vector sized `day_cnt + 1`.
- Replace direct members such as `series.now_status` with:
  - `column_index(block, subject, bucket, ring)`
  - `column(column_index)`
  - `at(block, subject, bucket, ring)`
  - `update(block, subject, ring, agegrp, day, change)`
- Define the small index/access/update functions inline in the header with Clang forced-inlining; keep hot-path access unchecked.
- Keep the `AllSeries` constructor signature and `SeriesColSpec` caller interface unchanged. Remove `SeriesGroup` and replace the old `resolve_series` API with numeric-column resolution.

## Implementation Changes

- Preserve the existing four-write update rule: specific ring/age, ring/total, aggregate-ring/age, and aggregate-ring/total, including the `RING_ALL` double-count guard.
- Carry every column belonging to a stock block forward; leave flow blocks zero-initialized per day.
- Preserve aggregate-only day-1 `UNEXPOSED` seeding, read-time vaccinated-brand summation, the empty finalization pass, and the variant invariant.
- Resolve selections through a small family registry for status, vaccine, and variant. Normal selections resolve to one outer-column index; vaccinated totals resolve to the relevant brand-column indices and are summed when materialized.
- Canonicalize output labels from resolved coordinates. Real rings use their registered names; aggregate ring labels omit a ring suffix. Requested column order remains unchanged.
- Convert disease transitions, vaccination updates, tests, and the direct `r0_simulation` read to the block-based API. Plotting and serialization continue consuming the shared resolved-column representation.
- Do not add condition blocks, collection-time schemas, conditional `do_vax`/`do_rings` storage, dead-monotonicity assertions, or ring/age pruning in this refactor.

## Equivalence and Test Plan

- Before production changes, extend the deterministic `runsim` fixture to enable vaccination and two rings, retain its returned `AllSeries`, and serialize:
  - every canonical `"all"` selection across all six age buckets;
  - the same selections for every registered real ring.
- Run `xmake run test runsim --artifacts`, preserve the comprehensive CSV in an isolated temporary baseline directory, record its SHA-256 and reported history timing.
- Add focused tests for block bases/strides, numeric column indices, four-cell updates, no-rings behavior, stock carry versus flow reset, day-1 seeding, sentinel exclusion, vaccinated reductions, canonical numeric-ring aliases, invalid selections, serialization, and variant invariants.
- Run:
  - `xmake run test series`
  - `xmake run test disease_modeling`
  - `xmake run test vaccination`
  - `xmake run test plot`
  - `xmake build epi_sim`
  - `xmake run test`
  - `xmake run test runsim --artifacts`
- Preserve the post-refactor comprehensive CSV separately and require byte-identical `cmp` against the baseline. Use `diff` to diagnose any mismatch. Timing is reported but is not a pass/fail gate.
- The equivalence file uses canonical ring names, allowing byte-identical headers. Separately test the intentional behavior change that numeric ring aliases now serialize with registered names.

## Documentation and Assumptions

- Update the series hierarchy, indexing inventory, architecture documentation, and the saved preliminary design to describe the flat column map; mark the legacy `HistorySeries` description as superseded.
- Update `SESSION.md` with the final layout, baseline hashes, validation results, and remaining follow-ups.
- Later condition support adds two descriptors plus transition updates. Later storage pruning can introduce a collection schema after plot requirements are understood.
- No UI assistance is expected for the equivalence proof because the automated fixed-seed fixture provides broader coverage.
- Preserve unrelated worktree changes.
