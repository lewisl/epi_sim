You’re basically right. In the current design, [src/series.h](/Users/lewislevin/code/epi_sim/src/series.h) already tells us the real model:

```cpp
data[name][bucket][day]
```

So `SeriesKey` is not carrying its weight as a public concept anymore. It was useful when the API was “one token names a full column,” but once we commit to explicit 2-level access, the real API should be `(name, bucket)`, and the text layer should map to those two things separately.

The one nuance is this: a tiny “pair object” can still be useful internally when you want to carry around a selected series reference. But that does not need to be the API, and it definitely does not need the current `m x n` text map.

**Phase 1**
Simplify the current status-series API without changing the physical storage.

1. Keep the storage model exactly as it is: `cols[name][bucket][day]`.
2. Keep `DayData::at(name, bucket)` as the main access point.
3. Remove `SeriesKey` from the public access/update path.
4. Replace the current string-to-`SeriesKey` map with two smaller maps:
   - `string -> SeriesName`
   - `string -> AgeBucket`
5. Rename `SeriesBucket` to `AgeBucket`.
6. Normalize text names to lowercase snake_case everywhere.
   - `age0_19`, `age20_39`, `age40_59`, `age60_79`, `age80_up`, `total`
7. Make [src/parameters.h](/Users/lewislevin/code/epi_sim/src/parameters.h) consistent with that naming so `Agegrp` and `AgeBucket` speak the same language.
8. Change selection/printing APIs so callers pass two strings or two typed values, not a precombined synthetic column name.

That gives you a much simpler rule:
- physical access: `series.at(name, bucket)[day]`
- text parsing: parse `name` and `bucket` separately
- no giant literal map of every combination

**Longer View**
I would not generalize by making one giant mega-enum. I would generalize by repeating the same 2-level pattern for each group.

1. Status, condition: fixed compile-time item sets.
2. Vaccine, variant: runtime-defined item sets.
3. Every group keeps the same shape: `[item][age_bucket][day]`.
4. Do not track full cross-products of traits.
5. Build specialized plots by filtering/deriving from these base series, as you already did in Julia.

So the likely destination is something structurally like:

- `status.stock`
- `status.flow`
- `condition.stock`
- `condition.flow`
- `vaccine.stock`
- `vaccine.flow`
- `variant.stock`
- `variant.flow`

Each one exposes the same 2-level API:
```cpp
group.at(item, age_bucket)[day]
```

That keeps the model honest:
- `item` is “what”
- `age_bucket` is “who”
- `day` is “when”

and it avoids hiding structure behind synthetic column names.

The part I would push back on is using distinct types like `StatusGroup`, `VaxGroup`, `VariantGroup` if those names are meant to replace the storage abstraction. The important distinction is not “group type names,” it is fixed-domain vs runtime-domain. Status/condition fit enums well; vaccine/variant probably need a runtime label table.

If you want, the next concrete step is straightforward: I can turn Phase 1 into a refactor plan against [src/series.h](/Users/lewislevin/code/epi_sim/src/series.h) and [src/series.cpp](/Users/lewislevin/code/epi_sim/src/series.cpp) with the exact types and function signatures I’d change first.