No code changes made.

The current shape is improved: `r0_sim()` is now thin, `run_r0_sim()` is private to the translation unit, model-member dereferencing is localized in the runner, and density is now derived from `model.mp.geodata` instead of hard-coded. The anonymous-namespace forward declaration at the top is also fine and preserves readability.

Further suggestions, prioritized:

**Correctness**
- `src/r0_simulation.cpp:161-164`: `run_r0_sim()` mutates global simulation time via `sim::incr_day()` and `sim::ds.day`. That is probably the biggest remaining risk. The standalone CLI path works only if `sim::current_day` starts at zero. A second call in the same process, or the unused `r0_sim(PopData, Model&)` being called during `runsim()`, can corrupt or inherit global day state. I would either reset and restore `sim::current_day`/`sim::ds` around the mini-simulation, or make the R0 runner use local day state instead of the global `sim` day.
- `src/r0_simulation.cpp:152` plus `spread.cpp:30`: `indoor_seq` is sized to `DURATIONLIM`, and `spread()` indexes it with `zidx(sim::get_day())`. If `sim::get_day()` is not `1..DURATIONLIM`, this goes out of bounds. This is another reason to isolate/reset/restore simulation day state.
- `src/r0_simulation.cpp:55`: the unused `r0_sim(PopData locdat, Model& model)` overload is risky as currently documented. If `locdat` already contains infectious people, `gen1_spreaders` will include those existing infectious people plus newly seeded people. That may be correct for an `Rt` estimator, but it is not the same measurement as the fresh-population `R0` case. I would not wire this overload into `runsim()` until its semantics are made explicit.
- `src/r0_simulation.cpp:55`: the second overload also copies `PopData` twice: once into the by-value parameter `locdat`, then again into `r0pop`. If this overload survives, prefer one deliberate copy: either take `const PopData& locdat` and copy inside, or take by value and move into `r0pop`.
- `src/r0_simulation.cpp:73`: `bool dovax = model.dovax` does not mirror the guard in `runsim()` that disables vaccination if vaccine definitions/schedules are missing. This may matter more for the second overload, where copied people may already have vaccine statuses. If `r0_sim()` can run against partially configured vaccine data, it should validate or normalize `dovax` consistently with `runsim()`.

**API Surface**
- `src/r0_simulation.h:4`: because the second overload is unused and semantically unfinished, I would consider removing it from the public header until there is a real call site. Keeping it defined privately in the `.cpp` while experimenting is less risky than exposing it as a callable API.
- `src/r0_simulation.h`: add `#pragma once` eventually. Repeated function declarations are legal, so this is not urgent, but this header is currently the odd one out compared with most project headers.
- `src/r0_simulation.cpp:17-19`: the anonymous-namespace forward declaration is fine. I would keep this layout if you prefer public wrappers first.

**Naming And Clarity**
- `src/r0_simulation.cpp:81`: `array<float, 6> trvec{}` is really a mutable scratch probability vector for `progression()`, not the model’s transition vector. I would rename it back to something like `probvec` to avoid implying it comes from `model.mp.trvec`.
- `src/r0_simulation.cpp:33` and `src/r0_simulation.cpp:60`: `Variant use_variant = 1` is correct given the project convention that variant `1` is base, but it is still a magic value. `Variant use_variant{1};` plus a short `base variant` comment would make the intent clearer.
- `src/r0_simulation.cpp:67`: `run_r0_sim()` is a good improvement over `_run_r0_sim`. If this helper is specifically shared by R0 and future Rt, a name like `run_reproduction_sim` may eventually be clearer, but I would not rename again until the second overload’s semantics settle.
- `src/r0_simulation.cpp:75`, `src/r0_simulation.cpp:82`, `src/r0_simulation.cpp:151`: `age_dist`, `popn`, and `still_infectious` appear unused. Removing them later would reduce noise.
- `src/r0_simulation.cpp:150`: `cnt_new_infected` can probably be scoped inside the spreader loop instead of kept function-wide.

**Seeding Logic**
- `src/r0_simulation.cpp:95-100`: the TODO is valid. `array<int, 5>` assumes `AGE_DIST.size() == 5`. That matches current age groups, but the function will silently become wrong if age groups change.
- `src/r0_simulation.cpp:98`: `std::ranges::min(AGE_DIST)` is recomputed in each loop iteration. Trivial cost, but cleaner to bind once before the loop.
- `src/r0_simulation.cpp:98`: the implicit `double -> int` truncation is probably acceptable if intentional, but I would make the rounding policy explicit eventually. The comment already flags this.
- `src/r0_simulation.cpp:103-135`: after all seed counts hit zero, the loop still scans the whole population. Not a correctness issue, but with a 200k synthetic pop it is easy to stop early once the remaining seed count is zero.

**Simulation Semantics**
- `src/r0_simulation.cpp:152`: for abstract R0, `indoor_seq` fixed at `1.0f` is reasonable. For the second `Rt`-style overload, it probably should not be fixed; it should either use the current simulation’s seasonal/indoor context or explicitly document that this is an Rt-like calculation with seasonality/social-distancing/rings intentionally suppressed.
- `src/r0_simulation.cpp:153-156`: empty social-distancing and ring structures are fine for pure R0. They are probably wrong or at least incomplete for the commented future “r at time t” behavior.
- `src/r0_simulation.cpp:79`: the comment says `r0series` is “not used but required by the spread API.” It is not used as an output, but it is mutated by `make_sick()` and `progression()`. A more precise comment would be “scratch series required by spread/progression APIs.”

**Validation**
- For the current standalone path, the important validation is `xmake build epi_sim` and one `--r0-sim` run against a known case.
- If you touch sim-day handling, validate repeated calls in one process if there is a convenient harness, because that is the failure mode most likely to expose the current global-state coupling.
- If you decide to keep the second overload, it deserves a small targeted test or scratch harness before being called from `runsim()`, because it currently has materially different semantics from the fresh-population R0 calculation.

**Accuracy of statistical estimate**
- Accept an iteration count for R0 simulation.
- For each iteration, create a fresh synthetic population and run the R0 mini-simulation independently.
- Report the mean R0 estimate across iterations.
- Also report at least standard deviation or standard error, because single runs vary widely and the mean alone hides Monte Carlo uncertainty.
- Ensure RNG state differs across iterations; do not reseed each iteration with the same fixed seed.