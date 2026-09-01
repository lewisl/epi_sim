I agree we should stop patching the current formula. The clean restart is to separate three concepts that are currently conflated:

```text
simulation values ──> physical atomic history columns
                              │
history queries ──────────────┴──> derived totals / CSV / plots
```

C++’s lack of reflection makes this harder, but the deeper problem is that raw enum values, aggregate selectors, physical positions, and index arithmetic currently share the same integers.

One correction: `test_index()`’s trait loop finds the beginning of the requested trait group, but it does not pick up `trait_value`. The parameter is never referenced. You are correct that one trait value represents a slab of columns—not one column—but the index still needs to advance by:

```text
value_position × ring_count × age_count
```

## Recommended first design decision

Store only atomic histories. Derive all totals when histories are selected, serialized, or plotted.

That means:

| Dimension | Physical histories | Aggregate handling |
|---|---|---|
| Status | Four real statuses | No status total |
| Age | Five concrete age groups | Total derived by summing ages |
| Variant | Real variants only | Total derived by summing variants |
| Vaccine | Real vaccine brands only | Vaccinated total derived by summing brands |
| Ring | Real rings, or one implicit lane when rings are disabled | Total derived by summing rings |
| Phase | `now` and `new_` | Both physical |

This removes every physical “total” column and therefore all conditional total-column rules.

Boundary behavior becomes natural:

- One variant: variant total resolves to that one variant.
- Multiple variants: variant total resolves to all variant columns.
- No vaccines: vaccinated total resolves to an all-zero derived history; per-brand queries are invalid.
- One vaccine: vaccinated total resolves to that vaccine.
- No rings: histories use one implicit unpartitioned lane.
- One ring: total-ring selection resolves to that ring.
- Multiple rings: total-ring selection resolves to all rings.
- Total age always resolves to the five concrete age columns.

The important judgment is that aggregate selection does not imply aggregate storage.

## Zero handling

I recommend not assigning a universal meaning to numeric zero.

Raw simulation values retain their existing meanings:

- `Agegrp{0}` = unknown
- `Variant{0}` = none/error during simulation
- `Vaxstatus{0}` = unvaccinated
- ring `0` may continue to mean no assigned real ring

History queries should represent totals with a separate type or tag, such as:

```cpp
enum class Aggregate { total };
```

Thus “total variant” is not `Variant{0}`. It is a query asking for all real variants.

Physical positions are separate again:

```text
raw status 1  -> stored position 0
raw variant 1 -> stored position 0
raw vaccine 1 -> stored position 0
raw age 1     -> stored position 0
raw ring 1    -> stored position 0
```

Those stored positions are private indexing details, not enum values.

## Proposed atomic column order

Keep the understandable existing order:

```text
Trait
  Phase
    Real trait value
      Physical ring lane
        Concrete age
```

Each trait value occupies a slab:

```text
ring_lane_count × concrete_age_count
```

With:

```cpp
age_count = 5;
ring_lane_count = std::max(real_ring_count, size_t{1});

status_value_count = 4;
vax_value_count = real_vaccine_count;
variant_value_count = real_variant_count;
```

The grouping constants are:

```text
value_stride       = ring_lane_count × age_count
status_phase_width = 4 × value_stride
vax_phase_width    = vaccine_count × value_stride
variant_phase_width= variant_count × value_stride
```

The index expression is then:

```text
trait_base
+ phase × trait_phase_width
+ value_position × value_stride
+ ring_position × age_count
+ age_position
```

For case-1:

```text
variants = 6
vaccines = 0
real rings = 0
physical ring lanes = 1
ages = 5
statuses = 4
```

Therefore:

```text
status/now      0–19
status/new     20–39
vax/now        empty
vax/new        empty
variant/now    40–69
variant/new    70–99

total physical histories = 100
```

There are no unused sentinel or aggregate histories.

## Introspection must come before the optimized index

The new layout component should initially contain no simulation logic and no history data. It should enumerate physical column keys:

```cpp
struct HistoryColumnKey {
    Trait trait;
    Phase phase;
    uint8_t raw_trait_value;
    std::optional<uint8_t> raw_ring;
    Agegrp age;
};
```

As each key is enumerated, its vector position is its definitive index. From that one enumeration, build:

- `describe(index)` → exact column coordinates and names
- `index_of_atomic(key)` → exact index
- `dump_schema()` → every physical index and its meaning
- `explain_query()` → physical indices contributing to a total
- `validate_schema()` → uniqueness, completeness, and round-trip checks

Example diagnostic output should look like:

```text
[74] variant/new/delta/no-rings/age60_79

query: variant/new/total/total-ring/total-age
sources:
  [70] variant/new/base/no-rings/age0_19
  ...
  [99] variant/new/omicron_ba4_5/no-rings/age80_up
```

This metadata can exist permanently. It costs nothing in the simulation hot loop unless explicitly printed.

## Avoid maps in the hot loop

String names and maps are only needed when resolving output queries after the simulation.

For simulation updates, use either:

1. The closed-form expression above, or
2. A dense precomputed array from raw coordinates to physical index.

A dense array is not a map. It is direct indexed access and should be as fast as the current arithmetic. It also makes out-of-range and absent-dimension cases explicit during construction.

I would build both initially:

- Enumeration is the reference implementation.
- Closed-form or dense lookup is the production implementation.
- Exhaustive tests require both to return the same index.
- Once proven, only the fast path runs during simulation.

## Explicit judgment decisions to settle first

Before writing the replacement, we should approve these individually:

1. **Physical aggregates**

   Recommended: no physical age, variant, vaccine, or ring totals.

2. **No-vaccine aggregate query**

   Recommended: `now_vaccinated` and `new_vaccinated` return derived zero histories when no vaccines are configured; named vaccine queries are invalid.

3. **No-rings representation**

   Recommended: one implicit physical lane, meaning “ring dimension disabled,” not a total-ring column.

4. **Status/phase combinations**

   Decide whether to store `new_unexposed`. It is currently structurally valid but normally always zero. For the first implementation, I recommend retaining all four statuses in both phases for symmetry, then pruning only after semantics are explicitly agreed.

5. **Variant count zero**

   Recommended: reject the configuration. A valid simulation requires at least the base variant.

6. **Aggregate materialization**

   Recommended: totals are produced by the output resolver and are never written during hot-loop updates.

7. **Physical ordering**

   Recommended: preserve `Trait → Phase → Value → Ring → Age`.

## Step-by-step build process

1. **Write the semantic contract**

   Document raw values, physical values, and query aggregates separately for every dimension. No formulas yet.

2. **Approve the seven decisions above**

   Especially atomic-only storage and the behavior of zero-vaccine queries.

3. **Write hand-checked golden layouts**

   At minimum:

   - 1 variant, 0 vaccines, 0 rings
   - 2 variants, 1 vaccine, 1 ring
   - 3 variants, 2 vaccines, 2 rings

   List every physical index for a reduced test age set or representative boundaries.

4. **Implement only the layout enumerator**

   It creates `HistoryColumnKey` entries and introspection output. It does not allocate day vectors or participate in simulation.

5. **Implement the independent index calculation**

   Add the closed-form expression or dense lookup. Compare every result with enumeration.

6. **Test all cardinality boundaries**

   Exhaustively test:

   ```text
   variants: 0, 1, 2, 3
   vaccines: 0, 1, 2, 3
   rings:    0, 1, 2, 3
   ```

   That is 48 valid combinations plus 16 variant-zero rejection cases.

7. **For every configuration, verify**

   - Calculated count equals enumerated count.
   - Every index is in range.
   - Every physical key is unique.
   - Every legal atomic key resolves.
   - `describe(index_of(key)) == key`.
   - Indices are contiguous with no gaps.
   - Aggregate queries return exactly the expected source indices.
   - Zero/one/many aggregate behavior matches the approved decisions.

8. **Attach day-vector storage**

   Allocate exactly one `std::vector<HistoryValue>` per enumerated atomic key. Keep bounds-checked access during this stage.

9. **Integrate one trait at a time**

   - Status only: initialization, updates, and carry-forward.
   - Variant updates.
   - Vaccine updates.
   - Ring partitioning.
   - Aggregate query resolution.
   - Serialization and plotting.

10. **Enable the unchecked hot path only after equivalence**

    Once all schema, lookup, update, and selection tests pass, switch production updates to direct unchecked indexing while retaining checked diagnostic access for tests.

11. **Run end-to-end equivalence**

    Existing selected CSVs should remain byte-identical because derived totals should equal the currently maintained totals. Any intentional difference must be explained before acceptance.

This approach turns the in-memory database analogy into an advantage: one explicit physical schema, one query layer, and one tested translation between them. There is no descriptor overlay claiming what the vector contains—the enumerated column metadata says exactly what it contains.

No files changed.
