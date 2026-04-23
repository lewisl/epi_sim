# HistorySeries

`HistorySeries` stores the simulation's day-by-day aggregate counts for a fixed set of epidemiological series, split by age bucket.

The physical layout is:

```cpp
cols[series_name][age_bucket][day]
```

That is not a matrix in the usual sense. It is closer to "an array of grouped column vectors":

- the outer array chooses what is being measured
- the inner array chooses which age bucket
- each leaf `std::vector<int>` holds values over time

## Verified Mental Model

The simplified description is mostly right, with a few corrections.

- `HistorySeries` has two data members:
  - `day_cnt`
  - `cols`
- `cols` is a nested fixed-size array of vectors, not a flat 2D array.
- Access is typed with two scoped enums, not one:
  - `SeriesName` for the outer axis
  - `AgeBucket` for the inner axis
- There is a string-based selection layer, but it is not elaborate. It is mainly:
  - `to_string(...)` to map enum to label
  - `series_name_from_string(...)` and `age_bucket_from_string(...)` to parse labels back to enums
- Semantically, the structure is:
  - 10 series groups
  - each group has 6 age-bucket vectors
  - each vector has length `day_cnt + 1`
- Day indexing is 1-based. Slot `0` is intentionally unused.

So this statement is accurate:

- "It is closer to a struct of vectors than to a matrix."

The reason is also accurate:

- runtime selection of a series group is easy with array indexing and enums
- a real `struct` would not support `some_runtime_value` selecting one member directly

## Axes

### `SeriesName`

The outer axis has 10 values:

1. `now_infected`
2. `now_unexposed`
3. `now_recovered`
4. `now_dead`
5. `now_vaccinated`
6. `new_infected`
7. `new_recovered`
8. `new_dead`
9. `new_vaccinated`
10. `net_infected`

Broadly:

- `now_*` series are end-of-day stock counts
- `new_*` series are within-day flow counts
- `net_infected` is derived at the end from `now_infected`; it is not incremented during the day

### `AgeBucket`

The inner axis has 6 values:

1. `total`
2. `age0_19`
3. `age20_39`
4. `age40_59`
5. `age60_79`
6. `age80_up`

So the full container holds `10 x 6 = 60` day-series vectors.

## Type Layout

From `src/series.h`:

```cpp
using BucketSeries = std::array<std::vector<int>, size_t(AgeBucket::COUNT)>;

size_t day_cnt;
std::array<BucketSeries, size_t(SeriesName::COUNT)> cols;
```

Equivalent mental expansion:

```cpp
std::array<
    std::array<std::vector<int>, 6>,
    10
> cols;
```

## Constructors

### `HistorySeries(size_t day_cnt)`

This constructor:

- stores `day_cnt`
- default-constructs `cols`
- assigns every leaf vector to length `day_cnt + 1`
- fills every entry with `0`

Important detail:

- `day_cnt` is initialized in the member initializer list
- `cols` is default-constructed first
- the constructor body then mutates each vector with `assign(day_cnt + 1, 0)`

### `HistorySeries(size_t day_cnt, const PopData& pop)`

This delegates to the first constructor, then seeds day 1 of `now_unexposed`:

- `total` bucket gets `pop.popn`
- each age bucket gets the corresponding `pop.agegrp_parts[i]`

If `day_cnt == 0`, it returns without seeding.

This is the constructor used in the main simulation path.

## Day Indexing

`HistorySeries` uses 1-based indexing for days:

- valid simulation days are `1..day_cnt`
- slot `0` is allocated but unused by normal logic

That is why vectors are sized to `day_cnt + 1`.

This is separate from the project's `PopData` person indexing, but the convention is similar: index `0` is reserved and ordinary logic starts at `1`.

## Accessors

### `at(SeriesName name, AgeBucket bucket)`

Returns the vector for one specific `(series, bucket)` pair.

Example:

```cpp
series.at(SeriesName::now_infected, AgeBucket::total)[day]
```

This is the main access API.

### `group(SeriesName name)`

Returns the entire 6-vector bucket group for one `SeriesName`.

Use this when code wants all age buckets for a single semantic series.

## Enum/String Helpers

These helpers make runtime text selection possible without exposing raw integer indices:

- `to_string(SeriesName)`
- `to_string(AgeBucket)`
- `series_name_from_string(std::string_view)`
- `age_bucket_from_string(std::string_view)`

These are used by printing and plotting code that accepts pairs of strings such as:

```cpp
{"now_infected", "age60_79"}
```

The selection type is:

```cpp
using SeriesSelection = std::pair<string, string>;
```

## Member Functions That Update Series

### `init_history_series(size_t day)`

This starts a new day by carrying forward the stock series from the previous day.

For `day > 1`, it copies yesterday's value into today's slot for every age bucket of:

- `now_infected`
- `now_unexposed`
- `now_recovered`
- `now_dead`
- `now_vaccinated`

It does not copy any `new_*` series, because those are per-day flows.

### `delta_series(SeriesName name, Agegrp agegrp, size_t day, int change)`

This applies one event delta to both:

- the `total` bucket
- the specific age bucket corresponding to `agegrp`

This is the main helper used by simulation logic when one person changes state.

Examples in the current code:

- infection increments `new_infected` and `now_infected`
- recovery increments `new_recovered` and `now_recovered`, and decrements `now_infected`
- death increments `new_dead` and `now_dead`, and decrements `now_infected`
- first vaccination increments `new_vaccinated` and `now_vaccinated`

### `finalize_series()`

This is an end-of-simulation post-processing step.

Currently it computes `net_infected` as the first difference of `now_infected`, bucket by bucket:

- day 1: `net_infected[1] = now_infected[1]`
- later days: `net_infected[day] = now_infected[day] - now_infected[day - 1]`

So `net_infected` is a derived series, not a directly maintained one.

## Free Functions Related to `HistorySeries`

### `AgeBucket bucket_from_age(Agegrp agegrp)`

Maps the project's `Agegrp` value to the corresponding `AgeBucket`.

This is used by `delta_series(...)` to keep:

- the `total` bucket
- the matching age-specific bucket

in sync.

### `write_daily_trace_csv(...)`

Debug-oriented CSV output for selected daily series.

Current behavior:

- validates `caldays.size() == series.day_cnt`
- creates parent directories if needed
- writes a CSV file from selected `new_*` columns

Current status:

- marked in code as "debug code currently not called"
- the CSV header text appears stale relative to the values actually written

So this function should be treated as debug-only until it is cleaned up.

### `print_total_status_series(...)`

Pretty-prints blocks of days for the total bucket of:

- infected
- unexposed
- recovered
- dead

This is a console inspection helper.

### `print_selected_series(...)`

Pretty-prints any caller-selected `(series_name, age_bucket)` columns.

The function:

- parses each string pair with `series_name_from_string(...)` and `age_bucket_from_string(...)`
- rejects unknown names
- prints day blocks for the requested vectors

This is the main runtime text-selection utility for `HistorySeries`.

## Update Lifecycle in the Simulation

The main simulation uses `HistorySeries` in this order:

1. Construct `HistorySeries series(model.ndays, pop);`
2. Seed day 1 `now_unexposed` from the initial population
3. At the start of each day, call `series.init_history_series(day)`
4. During the day, apply transitions with `series.delta_series(...)`
5. After the day loop, call `series.finalize_series()`

That means:

- stock series are carried forward each day, then adjusted by events
- flow series are written only for the day where events occur
- `net_infected` is derived after the simulation completes

## Important Distinction: Physical Layout vs Semantics

Physically, the data is:

```cpp
array -> array -> vector
```

Semantically, it is:

- one group per epidemiological series
- one vector per age bucket within that group
- one entry per day within that vector

That is why "array of grouped vectors" is the best mental model for this class.
