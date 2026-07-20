# Runtime Trait Names: Model Ownership

## Purpose

Keep the human-readable names of runtime traits with the `Model` that uses
their numeric IDs.  This makes a retained model and its `AllSeries`
interpretable after another case has been loaded, and provides the metadata
needed for a future serialized-model round trip.

This is a small data-ownership change.  It does not add model serialization,
simulation extension, breakpoints, or a new `runsim()` mode.

## Fixed and runtime traits

The wrapper types remain global C++ types.  `PopData` keeps compact numeric
IDs such as `Variant{3}` and `Vax{2}`; it must not store strings per person.

The fixed trait definitions remain code-owned:

- `Agegrp`
- `Status`
- `Condition`
- `Vaxstatus`
- `Progressionmap`

Their allowed names and indexing rules are compile-time program rules.  In
particular, agent status and condition values remain mutable even though the
tables of allowed values are fixed.

The runtime name registries are case/run data:

- `Variant::names`
- `Vax::names`
- `Ring::names`
- `SDCase::names`

Today these are mutable static vectors.  A `Variant`, `Vax`, `Ring`, or
`SDCase` instance stores only its numeric `v` value.  Loading a second case
replaces the shared name vectors, so an older retained model no longer carries
the names needed to interpret its IDs.

## Proposed ownership

Store an ordered copy of each runtime name table in the model data that owns
the matching runtime parameters:

| Runtime trait | Model-owned source of truth |
| --- | --- |
| Variant | `ModelParams::variant_names` |
| Vax | `VaxSet::names` |
| Ring | `RingTraits::names` |
| SDCase | Existing `Model::sd_cases[*].name` |

`SDCase` needs no duplicate table initially: its model-owned
`SocialDistancing` entries already retain their names.  Its active registry can
be reconstructed as `"none"` followed by the names of `model.sd_cases` in
order.

The copied tables are deliberately small and index-aligned with the existing
numeric data.  They preserve the current fast lookup scheme:

```cpp
const std::string& vaccine_name = model.mp.vaxset.names[vax.v];
```

No lookup through a population vector and no string storage in `PopData` is
needed.

## Required invariants

- The loader builds the static registry and the Model-owned table in exactly
  the same order.
- `Variant` index `0` is `"none"`; index `1` is the required `"base"`
  variant.
- `Vax` index `0` is `"none"`, and `vaxset.names.size()` matches
  `vaxset.params.size()`.
- `Ring` index `0` is its unused empty-string sentinel; real ring names begin
  at index `1`.
- `SDCase` index `0` is `"none"`; the remaining entries match the order of
  `model.sd_cases` and the `PopData::sdcase` IDs.

The numeric wrapper constructors such as `Vax{3}` do not register names and
need not be replayed to restore a model.  Once the associated name table is
available, existing numeric IDs are meaningful again.

## Compatibility with current code

Existing simulation code reads the static registries.  The first change can
continue to populate them during normal parameter loading while also saving
the model-owned copies.  Therefore a freshly loaded model behaves exactly as
it does today.

If a later feature must run or inspect a retained/deserialized model after
another case has been loaded, use one explicit activation step:

```cpp
install_runtime_trait_names(model);
```

That step copies the model-owned tables into the current static registries.  It
does not parse input files, rebuild `PopData`, or recreate every trait wrapper;
it only activates the metadata for the one model being used.  The static
registries are then an implementation cache for the active model, not the
source of truth.

## Explicitly deferred

- Serializing and deserializing `Model` or `AllSeries`.
- A `runsim()` partial-load or alternate start mode.
- Retaining more than one completed run simultaneously.
- Extending a simulation, changing runtime definitions mid-run, and resizing
  `AllSeries`.
- Generalizing fixed age groups, statuses, or conditions.

Those features can build on model-owned runtime names, but none is required to
make this ownership change correct.
