# Histories Layout And Selection

`Histories` is an in-memory table whose physical columns are ordinary
`std::vector<HistoryValue>` objects. Each physical column is atomic: it
represents one trait, phase, concrete trait value, concrete age group, and
concrete ring lane. Its inner vector contains the count for each simulation
day.

Totals are query results. They are not physical history columns and simulation
updates never maintain them.

## Stored axes

| Axis | Stored values |
|---|---|
| `Trait` | `status`, vaccine brand (`vax`), `variant` |
| `Phase` | `now`, `new_` |
| status value | `unexposed`, `infectious`, `recovered`, `dead` |
| vaccine value | Each active real vaccine brand; never `Vax{0}` |
| variant value | Each real variant; never `Variant{0}` |
| age | The five concrete age groups, raw values `1..5` |
| ring | Each active real ring, or one implicit population lane if rings are disabled |

There is no `Vaxstatus` history. Vaccine histories retain the original brand
semantics:

- `new_vax:<brand>` counts first vaccinations with that brand on that day.
- `now_vax:<brand>` is the carried stock of people ever vaccinated with that
  brand.
- `Vax{0}` remains “unvaccinated” in `PopData`, but it is not a stored history
  value.

`new_unexposed` is the one deliberate schema placeholder. Its atomic columns
exist so status `now` and `new_` have equal widths. Simulation code never
updates them, the selector rejects them, and `HistorySelectionSpec("all")`
does not include them.

## Compact physical indexing

The flat outer-vector order is:

```text
Trait -> Phase -> TraitValue -> Ring -> Age

status/now
status/new_
vax/now
vax/new_
variant/now
variant/new_
```

Let:

```text
A = 5 concrete ages
R = max(number of active real rings, 1)
Cstatus = 4
Cvax = number of active real vaccine brands
Cvariant = number of real variants

S = Cstatus  * R * A
V = Cvax     * R * A
W = Cvariant * R * A
```

`S`, `V`, and `W` are the three phase widths. Group bases are derived directly:

```text
status/now     0
status/new_    S
vax/now        2S
vax/new_       2S + V
variant/now    2(S + V)
variant/new_   2(S + V) + W
```

Raw one-based values are converted to compact ordinals only at the history
index boundary:

```text
value_ordinal = raw_trait_value - 1
age_ordinal   = raw_age - 1
ring_ordinal  = raw_ring - 1              when rings are active
ring_ordinal  = 0                         when rings are disabled
```

The single physical-index expression is:

```text
index = trait_phase_base
      + value_ordinal * R * A
      + ring_ordinal * A
      + age_ordinal
```

There is no `TraitPhaseLayout` descriptor or parallel overlay structure.
`Histories` stores only the three phase widths and the active value/ring
counts needed to evaluate this expression.

Days remain 1-based inside each column vector; inner slot 0 is allocated but
not emitted.

## Updates and initialization

Simulation transitions call:

```cpp
histories.update(trait, phase, raw_trait_value, raw_ring,
                 concrete_age, day, change);
```

One call changes exactly one physical column. There are no mirror writes for
total age or total ring.

The constructor seeds `now_unexposed` day 1 by walking people `1..popn`, so it
seeds the correct atomic age and ring cell. `init_history_series(day)` carries
only every `Phase::now` atomic vector forward. `Phase::new_` vectors start each
day at zero.

## Selection and materialized totals

`HistorySelection` remains the public string query:

```cpp
struct HistorySelection {
    std::string name;
    std::string age;       // "total" or a concrete age name
    std::string ring = ""; // empty means all rings
};
```

Examples:

```text
now_infectious
new_vax:Pfizer
now_variant:delta
now_vaccinated
```

Resolution expands a requested total into its atomic source-column indices,
sums those columns into an owned `std::vector<HistoryValue>`, and records the
source indices in `ResolvedHistoryVector::source_history_vectors`. A resolved
vector is therefore the materialized total used by printing, CSV
serialization, and plotting; it is not inserted back into `Histories`.

The resolver supports:

- total age: sum all five concrete ages;
- total ring: sum every real ring, or use the implicit lane when rings are
  disabled;
- `now_vaccinated` / `new_vaccinated`: sum every active real vaccine brand;
- any combination of the above.

With no active vaccine brands, `vaccinated` is a valid all-zero materialized
vector. A named vaccine brand is invalid because no such physical column
exists. Invalid selections are reported and skipped; valid selections in the
same request continue through printing, serialization, or plotting.

## Introspection

The layout can be inspected in both directions without storing a duplicate
schema:

- `history_vector_index(...)` maps valid raw coordinates to a physical index.
- `describe_history_vector(index)` reconstructs raw coordinates.
- `history_column_label(index)` produces a field-labelled description.
- `explain_history_vector_index(...)` shows the index and each ordinal.
- `dump_history_layout(out)` prints every physical column.
- `validate_history_layout()` exhaustively checks uniqueness, coverage, and
  forward/reverse round trips.

This gives debugging and user-facing inspection a stable vocabulary while the
simulation hot path keeps direct vector indexing.
