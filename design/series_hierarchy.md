# AllSeries Column Table And Selection

`AllSeries` stores simulation history as one private `columns × days` table.
It is separate from `PopData` serialization: population output formats
person-level fields, while series output selects already-collected integer count
columns.

## Logical Blocks

The table contains six typed blocks:

| `SeriesBlock` | Subject axis | Meaning |
|---|---|---|
| `now_status` | `Status` | current status stock |
| `new_status` | `Status` | status-transition flow |
| `now_vax` | `Vax` brand | current vaccinated stock |
| `new_vax` | `Vax` brand | first-vaccination flow |
| `now_variant` | `Variant` | current infectious stock by variant |
| `new_variant` | `Variant` | new infection flow by variant |

Status, vaccine, and variant are parallel classifications. They are not crossed
with each other. Every block is dense across its own subjects, all ring slots,
and all six age buckets, so ring-by-age intersections remain available.

The `now_*` blocks are stocks. `init_history_series(day)` carries every column
in each stock block from `day - 1` to `day`. The `new_*` blocks are flows; their
preallocated day cells remain zero until an event updates them.

## Physical Layout

The storage member is:

```cpp
std::vector<std::vector<std::int32_t>> cols_;
```

Each outer element is one resolved history column. Each inner vector has
`day_cnt + 1` cells. Simulation days are 1-based, so slot 0 is allocated but
not emitted.

`SeriesColumnMap` builds a `SeriesBlockDescriptor` for every block. A descriptor
contains the block's base column, subject count, subject stride, ring stride,
column count, and stock/flow flag. Physical order is:

```text
block -> subject -> ring -> age bucket
```

Age bucket is the fastest-changing coordinate. For a descriptor `d`:

```text
column = d.base_col
       + subject * d.subject_stride
       + ring * d.ring_stride
       + bucket

d.ring_stride    = AgeBucket::COUNT
d.subject_stride = ring_count * AgeBucket::COUNT
d.column_count   = subject_count * d.subject_stride
```

The day selects a value inside that column:

```cpp
series.at(block, subject, bucket, ring)[day]
```

`column_index(...)`, `column(...)`, `at(...)`, and `update(...)` are the public
numeric access layer. Their hot-path operations are inline and unchecked.
Callers must supply valid block, subject, ring, bucket, and day coordinates.

## Index Semantics

| Coordinate | Convention |
|---|---|
| block | Zero-based `SeriesBlock` enum; all six values are physical blocks. |
| subject | Raw `Status`, `Vax`, or `Variant` ID. Slot 0 exists in storage as that family's sentinel but is excluded from normal output selection. |
| ring | `RING_ALL == 0` is the aggregate slot; real rings are `1..N`. |
| age bucket | Zero-based `AgeBucket`; `total == 0`, followed by five concrete age buckets. |
| day | `1..day_cnt`; slot 0 is unused by normal output. |

When no real rings are configured, `AllSeries` still has one ring slot: the
`RING_ALL` aggregate.

## Updates And Aggregates

Simulation transitions call:

```cpp
series.update(block, subject, ring, agegrp, day, change);
```

`Agegrp` is converted to its concrete `AgeBucket`. One event normally updates
four columns:

```text
(subject, specific ring, specific age)
(subject, specific ring, total age)
(subject, RING_ALL, specific age)
(subject, RING_ALL, total age)
```

If the supplied ring is already `RING_ALL`, the aggregate mirror writes are
skipped. This preserves the no-rings behavior without double-counting.

Disease transitions update status and variant blocks. First vaccinations update
vaccine blocks. A vaccination total is not stored: `now_vaccinated` and
`new_vaccinated` are materialized at read time by summing every non-sentinel
vaccine-brand column for the requested ring and age bucket.

The constructor preserves the existing day-1 seed: only aggregate-ring
`now_status[UNEXPOSED]` is populated from `PopData`, for the total and each age
bucket. Per-ring day-1 unexposed stocks remain a follow-up.

`finalize_series()` is intentionally empty. Ring and age totals are maintained
during updates, and vaccinated totals are reductions during selection.

`validate_variant_invariant()` checks every simulated day:

```text
sum(now_variant[real variant, RING_ALL, total])
    == now_status[INFECTIOUS, RING_ALL, total]
```

## Text Selection And Numeric Resolution

Strings are post-run selectors. They do not determine which history columns are
collected.

```cpp
struct SeriesSelection {
    std::string name;
    std::string bucket;
    std::string ring = "";
};
```

`SeriesColSpec` preserves requested order and supports explicit selections or
the `"all"` expansion. The expansion emits every public status, vaccine-brand,
variant, and vaccinated-total selection for the requested age buckets; sentinel
subjects are omitted.

Resolution proceeds as follows:

1. Parse `bucket` to an `AgeBucket`.
2. Parse `ring`: empty means `RING_ALL`; a decimal token is a ring ID; otherwise
   use the registered `Ring::names` value.
3. Split `name` into `now_`/`new_` and a family/subject selector.
4. Use the family registry for status, vaccine, or variant to find the subject
   ID and block.
5. Calculate the outer-column index with `column_index(...)`.
6. Materialize that physical column, or sum vaccine-brand columns for a
   vaccinated-total selection.

Examples:

```text
now_infectious       -> now_status, INFECTIOUS
new_vax:pfizer       -> new_vax, registered Pfizer subject
now_variant:delta    -> now_variant, registered Delta subject
now_vaccinated       -> reduction across non-sentinel now_vax subjects
```

Labels are generated from resolved coordinates. Aggregate-ring labels omit a
ring suffix. Real-ring labels use the registered ring name, so a numeric alias
such as `"2"` selects ring 2 but serializes using `Ring::names[2]`.

Invalid selections are reported separately. Serialization skips invalid
columns while retaining valid selections in caller order; printing rejects a
set containing an invalid selection.

## Output Consumers

`print_selected_series`, `serialize_selected_series`, and `seriesplot` share
`resolve_selected_series`:

```text
SeriesColSpec
  -> typed coordinates and numeric outer-column indices
  -> ResolvedSeriesSelection
       -> terminal table
       -> CSV
       -> Plotly traces
```

Condition history is not part of the current table. Later support adds
`now_condition` and `new_condition` descriptors plus transition updates; it does
not require changing the physical indexing scheme.
