# Series Refactoring Plan

Replace `HistorySeries` with `AllSeries` containing typed `SeriesGroup` members.
Goal: support runtime-determined vax and variant series columns using the same
pattern as the existing compile-time status series.

Read AGENTS.md first for general repo conventions.

## Rule: validate after every step

After every code change, build and run the validation commands below.
Do not report a step as complete until validation passes.
If the diff is non-empty, the step is broken. Fix before proceeding.

```
TODO_BUILD_COMMAND
TODO_RUN_COMMAND
diff output/status_series.csv reference/status_series.csv
```

## Target data structures

These go in series.h, replacing the current HistorySeries.

```cpp
struct SeriesGroup {
    using BucketArray = std::array<std::vector<int>, size_t(AgeBucket::COUNT)>;

    std::vector<BucketArray> subjects;  // indexed by trait's uint8_t value
    size_t day_cnt{};

    SeriesGroup() = default;
    SeriesGroup(size_t n_subjects, size_t day_cnt);

    // Increments subjects[subject_idx] for both the specific age bucket
    // AND the total bucket (index 0). Every call site uses this —
    // no caller should touch BucketArray directly for updates.
    void update(uint8_t subject_idx, Agegrp agegrp, size_t day, int change);

    auto& at(uint8_t subject_idx, AgeBucket bucket) {
        return subjects[subject_idx][size_t(bucket)];
    }
    auto const& at(uint8_t subject_idx, AgeBucket bucket) const {
        return subjects[subject_idx][size_t(bucket)];
    }
};
```

The constructor allocates `n_subjects` BucketArrays, each with `day_cnt + 1`
zero-filled vectors (days are 1-indexed, slot 0 unused).

`update()` implementation:
```cpp
void SeriesGroup::update(uint8_t subject_idx, Agegrp agegrp, size_t day, int change) {
    auto bucket = size_t(bucket_from_age(agegrp));
    subjects[subject_idx][bucket][day] += change;
    subjects[subject_idx][size_t(AgeBucket::total)][day] += change;
}
```

Top-level container:
```cpp
struct AllSeries {
    SeriesGroup now_status, new_status;
    SeriesGroup now_vax,    new_vax;
    SeriesGroup now_variant, new_variant;
    size_t day_cnt;

    AllSeries(size_t day_cnt, const PopData& pop,
              size_t n_variants, size_t n_vax);
};
```

The constructor initializes each SeriesGroup:
- `now_status`, `new_status`: `Status::names.size()` subjects
- `now_vax`, `new_vax`: `n_vax` subjects (from `Vax::names.size()`)
- `now_variant`, `new_variant`: `n_variants` subjects (from `Variant::names.size()`)

It also seeds `now_status` for UNEXPOSED on day 1 from PopData age bucket
counts, same as the current `HistorySeries(day_cnt, pop)` constructor does.

Nesting summary:
- AllSeries (one instance, passed by reference everywhere)
  - 6 SeriesGroup members (compile-time named fields)
    - vector of BucketArray (indexed by trait uint8_t: Status, Vax, or Variant)
      - array of 6 vector<int> (indexed by AgeBucket enum)
        - vector<int> (indexed by simulation day, 1-based)

## Step 1: Replace HistorySeries with AllSeries for status series only

- Add `SeriesGroup` and `AllSeries` to series.h
- `AllSeries` constructor only needs to set up `now_status` and `new_status`
  for now. Declare `now_vax` etc. as members but leave them default-constructed.
- Change all function signatures that take `HistorySeries&` to take `AllSeries&`
- Convert every `series.update_series(SeriesName::now_infected, agegrp, day, delta)`
  call to `series.now_status.update(INFECTIOUS, agegrp, day, delta)`.
  Mapping:
    - `SeriesName::now_unexposed` → `series.now_status.update(UNEXPOSED, ...)`
    - `SeriesName::now_infected`  → `series.now_status.update(INFECTIOUS, ...)`
    - `SeriesName::now_recovered` → `series.now_status.update(RECOVERED, ...)`
    - `SeriesName::now_dead`      → `series.now_status.update(DEAD, ...)`
    - `SeriesName::now_vaccinated`→ moves to vax series (see below)
    - `SeriesName::new_infected`  → `series.new_status.update(INFECTIOUS, ...)`
    - `SeriesName::new_recovered` → `series.new_status.update(RECOVERED, ...)`
    - `SeriesName::new_dead`      → `series.new_status.update(DEAD, ...)`
    - `SeriesName::new_vaccinated`→ moves to vax series (see below)
    - `SeriesName::net_infected`  → drop it. Compute on demand if needed.
- Initialize `now_vax` and `new_vax` in AllSeries constructor using
  `Vax::names.size()` as subject count. Vaccinated is not a Status value —
  it belongs in the vax SeriesGroup, indexed by brand (Vax trait uint8_t).
- Convert the existing `update_series(SeriesName::now_vaccinated, ...)`
  and `new_vaccinated` calls in doshots to `series.now_vax.update(...)` and
  `series.new_vax.update(...)` using the person's vax brand as subject index.
  Currently these only fire on first shot. Keep that behavior for now;
  step 3 is where the human developer will decide whether to also track
  second shots and boosters.
- Update `init_history_series`, `finalize_series`, serialization functions
  to work with AllSeries
- Delete old `HistorySeries` struct and `SeriesName` enum once nothing
  references them
- VALIDATE: build, run, diff status CSV against reference. Must be identical.
  Vax series will have values (existing first-shot tracking carries over).

## Step 2: Add variant SeriesGroup

- Initialize `now_variant` and `new_variant` in AllSeries constructor using
  `Variant::names.size()` as subject count
- No new accumulators wired up yet — variant series will be empty
- VALIDATE: status output unchanged

## Step 3: Wire up accumulators at call sites (HUMAN-DRIVEN)

The human developer will update the call sites. After changes are made,
run the validation commands to verify. If the human asks you to validate
or test, build and run the sim and check the output.

This step requires care. Every place a person's vax or variant state changes,
add the corresponding series update.

Variant accumulators:
- `make_sick`: already receives `Variant var`. Add:
  `series.new_variant.update(var, agegrp(), today, 1);`
  `series.now_variant.update(var, agegrp(), today, 1);`
- `make_well`: person is leaving infectious. The variant they had is
  `this->variant()`. Add:
  `series.now_variant.update(variant(), agegrp(), today, -1);`
- `make_dead`: same pattern as make_well for the decrement.

Vax accumulators:
- `doshots`: the existing now_vax/new_vax updates from step 1 already
  cover this — first shot only, indexed by brand. This gives "people
  vaccinated with brand X" which is the useful simulation number.
  No changes needed here unless the human decides otherwise.

After wiring, add vax and variant series to serialization/CSV output.

VALIDATE: status diff still clean. Eyeball vax and variant CSVs — nonzero
values should appear on days when seeding/vaccination occurs. Ask me to
review the CSV if unsure.

## Step 4: Cleanup

- Remove any bridge code, old SeriesName enum remnants, compatibility shims
- Remove `net_infected` if still lingering anywhere
- VALIDATE
