# Social Distancing Implementation — Canonical Plan (v4)

## Status

- **Phase 1**: complete.
- **Phase 2**: complete and validated via the `this` scratch target.
- **Phase 3**: complete. Setup helpers merged into `load_sd_cases` (see below);
  `epi_sim.cpp` now makes a single loader call. Obsolete cancellation comment
  block removed from `sample_parameters/soc_dist.json`.
- **Phase 4**: not started.
- **Phase 5**: not started.


## Naming

- **`SocialDistancing`** — the data struct defining one case (in `cases.h`):
  name, startday, endday, comply, deltas, include_ages, shifted factor
  matrices.
- **`SDCase`** — the runtime trait (in `traits.h`): `uint8_t` index + static
  `names` vector, matches the `Variant` / `Vax` pattern. Stored per-agent in
  `pop.sdcase`.

## Goal

Make per-agent social-distancing factors active in `spread()`. An agent whose
`pop.sdcase[i]` is nonzero uses the shifted `contactfactors` / `touchfactors`
from the referenced `SocialDistancing` instead of the global `SocialParams`
factors. No other spread logic changes.

## Design decisions (locked)

- **Runtime trait `SDCase`**: pattern matches `Variant` / `Vax`. Index 0 =
  "none", indices 1..N = loaded cases in insertion order. `pop.sdcase[i]`
  value is an `SDCase` whose `v` indexes `SDCase::names`.
- **`pop.sdcase` type**: changes from `vector<uint8_t>` to `vector<SDCase>`
  (trait-typed). Done in Phase 4 alongside the first per-agent writes.
- **Cancellation is a JSON field, not a separate case.** Each
  `SocialDistancing` carries its own `endday`. No cancel entries. No
  setup-time pairing.
- **Compliance**: re-drawn each day. `pop.sdcase[i]` is per-day state, not
  persistent across days.
- **Overlap**: not enforced. Iteration order means last-active-case-to-write
  wins.
- **Shifter**: two new overloads in `helpers.h` / `.cpp`, one per array shape.
  No template.
- **`PopData` / `AgentView` / `SocialParams` unchanged.**
- **JSON schema**: new optional `endday` on each SD case (default `INT_MAX` =
  never ends). Files without `endday` continue to load.

### Example JSON (updated `sample_parameters/soc_dist.json`)

```json
[
  {
    "name": "young",
    "startday": 100,
    "endday": 300,
    "comply": 0.8,
    "contact_delta": [0.2, 1.5],
    "touch_delta": [0.2, 1.8],
    "include_ages": ["Age20_39", "Age40_59"]
  },
  {
    "name": "old",
    "startday": 100,
    "endday": 400,
    "comply": 0.9,
    "contact_delta": [0.2, 1.5],
    "touch_delta": [0.2, 1.8],
    "include_ages": ["Age60_79", "Age80_up"]
  }
]
```

Updated as part of Phase 3, not earlier.

---

## Phase 1 — Foundations (standalone additions, no wiring)

### 1a. `src/traits.h` — `SDCase` runtime trait

Modeled on `Variant` / `Vax`:

- `uint8_t v{};`
- `inline static std::vector<std::string> names;`
- Default, `uint8_t`, `int` constructors;
  `explicit SDCase(std::string_view name)` that asserts
  first-must-be-`"none"` and appends to `names`.
- `std::string show() const noexcept` returning `names[v]` or `""` if out of
  range.
- `operator uint8_t()`, default `operator==`.

### 1b. `src/helpers.h` / `src/helpers.cpp` — 2D `shifter` overloads

Alongside the existing 1D `shifter(vector<float>&, float, float)`:

```cpp
void shifter(array<array<float,5>,4>& arr, float newmin, float newmax);
void shifter(array<array<float,5>,6>& arr, float newmin, float newmax);
```

Each implementation: one pass to find oldmin/oldmax over all elements, second
pass to rescale in place. Edge-case handling matches the 1D version (all-equal
→ fill with 1.0f).

### 1c. `src/cases.h` — rename `SDCase` → `SocialDistancing`; add `endday`

Rename the existing data struct `SDCase` to `SocialDistancing` (the name
`SDCase` is now the trait in `traits.h`). Add a trailing `endday` member:

```cpp
int endday{INT_MAX};
```

Also reorder members so `include_ages` comes before
`contactfactors` / `touchfactors`. This matches the current 6-arg brace-init
in `load_sd_case` (which relies on `contactfactors` / `touchfactors` being
default-initialized as trailing members) and keeps aggregate initialization
valid once `endday` is added. Final order:

```cpp
struct SocialDistancing {
  string name;
  int startday{0};
  float comply{0.0f};
  vector<float> contact_delta;
  vector<float> touch_delta;
  vector<Agegrp> include_ages;
  array<array<float, 5>, 4> contactfactors {};
  array<array<float, 5>, 6> touchfactors {};
  int endday{INT_MAX};
};
```

Update call sites that referenced the old struct name to
`SocialDistancing`: `cases.cpp` (loader / printer), `sim.h` / `sim.cpp`
(`runsim` signature), `epi_sim.cpp` (local `sd_cases` declaration). No other
code depends on the layout.

### Phase 1 validation

- Full build succeeds.
- No behavior change.

---

## Phase 2 — JSON loader and merged setup

### 2a. `src/cases.cpp` — update `load_sd_case`

Read `endday` from JSON with a default, assigned after brace-init (the struct
has defaulted factor matrices sitting between `include_ages` and `endday`, so
positional aggregate-init with a trailing `endday` does not compile cleanly):

```cpp
SocialDistancing sd{sdc["name"], sdc["startday"], sdc["comply"],
                    sdc["contact_delta"], sdc["touch_delta"], std::move(inc_ages)};
sd.endday = sdc.value("endday", INT_MAX);
return sd;
```

`json::value(key, default)` handles missing keys cleanly and avoids exceptions
for backward-compatible files.

### 2b. `src/cases.h` / `.cpp` — merged loader

Single public entry point. The shift and trait-name registration are folded
into `load_sd_cases` itself, matching the pattern used by `load_variants_data`
and `load_vax_data` (both of which mutate the corresponding `::names` vector
during load):

```cpp
vector<SocialDistancing> load_sd_cases(const json& jdata, const SocialParams& social);
```

Implementation, in order:

1. Build the vector by calling `load_sd_case(sdc)` for each JSON entry.
2. For each `sdc`:
   - Validate `contact_delta.size() == 2` and `touch_delta.size() == 2`;
     throw with the case name on failure.
   - Copy `social.contactfactors` into `sdc.contactfactors` and call the 4×5
     `shifter` overload with `sdc.contact_delta[0]`, `sdc.contact_delta[1]`.
   - Same for touch (6×5).
3. `SDCase::names.clear();` — matches `Variant::names.clear()` in
   `load_variants_data`.
4. `SDCase{"none"};` — registers index 0.
5. For each `sdc`, `SDCase{sdc.name};` — indices 1..N match positions in the
   returned vector.

No separate `apply_shifts_to_sd_cases` / `register_sd_case_names` helpers are
exposed.

### 2c. Update `print_sd_cases`

Add `endday`, plus the shifted `contactfactors` / `touchfactors` matrices to
the per-case output so validation can inspect the scaling visually.

### Phase 2 validation (done)

- Full build succeeds.
- `this` scratch target prints:
  - `endday = 300` when present, `endday = 2147483647` (= `INT_MAX`) when
    omitted.
  - Shifted `contactfactors` range ⊆ `[0.2, 1.5]`;
    `touchfactors` range ⊆ `[0.2, 1.8]`.
  - `SDCase::names = [none, young, old]`.

---

## Phase 3 — Wire merged loader into `src/epi_sim.cpp`

Update `main`:

```cpp
Model model = setup_sim(config);
vector<SeedCase> seedcases = load_seed_cases(seed_json, model.pop, model.mp);
vector<SocialDistancing> sd_cases;
if (!sd_seed_path.empty()) {
  sd_cases = load_sd_cases(load_json_params(sd_seed_path), model.mp.socialdata);
}
runsim(model, seedcases, sd_cases);
```

Single loader call. No separate shift or trait-registration step in `main`.

Also: remove the obsolete trailing sentinel-cancellation comment block from
`sample_parameters/soc_dist.json` (pre-`endday` design note, no longer
accurate).

### Phase 3 validation (done)

- `epi_sim` and `this` targets both build clean.
- With `--sd_seed soc_dist.json`: shifted matrices and
  `SDCase::names = [none, young, old]` confirmed via scratch output.
- Without `--sd_seed`: `sd_cases` is default-constructed empty and never
  passed through the loader; `SDCase::names` stays empty (acceptable —
  `spread()` will still be skipping SD logic until Phase 5).

---

## Phase 4 — Per-day application (`apply_sd_cases_for_day`)

Purpose: make `pop.sdcase` reflect today's compliance state.

### Change `pop.sdcase` type in `src/population.h`

Change the declaration from `vector<uint8_t> sdcase;` to
`vector<SDCase> sdcase;`. `SDCase` is the runtime trait defined in
`traits.h`; it has an implicit conversion to `uint8_t`, a default
constructor producing `v == 0` ("none"), and the same size as a `uint8_t`,
so allocation, initialization, and comparisons remain equivalent. Any
existing index-based reads of `pop.sdcase[i]` continue to work because
`SDCase` converts implicitly to `uint8_t`.

### Declaration in `src/cases.h`

```cpp
void apply_sd_cases_for_day(int day, const vector<SocialDistancing>& sd_cases, PopData& pop);
```

### Implementation in `src/cases.cpp`

Logic:

- Determine whether any case is active today
  (`sdc.startday <= day && day < sdc.endday`). If none, return immediately.
- Otherwise, set `pop.sdcase[i] = SDCase{}` for `i = 1..popn` (resets to
  index 0 / "none").
- For each active case (indexed `k` in `sd_cases`, 0-based):
  - For each agent `i = 1..popn` whose `agegrp` is in `sdc.include_ages`:
    - Draw `xo::bernoulli(sdc.comply)`; on hit, set
      `pop.sdcase[i] = SDCase{static_cast<uint8_t>(k + 1)}`.
- Iteration order = vector order; later active cases overwrite earlier ones
  on overlap.

### Wire into `src/sim.cpp`

Replace the empty stub at line 86 with:

```cpp
apply_sd_cases_for_day(sim::ds.day, sd_cases, pop);
```

Remove the current empty `for (auto& sdc : sd_cases) { }` stub.

### Phase 4 validation

- Temporary log inside `apply_sd_cases_for_day` when it mutates: day, count
  of nonzero sdcase per case index.
- On `startday`: count ≈ `comply × count(agegrp ∈ include_ages)`.
- On `startday + 1`: count fluctuates around that expected value (re-draw).
- On `day == endday` and after: count for that case returns to 0.
- Full sim output must still match pre-phase-4 run, because `spread()` still
  ignores `pop.sdcase`.

---

## Phase 5 — Spread integration

### `src/spread.h` and `src/spread.cpp`

Add a parameter to `spread()`:

```cpp
void spread(PopData& pop, AllSeries& series, AgentView person, SocialParams& social,
            vector<InfectParams>& infectparams, const VaxSet& vaxset, bool dovax,
            vector<size_t>& contacts, float density_factor,
            vector<float>& indoor_seq,
            const vector<SocialDistancing>& sd_cases);   // new, at end
```

Inside `spread()`, replace the existing two aliases:

```cpp
const auto& contactfactors = social.contactfactors;
const auto& touchfactors   = social.touchfactors;
```

with:

```cpp
const uint8_t sd_idx = person.sdcase();   // SDCase → uint8_t implicit
const auto& contactfactors = sd_idx == 0 ? social.contactfactors
                                         : sd_cases[sd_idx - 1].contactfactors;
const auto& touchfactors   = sd_idx == 0 ? social.touchfactors
                                         : sd_cases[sd_idx - 1].touchfactors;
```

No other logic changes. Downstream `contactfactors[...][...]` /
`touchfactors[...][...]` reads work unchanged because shapes match.

### `src/sim.cpp` call site

Update the one `spread(...)` call inside the per-person loop to pass
`sd_cases` as the final argument.

### Phase 5 validation

- **Regression**: with `sd_cases` empty (no `--sd_seed`), behavior must be
  bitwise identical to the pre-phase-5 run — same seed → identical series
  output. This is the final correctness gate.
- **With SD cases**: a full run with `soc_dist.json`. Expect reduced total
  infections proportional to `contact_delta` / `touch_delta` and to the
  fraction of population covered by `include_ages × comply`.

---


## Files touched (canonical)

| File | Phase | Change |
|------|-------|--------|
| `src/traits.h` | 1 | Add `SDCase` runtime trait. |
| `src/helpers.h` / `.cpp` | 1 | Add two 2D `shifter` overloads. |
| `src/cases.h` | 1, 2, 4 | Rename `SDCase` → `SocialDistancing`; add `endday`; update `load_sd_cases` signature to take `const SocialParams&`; declare `apply_sd_cases_for_day`. |
| `src/cases.cpp` | 2, 4 | Update `load_sd_case` to read `endday`; merge shift + `SDCase::names` registration into `load_sd_cases`; extend `print_sd_cases` to show `endday` and shifted matrices; implement `apply_sd_cases_for_day`. |
| `src/sim.h` / `.cpp` | 1, 4, 5 | Update `runsim` signature to `vector<SocialDistancing>`; replace stub with `apply_sd_cases_for_day`; pass `sd_cases` to `spread()`. |
| `src/epi_sim.cpp` | 1, 3 | Update `sd_cases` declaration; single-call `load_sd_cases(json, model.mp.socialdata)` before `runsim`. |
| `src/population.h` / `.cpp` | 4 | Change `pop.sdcase` from `vector<uint8_t>` to `vector<SDCase>`. |
| `src/spread.h` / `.cpp` | 5 | Add `const vector<SocialDistancing>&` parameter; rebind factor aliases per-agent. |
| `sample_parameters/soc_dist.json` | 3 | Add `endday` to each entry; remove obsolete cancellation comment block. |
| `scratch/load_sdcases.cpp` | 2, 3 | Use single-call merged loader. |

No changes to: `parameters.h` / `.cpp`, `setup.h` / `.cpp`.

---

## Implementation order

One phase at a time. Each phase must build and run before moving on.
Phase 5's regression test (with `--sd_seed` empty producing unchanged
output) is the final correctness gate.
