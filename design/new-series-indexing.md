# AllSeries column-table refactor

## Status

Implemented in August 2026. This document began as a preliminary proposal; it
now records the final scope and design. Validation results are recorded in
`SESSION.md`.

## Intent

Replace six separately nested `SeriesGroup` members with one uniform history
table. A semantic coordinate `(block, subject, ring, age bucket)` calculates an
outer-column number; the selected inner vector contains the day values.

This is a storage/API refactor, not a change to the collected history space or
numeric results. String specifications remain post-run output selectors and do
not control collection.

## Scope

The table preserves all current intersections:

```text
now/new × status/vaccine/variant × ring × age bucket
```

This notation names six parallel blocks. Status, vaccine, and variant are not
crossed with one another. Within each block, every subject is crossed with every
ring slot and every age bucket, including ring-by-age detail.

The six `SeriesBlock` values are:

```text
now_status, new_status
now_vax, new_vax
now_variant, new_variant
```

Condition history is deferred. It can later be added as two descriptors plus
updates at condition transitions.

## Storage and indexing

`AllSeries` owns private storage:

```cpp
std::vector<std::vector<std::int32_t>> cols_;
```

Every inner vector is sized `day_cnt + 1`; days remain 1-based. Each block has
a `SeriesBlockDescriptor` containing:

- base column
- subject count
- subject stride
- ring stride
- total column count
- stock/flow classification

Physical order is `block → subject → ring → age bucket`, with age bucket
fastest:

```text
column = base
       + subject * (ring_count * bucket_count)
       + ring * bucket_count
       + bucket
```

`RING_ALL == 0`, real rings start at 1, `AgeBucket::total == 0`, and subject IDs
are the raw `Status`, `Vax`, or `Variant` values. Subject slot 0 remains present
as the family's sentinel but is excluded from normal output selection.

The public numeric API is:

```cpp
column_index(block, subject, bucket, ring)
column(column_index)
at(block, subject, bucket, ring)
update(block, subject, ring, agegrp, day, change)
```

The small index/access/update functions are defined inline in `series.h` with
Clang forced inlining. Hot-path access remains unchecked.

## Preserved behavior

### Four-write aggregation

Each event writes its specific ring/age cell, ring/total cell,
aggregate-ring/age cell, and aggregate-ring/total cell. When the input ring is
already `RING_ALL`, the mirror writes are skipped to avoid double-counting.

### Stock and flow days

`now_*` descriptors are stocks. `init_history_series(day)` walks every physical
column in each stock block and copies `day - 1` into `day`.

`new_*` descriptors are flows. Their next-day cells remain at the zero value
created during allocation.

### Day-1 seed

The constructor seeds only aggregate-ring `now_status[UNEXPOSED]` for day 1:
the total receives `pop.popn`, and the concrete age buckets receive
`pop.agegrp_parts`. Per-ring day-1 unexposed stocks are unchanged and remain a
future decision.

### Vaccinated totals

There is no physical all-brands column. `now_vaccinated` and `new_vaccinated`
are read-time reductions over vaccine subjects `1..N`, skipping the `none`
sentinel.

### Finalization and invariant

`finalize_series()` remains empty. Ring/age aggregates are maintained during
updates and vaccinated totals are materialized on demand.

`validate_variant_invariant()` still requires, for each day:

```text
sum(now_variant[real variants, RING_ALL, total])
    == now_status[INFECTIOUS, RING_ALL, total]
```

No dead-monotonicity assertion was added.

## Output resolution

`SeriesColSpec` and its caller interface are unchanged. Resolution uses a small
family registry for status, vaccine, and variant:

1. Parse the requested age bucket and ring.
2. Split the series name into `now_`/`new_` and its family/subject name.
3. Resolve the subject against the family's registered names.
4. Calculate one outer-column index, or a list of vaccine-brand indices for a
   vaccinated-total reduction.
5. Materialize a `ResolvedSeriesCol` shared by printing, CSV serialization, and
   plotting.

Requested order is preserved. Output labels are generated from the resolved
coordinates. Aggregate rings omit a suffix; real rings use their registered
names. Consequently, a numeric ring alias intentionally serializes with the
canonical registered ring name.

## Migrated callers

Disease transitions and vaccination updates use `AllSeries::update` with a
typed `SeriesBlock`. The direct R0-simulation read uses `AllSeries::at`.
Printing, serialization, and plotting consume the shared resolved-column
representation.

`SeriesGroup` and the old public `resolve_series` interface were removed.

## Validation strategy

The deterministic `runsim` fixture enables vaccination and two real rings. Its
artifact serializes every canonical `"all"` selection over all six age buckets
for the aggregate ring and for both real rings.

A pre-refactor copy and post-refactor copy are hashed and compared with `cmp`.
Byte identity is the equivalence requirement. Focused tests cover descriptors,
strides, column indices, four-cell updates, the no-rings case, stock versus flow,
day-1 seeding, sentinel exclusion, vaccinated reductions, numeric-ring label
canonicalization, invalid selections, serialization, and the variant invariant.

History timing is reported for comparison but is not a pass/fail gate.

## Deferred work

- Add `now_condition` and `new_condition` blocks when condition plots are
  specified.
- Reconsider pruning ring/age combinations only after plot requirements are
  understood.
- A future collection schema may avoid unused blocks for some runs; this
  refactor deliberately keeps eager dense collection independent of `do_vax`
  and `do_rings`.
- Arrow or Parquet can be added later as an export path without changing the
  in-memory table.
