## Spread Hot-Path Optimization Plan

### Summary
`xmake` is already building with `set_optimize("fastest")`, so this should be treated as a code-path optimization effort, not a compiler-flags problem. The plan should stay behavior-preserving, avoid parallelism/approximations in the first pass, and optimize `spread` by reducing per-contact helper overhead, RNG setup cost, and repeated data lookups.

### Implementation Changes
1. Add measurement inside the spread kernel before changing behavior.
- Split current `spread` timing into sub-timers for: contact-count draw, contact ID generation, touch screening, infection-risk evaluation, and successful infection updates.
- Record these counters on the same representative large run you used for the 9M-agent benchmark so each later change has a clear delta.

2. Hoist all spreader-invariant work out of the per-contact path.
- Build a small per-spreader context once in `spread`: `thisday`, `spr_variant`, `spr_duration`, `sendrisk`, spreader age/condition, and direct references to the relevant `InfectParams` vectors.
- Stop recomputing `pop.get_variant(spreader)`, `pop.duration[spreader]`, and repeated `infectparams[...]` lookups inside `isinfected`.
- Keep `runsim`’s public call shape stable; make this an internal refactor.

3. Replace the dynamic contact buffer with a fixed-capacity buffer.
- Current contact count is hard-capped at 12 by `gamma_int(..., 12)`, so replace `vector<size_t> contacts` with a fixed buffer plus an explicit count.
- Update `get_contacts`/`how_many_contacts` to fill that fixed buffer directly instead of `clear()` + `push_back()`.
- Treat the buffer size as derived from the current hard cap so it stays synchronized if the cap changes later.

4. Collapse the helper-heavy inner loop into fast paths.
- Inline or move hot helpers into header-visible form only where needed for optimization.
- In the contact loop, first reject non-eligible statuses directly from `pop.status[c]`; only `Unexposed` and `Recovered` should continue.
- For `Unexposed`, skip recovery-immunity work entirely.
- For `Recovered`, compute `recoveffect` only when `recovday[c] > 0`, and use hoisted variant/halflife lookups.
- Remove avoidable `touch_map`, `AgentView`, and repeated branch work from the hot path.

5. Precompute and reuse stable probability inputs.
- Precompute contact-scale lookups for `[condition][age group]` for the current locale/day instead of rebuilding from scalar multiplications in the hot path.
- Precompute touch-probability lookups for the current `indoor_factor`.
- Preserve a general fallback for non-`1.0` indoor factors, but add a dedicated fast path for `indoor_factor == 1.0f`, since that is the current execution path.

6. Optimize RNG setup only after the structural refactor lands.
- Reuse or cache the contact-count gamma machinery for the small set of distinct scales instead of constructing a new gamma distribution for every spreader.
- Replace per-call `std::uniform_int_distribution` setup in contact generation with a reusable bounded-int path.
- Keep this as a second-stage optimization after measurement confirms RNG construction is still material.

7. Trim the successful-infection path.
- Only after a contact passes infection, perform the state mutation.
- If needed, add a `make_sick` overload that accepts the contact index and `thisday` directly to avoid extra `AgentView` construction and repeated `sim::get_day()` calls.
- Do not change epidemiological semantics or history bookkeeping.

### Public Interfaces / Type Changes
- Internal spread helpers should move from `vector<size_t>` contact storage to a fixed-capacity contact buffer type.
- `spread` may gain an internal spreader-context helper, but the external `runsim -> spread(...)` call pattern should remain unchanged.
- `get_contacts` and `how_many_contacts` can be folded into internal hot-path helpers if they no longer justify separate function boundaries.

### Test and Benchmark Plan
- Benchmark the baseline and every optimization stage on the same large simulation input; report total runtime, total spread runtime, and the new substage timings.
- For pure refactors, require exact equality on daily counters such as `starting_spreaders`, `num_contacts`, `num_touched`, and `num_new_infected` with the same seed.
- If RNG implementation changes are introduced, compare aggregate outcomes across repeated runs and require statistical equivalence rather than bitwise-identical trajectories.
- Add edge-case coverage for 0 contacts, max contacts, unexposed vs recovered contacts, recovered contacts with and without recovery history, and preservation of 1-based indexing with index 0 untouched.

### Assumptions
- First pass is behavior-preserving: no approximations, no multithreading, no model-rule changes.
- Focus is limited to `spread.cpp`, `disease_modeling.cpp`, and the RNG/helpers they depend on.
- Scanning the full population each day to find infectious agents is a separate follow-on optimization, not part of this plan.
