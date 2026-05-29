# Implementation plan: add a ring dimension to the series subsystem

Branch: `ring_by_ai` of `epi_sim`. C++23, xmake, doctest-style tests in `test/`.

## Goal

Make per-ring outcome statistics available in the time-series subsystem without
changing the cost profile of the hot loop (event-driven updates in
`make_sick` / `make_well` / `make_dead` / vaccination), and without breaking any
existing output or test.

Rings are already wired into spread logic and the population table. This work is
**only** about the `SeriesGroup` / `AllSeries` accumulation, the `resolve_series`
read path, and the `SeriesColSpec` selection layer.

## Design summary (read before touching code)

The series is six named `SeriesGroup` fields on `AllSeries`
(`now_status`/`new_status`, `now_vax`/`new_vax`, `now_variant`/`new_variant`).
Each `SeriesGroup` currently indexes `subjects[subject_idx][AgeBucket][day]`.

Add ring as a new axis **between subject and age bucket**:
`subjects[subject_idx][ring][AgeBucket][day]`.

Ring index 0 is the reserved aggregate slot, `RING_ALL`. This reuses the
existing convention: `Ring::names[0]` is the empty sentinel, `assign_rings`
assigns real persons `1..nrings` (setup.cpp ~line 82: `Ring{r + 1}`), and
`build_ring_members` already treats outer index 0 as "unassigned". No real
person ever occupies ring 0 when rings are active.

`update` writes four cells instead of two: (this ring, this age),
(this ring, total age), (RING_ALL, this age), (RING_ALL, total age). The
RING_ALL mirror-writes accumulate the all-rings aggregate inline, so every
existing reader that asks for the aggregate gets today-identical numbers for
free. This is the regression guarantee.

Ring-dimension slot count = `max(Ring::names.size(), 1)`:
- No rings input: `Ring::names.size() == 0` -> 1 slot (index 0). Everyone is
  ring 0; per-ring and RING_ALL writes coincide; output bit-identical to today.
- N rings defined: `Ring::names.size() == N+1` (sentinel + N names) -> N+1 slots;
  index 0 = RING_ALL, indices 1..N = real rings. Correct.

Filtering by ring happens **only** at report time (`SeriesColSpec` /
`resolve_series`), never in accumulation. Accumulate full width unconditionally.

## Constants / types to introduce (these do NOT exist yet)

In `series.h`:

```cpp
// Reserved ring slot holding the all-rings aggregate. Reuses the unused
// sentinel index so no real person (rings are 1-based) ever collides with it.
inline constexpr uint8_t RING_ALL = 0;
```

There is no `RingBucket` enum analogous to `AgeBucket` — ring count is runtime
(from `Ring::names`), so the ring axis is a `std::vector`, not a fixed
`std::array`. Do not invent an enum for it.

## Step-by-step

### Step 1 — widen `SeriesGroup` (series.h + series.cpp)

`series.h`, change the storage type and signatures:

- `BucketArray` is currently `std::array<std::vector<int>, AgeBucket::COUNT>`.
  Keep that as the per-ring age array, and add a ring layer around it:
  ```cpp
  using BucketArray = std::array<std::vector<int>, size_t(AgeBucket::COUNT)>; // unchanged: one ring's age buckets
  using RingArray   = std::vector<BucketArray>;   // indexed by ring id (0 = RING_ALL)
  std::vector<RingArray> subjects;                // indexed by subject uint8_t
  ```
- Add `size_t n_rings{};` member.
- Constructor gains a ring count:
  `SeriesGroup(size_t n_subjects, size_t n_rings, size_t day_cnt);`
- `update` gains a ring arg **with no default** in the hot-path signature:
  ```cpp
  void update(uint8_t subject_idx, uint8_t ring, Agegrp agegrp, size_t day, int change);
  ```
- `at` gains a ring arg. To avoid editing every test call site, give it a
  **defaulted ring = RING_ALL** so existing `at(subject, bucket)` calls keep
  compiling and resolve to the aggregate:
  ```cpp
  std::vector<int>&       at(uint8_t subject_idx, AgeBucket bucket, uint8_t ring = RING_ALL);
  std::vector<int> const& at(uint8_t subject_idx, AgeBucket bucket, uint8_t ring = RING_ALL) const;
  ```
  (Note: bucket-then-ring ordering keeps the defaulted arg last. Internally it
  indexes `subjects[subject_idx][ring][size_t(bucket)]`.)

`series.cpp`:

- Constructor: allocate `subjects` as `n_subjects` x `n_rings` x `COUNT` x
  `(day_cnt+1)`, all zero.
- `update` body (the four-write version):
  ```cpp
  void SeriesGroup::update(uint8_t subject_idx, uint8_t ring, Agegrp agegrp,
                           size_t day, int change) {
      auto bucket = size_t(bucket_from_age(agegrp));
      auto& subj  = subjects[subject_idx];
      subj[ring][bucket][day]                       += change;
      subj[ring][size_t(AgeBucket::total)][day]     += change;
      if (ring != RING_ALL) {
          subj[RING_ALL][bucket][day]                   += change;
          subj[RING_ALL][size_t(AgeBucket::total)][day] += change;
      }
  }
  ```
  The `if (ring != RING_ALL)` guard prevents double-counting in the no-rings
  case where everyone is ring 0 (then ring == RING_ALL and the first two writes
  already are the aggregate).

### Step 2 — `AllSeries` constructor (series.h + series.cpp + sim.cpp + tests)

- Add `size_t n_rings;` param to `AllSeries(size_t day_cnt, const PopData& pop,
  size_t n_variants, size_t n_vax, size_t n_rings)` and pass it into every
  `SeriesGroup` member init.
- Compute the slot count at the **call site**, not inside: in sim.cpp:31
  ```cpp
  size_t n_ring_slots = std::max<size_t>(Ring::names.size(), 1);
  AllSeries series(model.ndays, pop, Variant::names.size(),
                   Vax::names.size(), n_ring_slots);
  ```
- The day-1 seeding block in the `AllSeries` ctor (UNEXPOSED counts from
  `pop.agegrp_parts`) writes via `now_status.subjects[...][bucket][1]`. Those
  writes must now also target `RING_ALL` (index 0) explicitly, since the
  storage gained a ring layer. Verify the seed lands in `[UNEXPOSED][RING_ALL][bucket][1]`.
  Decide whether to ALSO seed per-ring day-1 UNEXPOSED counts (needed if you
  want correct per-ring unexposed stocks). For first pass, seeding only
  RING_ALL is acceptable and keeps aggregate output identical; per-ring day-1
  UNEXPOSED can be a follow-up (see "Known limitations").
- `init_history_series` carry-forward loops: extend to iterate rings, OR carry
  forward only the slots actually read. Simplest correct version: nest a ring
  loop `for (uint8_t r = 0; r < grp.n_rings; ++r)` inside the existing
  subject/bucket loops for all three now_* groups.
- `validate_variant_invariant`: reads `at(v, AgeBucket::total)` — with the
  defaulted ring arg this now reads RING_ALL automatically. Confirm it still
  compiles and passes (it checks aggregate variant sum == aggregate infectious,
  both at RING_ALL).
- Tests: `test/test_series.cpp` `make_series` helper (line 13) calls the 4-arg
  `AllSeries` ctor. Update it to pass a ring count (use 1, or
  `max(Ring::names.size(),1)`). All the `series.now_status.at(subject, bucket)`
  calls in the tests keep working via the defaulted ring arg.

### Step 3 — hot-loop call sites (disease_modeling.cpp, vaccination.cpp)

`AgentView::ring()` already exists (population.h:199), so the ring is in scope
in all three `AgentView` methods. Pass `ring().v`:

- `disease_modeling.cpp` `make_sick` (lines 44-53), `make_well` (87-90),
  `make_dead` (112-115): every `series.<grp>.update(SUBJECT, this_age, today, X)`
  becomes `series.<grp>.update(SUBJECT, ring().v, this_age, today, X)`.
- `vaccination.cpp` (lines 110-111): the agent is `agent`, so use
  `agent.ring().v`:
  `series.new_vax.update(uint8_t(choice), agent.ring().v, agent.agegrp(), today, 1);`
  (and the `now_vax` line).

There are exactly these update call sites (grep `\.update(` across
disease_modeling.cpp + vaccination.cpp confirms 12 + 2). No others.

### Step 4 — read/selection layer (series.h + series.cpp + callers)

This is where per-ring filtering is exposed. Keep it additive and default to
RING_ALL so all existing `SeriesColSpec` usage is unchanged.

- `SeriesSelection` is currently `std::pair<string,string>` = (name, bucket).
  Extend the resolved column identity with a ring. Two options; pick (a):
  (a) Keep `SeriesSelection` as-is and add an optional ring qualifier parsed
      from the name string, e.g. `"now_infectious@ring:Jail"` or
      `"now_infectious@ring:3"`. Lowest-churn: no struct/signature changes to
      `SeriesColSpec` constructors, all current call sites compile untouched.
  (b) Change `SeriesSelection` to a struct `{string name; string bucket;
      uint8_t ring = RING_ALL;}`. Cleaner long-term but touches every
      `SeriesColSpec` ctor and the structured-binding loops in
      print/serialize/plot.
  Recommend (a) for this pass.

- `resolve_series` gains a ring argument (default RING_ALL):
  ```cpp
  std::optional<vector<int>> resolve_series(const AllSeries& series,
      std::string_view name, AgeBucket bucket, uint8_t ring = RING_ALL);
  ```
  Thread `ring` into each `.at(idx, bucket, ring)` call inside it. The
  `now_vaccinated` aggregate loop also takes `ring`.

- If using parse option (a): add a small helper that splits a selection name
  into (base_name, ring_id) by looking for `"@ring:"`, resolving the suffix
  against `Ring::names` (by name) or as a decimal index, defaulting to
  RING_ALL when absent. Call it in `print_selected_series`,
  `serialize_selected_series`, and `seriesplot` (plot.cpp:117) right before
  they call `resolve_series`, and fold the ring suffix into the emitted column
  label so CSV headers disambiguate rings.

- `SeriesColSpec` "all" sentinel (`build_for_buckets`): leave it emitting
  RING_ALL columns by default (today's behavior). Optionally add a sentinel
  variant later to expand across rings; not required for this pass.

### Step 5 — callers that must keep compiling/passing

- `src/sim.cpp` lines 175, 189-222: `serialize_selected_series` / `seriesplot`
  calls with `{name, bucket}` selections — unchanged under parse option (a).
- `src/plot.cpp:98-117` `seriesplot` — add the same ring-suffix parse before
  `resolve_series`, mirror of print/serialize.
- `test/test_series.cpp` — update `make_series` ctor arity (Step 2); everything
  else compiles via defaulted args. Add NEW tests (Step 6).

## Step 6 — tests to add

In `test/test_series.cpp`:

1. **No-rings identity:** build `AllSeries` with `n_rings = 1`, drive a few
   `update`s with ring = 0, assert RING_ALL slot equals the per-ring slot and
   equals pre-change expected totals (proves the no-ring path is unchanged).
2. **Aggregate equals sum of rings:** build with `n_rings = 3`, update subjects
   in rings 1 and 2, assert `at(subject, bucket, RING_ALL)` ==
   `at(subject,bucket,1) + at(subject,bucket,2) + at(subject,bucket,3)` for
   several buckets and days.
3. **No double count:** after updates in real rings, assert RING_ALL is not
   inflated (i.e. each real update contributed exactly once to RING_ALL).
4. **resolve_series ring arg:** seed distinct values in rings 1 vs 2, assert
   `resolve_series(..., ring=1)` and `ring=2` return the right vectors and
   `ring=RING_ALL` returns the sum.
5. **Selection parse (if option a):** `"now_infectious@ring:1"` resolves to ring
   1; bare `"now_infectious"` resolves to RING_ALL.

Existing tests in `test_series.cpp`, `test_runsim.cpp`, `test_pop_serialize.cpp`
must remain green unchanged (except the `make_series` ctor arity bump).

## Validation gate (do this before adding per-ring reporting polish)

1. Implement Steps 1-3 + Step 2 test arity fix only. Leave the read path
   defaulting to RING_ALL (no Step 4 yet).
2. Build, run full test suite, run a real simulation **with no rings** and a
   real simulation **with rings defined**.
3. Confirm: with no rings, all CSV/plot/print output is byte-identical to the
   pre-change branch. With rings, `validate_variant_invariant` passes (it reads
   RING_ALL) and aggregate columns match the no-ring totals for an equivalent
   run.
4. Only after that gate is green, add Step 4 (ring filtering) + Step 6 tests
   4-5.

## Known limitations to note in the PR (not blockers)

- Per-ring day-1 UNEXPOSED seeding: first pass seeds only RING_ALL. Per-ring
  initial stocks need `assign_rings` results (or `build_ring_members`) threaded
  into the `AllSeries` ctor. Follow-up.
- Apportionment stragglers: `assign_rings` uses `pop.apportion` per age group;
  rounding can leave a person at ring 0. `build_ring_members` already skips
  these. For series, a straggler's events land in RING_ALL only and in no real
  ring — benign for aggregates, slight under-count in a real ring's column.
  Worth a later check of `apportion` coverage.
- Memory: each `SeriesGroup` grows ~ (n_rings+1)x. Counters only, fine to hold,
  but never emit all-rings x all-subjects x all-ages by default — rely on
  selection.

## Files touched

- `src/series.h` — types, constants, ctor/update/at/resolve signatures
- `src/series.cpp` — ctor, update, init_history_series, resolve_series,
  print/serialize ring-suffix parse + labels
- `src/sim.cpp` — `AllSeries` construction (line 31), n_ring_slots
- `src/disease_modeling.cpp` — 12 update call sites (make_sick/well/dead)
- `src/vaccination.cpp` — 2 update call sites
- `src/plot.cpp` — `seriesplot` ring-suffix parse before resolve_series
- `test/test_series.cpp` — `make_series` ctor arity + new ring tests

`Ring::names`, `assign_rings`, `build_ring_members`, `AgentView::ring()` already
exist and are NOT modified.
