# Series Refinement: Collected Outcomes And Terminology

## Status

Discussion note from August 24, 2026. No implementation changes are authorized
by this note. The next design step is to decide which outcome series should be
collected or derived before refining the selection API or its terminology.

## Where The Implementation Is Now

`AllSeries` contains one `columns × days` table. Only a finite set of useful
outcome series is physically accumulated during a simulation. The current six
column groups are:

```text
now_status, new_status
now_vax, new_vax
now_variant, new_variant
```

Each group contains the applicable trait values crossed with ring and age. The
groups are ranges in one outer column vector, not separate history containers.

The first refactor deliberately began with the history that already existed. It
preserved output behavior and wired plotting, serialization, and printing
through `SeriesColSpec` while replacing the physical `SeriesGroup` hierarchy.

## Why There Is Not A Universal Cross Product

It is logically possible to describe outcome data using orthogonal selectors
such as:

```text
phase × status × vaccine × variant × vaxstatus × age × ring × condition
```

The current history does not collect that Cartesian product. It collects a
small number of projections chosen because they correspond to epidemiological,
clinical, and public-policy outcomes that are useful and reasonable to report.

For example, the current table can answer separate questions about:

```text
status × age × ring
vaccine brand × age × ring
variant × age × ring
```

It cannot answer an arbitrary joint question such as infectious people with a
particular vaccine, variant, vaccination status, age, and ring. Some additional
joint outcomes would have to be accumulated during the simulation. Others
might be derived from collected columns. Determining which is which introduces
real semantic and implementation complexity.

The next design should therefore start with the desired outcome inventory, not
with a universal selector grammar. The eventual set may remain deliberately
smaller than the full cross product.

## Provisional Terminology

The terminology should make sense to people accustomed to epidemic outcome
data and public-policy reporting. The following points are agreed or strongly
preferred, while a few names remain open.

### Trait

Use **trait**, not family.

A trait is a characteristic of an agent that has one value at a time. A trait
has a trait name and a set of allowed values. In code, traits are small classes
or structs whose values may be defined at compile time or loaded from input
parameters. That distinction should not matter to the reporting interface.

Examples include:

```text
status:     unexposed, infectious, recovered, dead
variant:    base, delta, alpha, ...
vaccine:    pfizer, moderna, jnj, ...
vaxstatus:  none, first, full, booster
agegrp:     age0_19, age20_39, age40_59, age60_79, age80_up
ring:       configured ring values
condition:  uninfected, nil, mild, sick, severe
```

### Age

At the selection/reporting interface, use **age** rather than **bucket**.
`bucket` was terminology inherited from the prior grouped storage structure and
has only meant age in this context.

An internal type may still need to represent both concrete age values and a
synthetic total, but that does not require exposing `bucket` in a column
request.

### Subject

The current idea of a **subject** combines:

```text
phase (now/new) + trait + trait value
```

Examples are current infectious status, new deaths, current Pfizer vaccination,
or new Delta infections. These are the quantities physically accumulated as
series during the run.

`subject` is provisional terminology. A clearer word may emerge after the
desired column inventory is defined.

### Column Group / Series Group

The current `SeriesBlock` values are metadata describing contiguous ranges in
the one column table. They correspond closely to what the simulation
accumulates. **Column group** or **series group** may communicate this better
than **block**.

The term remains open; no code rename should happen until the conceptual model
is settled.

### Series And Column

Physically, one series is one outer-table column containing values across all
simulation days. In that sense, **series**, **column**, and **outer-column
index** refer to equivalent things at different levels of description.

## Selection Versus Collection

A `SeriesSelection` should eventually be an easy-to-read expression of a
desired subject, age, and ring. It may also support symbolic expansion or
reduction values.

Important distinctions include:

```text
%all%    expand into all applicable individual values
%total%  produce one aggregate over applicable values
%any%    select the complement of a sentinel such as vaxstatus=none
```

These symbols are not necessarily meaningful for every trait or phase. Their
validity and semantics must be decided from the outcome inventory rather than
assumed uniformly.

`SeriesColSpec` currently serves both as a list of explicit selections and as a
mechanism for expanding shorthand such as `"all"`. That preserved existing
plotting, serialization, and printing usage through the first refactor. It may
be simplified or separated later, after the desired selection language is
clear.

## Collected And Derived Outcomes

The next inventory should classify every requested series as one of:

1. **Physically collected** during transitions in the simulation.
2. **Derived** from one or more collected series without person-level history.
3. **Unavailable without a new cross-classification** or additional history.

Current examples of derived series include aggregate age/ring views and the
sum of vaccinated vaccine-brand columns. Variant totals can reproduce total
infectious counts, although that duplicates the status view.

Vaccination status is present in `PopData` but is not currently collected as an
`AllSeries` column group. Condition history is also excluded from the current
table. Adding either requires an explicit outcome decision and transition
updates, not merely new selector syntax.

## Questions For The Next Discussion

1. Which exact outcome columns are useful enough to collect during every run?
2. Which useful outcomes can be derived from those columns?
3. Which crossings with age should exist?
4. Which crossings with ring should exist, and is ring-by-age useful?
5. Should vaccination status be collected, and what do `new` and `now` mean for
   first, full, and booster states?
6. Which joint trait crossings, if any, justify their collection cost?
7. What terms should replace or retain `subject`, `block`, and `SeriesColSpec`?
8. What should `%all%`, `%total%`, and `%any%` mean for each supported trait?
9. After the inventory is fixed, what is the smallest readable
   `SeriesSelection` representation that expresses it without implying
   unsupported Cartesian products?

Implementation details should remain deferred until these questions are
settled.
