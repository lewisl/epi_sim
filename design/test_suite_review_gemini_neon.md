# Test Suite Review & Evaluation (`test/` at `9c4e936`)

> [!NOTE]
> Refreshed strictly read-only evaluation against `HEAD` (`9c4e936`, after the test-suite revision and B1–B5 / G1–G5 / brand-mixing commits). No source or test files were modified.

## Executive Summary

The overhaul in `2aafb6b..9c4e936` transformed `test/` from a broken unit-test collection into a **well-engineered, fast, result-aware two-tier suite** (~5,100 lines; 1,760 sniff checks in ~0.3s, 195 deep checks in ~5s, 192 template checks; 18/18 mutation kills; 0 open `KNOWN_BUG` markers).

**Does the suite now catch issues in the results the code produces?**
**Substantially yes for single-variant (`base`) accounting, transition/indexing bugs, and intervention mechanics—far better than low-level unit tests alone.** The 9 whole-run invariants in [test_invariants.h](file:///Users/lewislevin/code/epi_sim/test/test_invariants.h), statistical kernel checks, paired SD control run, and 5 full `runsim` scenarios with CSV/plot/golden verification catch most silent data-corruption bugs.

**However, five meaningful result-level blind spots remain:**
1. **`r0_simulation.cpp` still has 0% coverage—and still contains live bugs** (an out-of-bounds read + dead-spreader resurrection bug in `run_r0_sim`, plus a final-day assert and `indoor_seq` off-by-one in `rt_sim`).
2. **Multi-variant dynamics are never exercised in any simulation test:** All 5 `runsim` scenarios and both sniff macro runs seed only `"base"`, leaving cross-variant immunity, derived `sendrisk`/`riskadjust`, and multi-variant `Histories` untested end-to-end.
3. **In-process multi-case isolation is untested:** Every test restores static registries via `RuntimeNamesGuard`, so no test verifies that running `all_on -> base -> all_on` back-to-back in one process produces identical results without static holdover.
4. **Cross-scenario behavioral guardrails are missing in `runsim`:** Exact goldens catch drift, but when goldens are refreshed after intentional changes (as happened 3 times in `SESSION.md`), no semantic assertions verify that `vax` and `sd` still reduce infections/deaths vs. `base` or that older ages still have higher CFR.
5. **Several minor-to-moderate code-quality and edge-case gaps** persist in `test_invariants.h`, `test_runsim.cpp`, `test_parameters.cpp`, `test_vaccination.cpp`, and `test_templates.cpp`.

---

## 1. What the Revised Suite Does Well

- **Clean two-tier execution ([test_main.cpp](file:///Users/lewislevin/code/epi_sim/test/test_main.cpp)):** Ordered bottom-up (`traits` → `parameters` → `series` → `disease_modeling` → `social_distancing` → `vaccination` → `setup` → `plot` → `pop_serialize`), with deep 180-day `runsim` and `~/.config`-touching `templates` cleanly separated.
- **Solid harness ([test_utils.h](file:///Users/lewislevin/code/epi_sim/test/test_utils.h), [test_support.h](file:///Users/lewislevin/code/epi_sim/test/test_support.h)):** Per-test exception/abort isolation (`run_test`), signal crash handler, value-printing macros (`CHECK_EQ`/`CHECK_NEAR`/`CHECK_MSG`/`CHECK_THROWS`), `RuntimeNamesGuard` saving/restoring both `::names` and `::count`, `suppress_browser()` via no-op `open` on `PATH`, forked `require_inputs_verify`, and `compare_golden` with `actual_*` output on mismatch.
- **Strong whole-run accounting ([test_invariants.h](file:///Users/lewislevin/code/epi_sim/test/test_invariants.h)):** Every simulated run checks daily population conservation, `sum(now_variant) == now_infectious`, `now_dead`/`now_infectious` flow balance, non-negativity, cumulative event agreement with `PopData`, final `(status|variant, age, ring)` cell agreement with `PopData`, vaccination history alignment, brute-force verification of every total/split-total vector, and per-person state consistency.
- **Real kernel & behavioral checks:**
  - [test_disease_modeling.cpp](file:///Users/lewislevin/code/epi_sim/test/test_disease_modeling.cpp): Exact formula checks for `sigdecay`, `effect_rise`, `recoveffect`, `infectrisk`; targeted cell-zeroing tests proving `spread` reads `contactfactors[cond-2][age-1]`, `touchfactors`, SD overrides, and `out_ring_prob` 0/1; statistical progression frequencies ($N=20{,}000$) and immunity redistribution on breakday 5 and day 25.
  - [test_social_distancing.cpp](file:///Users/lewislevin/code/epi_sim/test/test_social_distancing.cpp): Paired 10,000-person control vs. SD simulation confirming identical trajectories before start day and >50% fewer infections after; runtime seed-case eligibility, counting, and end-of-day pass.
  - [test_vaccination.cpp](file:///Users/lewislevin/code/epi_sim/test/test_vaccination.cpp): Multi-day curve integration, stock cap (B4 fixed), 2nd-shot brand mixing (`allow2ndshot`), per-brand daily allotment, alternate fallback, exact `vaxeffect` formula, and statistical protection in both `isinfected` and `progression`.
  - [test_parameters.cpp](file:///Users/lewislevin/code/epi_sim/test/test_parameters.cpp): Data-driven contract tests derived from template JSON (resistant to recalibration, sensitive to index/layout bugs).
  - [test_runsim.cpp](file:///Users/lewislevin/code/epi_sim/test/test_runsim.cpp): Exercises `setup_dir → input_verify → use_dir → runsim → post_simulation` across 5 scenarios (`base`, `rings`, `vax`, `sd`, `all_on`) with in-window vaccination schedules (`30..150`) and verifies CSVs and Plotly traces against in-memory state.

---

## 2. Updated Coverage Matrix (`src/` vs `test/` at `9c4e936`)

| Production Module | Current Coverage | Strengths | Remaining Gaps |
|---|---|---|---|
| `traits.*`, `population.h` | **Strong** | Wrappers, fixed/runtime bounds, `register_trait_name` (incl. 255 cap), `tr_val_*`, `PopData` init & `AgentView` bounds | Minor: `validate_fixed_trait_names` failure paths; `PopData::apportion` tiny-`n` edge cases |
| `parameters.*`, `input_verify.*` | **Strong** | Schema/error checks + template-driven contract tests for variants, progression, vaccines, schedules, rings, social params, SD, seeds, `allow2ndshot`, `pattern` | `recovery_immunity` completeness (known unfixed gap, `SESSION.md` 2026-11-14); `vaxhalflifeadjust` & `shotmode` unused in sim |
| `series.*` | **Strong** | Cardinalities, sim-map bijection/order, multi-ring & 1-ring alias totals, seeding, carry-forward, brute-force `sum_total_histories`, `select_indices`, CSV round-trip | `print_selected_histories` untested; `Histories(0, pop)` edge case (uninitialized count members) |
| `disease_modeling.*`, `progression.*` | **Strong** | Transitions, overflow, exact decay/rise/`recoveffect`/`infectrisk`, `touch_map`, statistical progression & immunity redistribution | `recoveffect` `thisday < recovday` unsigned underflow ([disease_modeling.cpp:171-173](file:///Users/lewislevin/code/epi_sim/src/disease_modeling.cpp#L171-L173)); `lindecay`/`expdecay` (if used) |
| `spread.*` | **Good** | Condition row, age col, touch rows, SD factors, `out_ring_prob` 0/1 | `indoor_uplift > 1.0` & `density_factor != 1.0` effect on spread (always `1.0` in tests); multi-ring (>2 rings) out-of-ring sieve |
| `cases.*` | **Good** | Seed validation/filtering/counting/EOD pass; SD loading/rescaling/tagging/clearing & whole-run effect | Overlapping SD cases where case 2 overwrites case 1 before case 1 expires; `apply_change` non-seed paths |
| `vaccination.*` | **Strong** | Daily supply, eligibility, delays, 2nd-shot brand mixing, alternates, curve integration, stock cap, `vaxeffect` formula, infection & severity protection | `Vaxstat::exhausted` never passed to `vaxeffect` in `test_vaxeffect_follows_parameters` (row created at [L427](file:///Users/lewislevin/code/epi_sim/test/test_vaccination.cpp#L427) but unasserted); `pct2ndshot`/`pctboost` fractional stochastic rates |
| `setup.*` | **Good** | `use_dir` base & all-on, age apportionment, indoor season dates, ring partition & stratification, `rings` config rules, 2 short sniff runs | Sequential multi-case reload isolation without `RuntimeNamesGuard` |
| `plot.*` | **Good** | HTML rendering, parseable Plotly JSON, 5-field `historyplot`, stacking, annotation text, empty selection | `post_simulation` `dovax` plot branch ([sim.cpp:283-292](file:///Users/lewislevin/code/epi_sim/src/sim.cpp#L283-L292)) has vaccinated trace commented out |
| `pop_serialize.*` | **Good** | Goldens (pretty, multi-value, all-column CSV), row order, escaping, bad-input throws, per-column cell checks | Full-sim `_pop_.csv` in `test_runsim` checks `status` column only (not `vax`, `variant`, `ring`, `sdcase`) |
| `sim.*` | **Good (single-variant)** | 5 deep scenarios + 2 sniff runs + 9 invariants + CSV/plot reconciliation + goldens | **No multi-variant simulation**; no cross-scenario comparative assertions; `_selected_series_.csv` content & `_all_series_.csv` values unchecked; `rt_sim_interval > 0` never exercised |
| **`r0_simulation.*`** | **Zero (0%)** | None | `r0_sim`, `rt_sim`, `seed_gen1`, `spread_and_count`, `sim::current_day` restoration — **contains live bugs** |
| `helpers.*`, `random.h` | **Minimal** | Indirect only | `sanitize_filename_component` lost its only test in `test_runsim` rewrite; `shifter` constant-array edge case; `xo::gamma_int`/`categorical_fast` |
| `param_init.*`, `template.*` | **Good** | Scaffolding, project dir preservation, managed case run | Mutates real `~/.config/epi_sim/project-dir.toml` and `$HOME` |
| `tui_app.*`, `tui_commands.*`, `epi_sim.cpp` | **Zero (0%)** | None | CLI flag parsing; TUI sequential multi-case runs |

---

## 3. Remaining Result-Level Blind Spots & Live `src/` Defects

### A. Concrete Live Bugs Still Present in Untested `src/r0_simulation.cpp`

> [!WARNING]
> Because `r0_simulation.cpp` has **0% test coverage**, three concrete bugs remain live at `9c4e936`:

1. **Out-of-bounds read + dead-spreader resurrection in `run_r0_sim` ([src/r0_simulation.cpp:243-247](file:///Users/lewislevin/code/epi_sim/src/r0_simulation.cpp#L243-L247)):**
   Commit `2aafb6b` added `if (person.status() != INFECTIOUS) continue;` to the `spread` loop at [L232](file:///Users/lewislevin/code/epi_sim/src/r0_simulation.cpp#L232), **but not to the `progression` loop right below it**:
   ```cpp
   // daily progression for spreaders: to get better or die
   for (size_t p : gen1_spreaders) {
     auto person = r0pop.agent(p);
     progression(person, r0_histories, progressionset, infectparams, dovax, vaxset);
   }
   ```
   - When a gen1 spreader recovers (`cond = UNINFECTED = 1`, `duration = 0`), `progression()` keeps incrementing `duration` until day 5 (a breakday), where [progression.cpp:94](file:///Users/lewislevin/code/epi_sim/src/progression.cpp#L94) indexes `tree.entries[entry_idx][person.cond() - uint8_t{2}]` → index **`-1` (out-of-bounds read / UB)**.
   - When a gen1 spreader dies (`status = DEAD`, `cond` unchanged), `progression()` keeps advancing their duration and transitions them at the next breakday—potentially setting `cond` or calling `make_well()` (resurrection), and if a recovered spreader lands on an active condition via OOB read or dead spreader stays infectious, it corrupts the computed $R_0$.
2. **`rt_sim` assertion abort on final day ([src/r0_simulation.cpp:83](file:///Users/lewislevin/code/epi_sim/src/r0_simulation.cpp#L83)):**
   `assert(saved_day < model.indoor_seq.size());` fires whenever `rt_sim_interval` divides `model.ndays` (e.g., `ndays = 180, rt_sim_interval = 30` → on day 180, `saved_day == 180 == indoor_seq.size()`).
3. **`rt_sim` `indoor_seq` off-by-one ([src/r0_simulation.cpp:88](file:///Users/lewislevin/code/epi_sim/src/r0_simulation.cpp#L88)):**
   `saved_day` is 1-based while `model.indoor_seq` is 0-based (`indoor_seq[zidx(thisday)]` in [spread.cpp:28](file:///Users/lewislevin/code/epi_sim/src/spread.cpp#L28)), so `model.indoor_seq.begin() + saved_day` slices starting from **day `saved_day + 1`** instead of `saved_day - 1`.

### B. Result-Level Gaps Across the Simulation Suite

1. **Zero multi-variant simulation coverage:**
   Every full-simulation test (`setup/short_runs_hold_invariants` and all 5 `runsim` scenarios) uses the default `seed.json`, which seeds only `"base"`. As a result:
   - `alpha`, `delta`, and `omicron_*` never infect a single person in any test run.
   - Cross-variant reinfection (`recovery_immunity[v1][v2]`), derived variant transmissibility (`basemultiplier`), derived severity (`riskadjust`), multi-variant vaccine effectiveness, and multi-variant `Histories` columns (`now_variant` / `new_variant` for `v >= 2`) are never exercised together in an actual run.
   - Invariant 2 (`sum(now_variant) == now_infectious`) is only ever stressed with 1 active variant.
2. **Zero cross-run / sequential-case isolation tests:**
   Because `RuntimeNamesGuard` wraps every test, no test verifies that running `all_on` (vax + 2 rings + SD) immediately followed by `base` (no vax, 1 ring, no SD) in the same process—without any test guard resetting globals—produces the exact same `base` output as a clean process.
3. **Golden-only verification without cross-scenario directional checks in `runsim`:**
   The 5 golden summaries show clear epidemiological structure (`base`: 72,124 infected / 776 dead / 1.08%; `vax`: 21,574 / 40 / 0.19%; `sd`: 42,915 / 318 / 0.74%; `all_on`: 4,843 / 34 / 0.70%; `age80_up` CFR 15.08% vs `age0_19` 0.04%), **but `test_runsim.cpp` does not assert any of these directional relationships**. Whenever goldens are regenerated after a model change, explicit comparative assertions (`vax` < `base`, `sd` < `base`, age-CFR ordering, `ring_2 == 0` when `out_ring_prob == 0`) act as a safety net against accidentally blessing a broken golden.
4. **Silent parameter / feature traps:**
   - `recovery_immunity` completeness: missing or typo'd variant names silently default to `0.0f` (`SESSION.md` 2026-11-14; [test_parameters.cpp:849](file:///Users/lewislevin/code/epi_sim/test/test_parameters.cpp#L849) currently allows missing keys).
   - `progression_factors.vaxhalflifeadjust` and `VaxSched::shotmode` are parsed from JSON and present in user templates but completely ignored at runtime—no test surfaces this or guards against accidental reliance.
   - `indoor_uplift` and `density_factor` are both fixed at `1.0f` in all `spread` tests and never varied in a paired simulation test.
   - `recoveffect` ([disease_modeling.cpp:171-173](file:///Users/lewislevin/code/epi_sim/src/disease_modeling.cpp#L171-L173)) still has the unsigned `size_t days_post_recov >= 0` check (never tested with `thisday < recovday`).

---

## 4. Test Code & Harness Quality Audit (Specific Findings)

| File & Location | Issue | Severity |
|---|---|---|
| [test_invariants.h:79](file:///Users/lewislevin/code/epi_sim/test/test_invariants.h#L79) | `now_infectious` flow skips `d == 1` (`if (d > 1 && ...)`), so day-1 seeding flow isn't checked. | Low |
| [test_invariants.h:150-169](file:///Users/lewislevin/code/epi_sim/test/test_invariants.h#L150-L169) | Vaccination history invariant checks global brand sums only (not per `(ring, age)` like status/variant in invariant 6), and assumes `vax_hist` never overflows 16 shots (`arr[0]`). | Low–Med |
| [test_runsim.cpp:76, 78-89](file:///Users/lewislevin/code/epi_sim/test/test_runsim.cpp#L76-L89) | `_selected_series_.csv` is only checked for existence; `_all_series_.csv` checks dimensions only; `_pop_.csv` checks only the `status` column (not `agegrp`, `ring`, `vax`, `variant`, `sdcase`). | Medium |
| [test_runsim.cpp:97](file:///Users/lewislevin/code/epi_sim/test/test_runsim.cpp#L97) | `text.find("const data = ") + 13` does not check `!= string::npos` first (wraps on missing token instead of failing `REQUIRE`). Also [L62-L67](file:///Users/lewislevin/code/epi_sim/test/test_runsim.cpp#L62-L67) re-splits every CSV row once per column. | Low |
| [test_disease_modeling.cpp:399-406](file:///Users/lewislevin/code/epi_sim/test/test_disease_modeling.cpp#L399-L406) | `if (q.status() == INFECTIOUS)` silently skips the non-breakday duration-advance check if agent 1 happened to recover/die; should locate an infectious agent or `REQUIRE`. | Low |
| [test_vaccination.cpp:427, 436-460](file:///Users/lewislevin/code/epi_sim/test/test_vaccination.cpp#L427-L460) | `make_effect_vaxset` populates the `Vaxstat::exhausted` zero row, but `test_vaxeffect_follows_parameters` never calls `at(Vaxstat::exhausted, ...)` to confirm `1.0`. | Low |
| [test_parameters.cpp:451-454, 794-797](file:///Users/lewislevin/code/epi_sim/test/test_parameters.cpp#L451-L454) | Unguarded `fs::current_path(case_dir)` around `expect_throws_containing` (an unexpected non-`runtime_error` exception leaves CWD in a deleted temp dir). Also stale empty comment stubs at [L258-L261](file:///Users/lewislevin/code/epi_sim/test/test_parameters.cpp#L258-L261) and [L301-L307](file:///Users/lewislevin/code/epi_sim/test/test_parameters.cpp#L301-L307), and mixed old `CHECK`/`expect_throws_containing` vs. new `CHECK_EQ`/`CHECK_THROWS` (same mix in [test_traits.cpp](file:///Users/lewislevin/code/epi_sim/test/test_traits.cpp)). | Low |
| [test_templates.cpp:36-56, 149-151](file:///Users/lewislevin/code/epi_sim/test/test_templates.cpp#L36-L56) | Duplicates `child_exit_code` (already in [test_support.h:326](file:///Users/lewislevin/code/epi_sim/test/test_support.h#L326)); creates temp dirs directly in `$HOME` rather than `test_output/tmp`; mutates real `~/.config/epi_sim/project-dir.toml`. | Low |
| [test_series.cpp](file:///Users/lewislevin/code/epi_sim/test/test_series.cpp) / `helpers.cpp` | `print_selected_histories` is untested, and `sanitize_filename_component` lost its only test when `test_runsim.cpp` was rewritten. | Low |

---

## 5. Proposed Prioritized Plan (If You Want to Close These Gaps)

No edits will be made until you approve scope. Recommended order of impact:

### Priority 1 — Fix & Test `r0_simulation.cpp` (Highest Risk / Live UB)
- **Production fixes (require explicit approval):**
  - Guard the `gen1_spreaders` progression loop in `run_r0_sim` ([r0_simulation.cpp:244](file:///Users/lewislevin/code/epi_sim/src/r0_simulation.cpp#L244)) with `if (person.status() != INFECTIOUS) continue;`.
  - Fix `rt_sim`'s `indoor_seq` 0-based offset (`saved_day - 1`) and allow `saved_day == model.ndays` cleanly ([r0_simulation.cpp:83-92](file:///Users/lewislevin/code/epi_sim/src/r0_simulation.cpp#L83-L92)).
- **Tests (sniff tier, fast small-pop / targeted):**
  - Test `r0_sim`: returns finite $R_0 > 0$ on template params, $R_0 = 0$ when `sendrisk = 0`, increases monotonically with `basemultiplier`, preserves `sim::current_day`, and triggers no OOB/resurrection under ASan/UBSan or state inspection.
  - Test `rt_sim` via `runsim` with `rt_sim_interval > 0` (including dividing `ndays`, e.g., `days = 60, rt_sim_interval = 30`): completes cleanly, restores `sim::current_day`, and produces $R_t < R_0$ once immunity/SD builds.

### Priority 2 — High-Value Result-Level Tests (Closing Remaining Simulation Blind Spots)
- **Multi-variant mini-simulation (sniff tier, ~8k pop, ~120 days):** Seed `"base"` on day 1 and `"omicron_ba4_5"` (or `"delta"`) mid-run with `dovax = true` and `do_rings = true`. Verify `invariants::check_run` passes, both variants register nonzero `new_variant`/`now_variant` trajectories, cross-variant reinfections occur, and the more transmissible variant grows faster.
- **Sequential in-process case isolation (sniff tier):** Run `all_on` → `base` → `all_on` in one test *without* an intervening `RuntimeNamesGuard` (outer guard only), asserting that `base` has `Vax::count == 0`, `Ring::count == 1`, and matches a fresh `base` run identically.
- **Cross-scenario behavioral assertions in `test_runsim.cpp` (deep tier):** Beyond exact goldens, explicitly assert `vax` reduces infections & deaths vs. `base`, `sd` reduces infections vs. `base`, and `age80_up` CFR > `age0_19` CFR—so future golden refreshes cannot silently accept a broken intervention.
- **Targeted `spread` & `vaccination` edge tests:** `indoor_uplift > 1` & `density_factor > 1` increase infections in `spread`; `Vaxstat::exhausted` returns `1.0` in `vaxeffect`; multi-ring (3-ring) sieve check; full-run `out_ring_prob == 0` keeps `ring_2` at zero infections.

### Priority 3 — Test Hygiene & Minor Coverage Cleanups
- Tighten `test_invariants.h` (day-1 `now_infectious` flow, per-`(ring, age)` vax history check, `now_unexposed` monotonicity).
- Add safe `npos` check and `_selected_series_.csv` / `_pop_.csv` multi-column checks in `test_runsim.cpp`.
- Add direct tests for `print_selected_histories`, `sanitize_filename_component`, and `recoveffect` `thisday < recovday` (plus fixing the unsigned `>= 0` guard in `disease_modeling.cpp` if approved).
- Clean up `test_disease_modeling.cpp:400`, RAII CWD guard + stale comments in `test_parameters.cpp`, and duplicate `child_exit_code` in `test_templates.cpp`.

---

## Open Questions

> [!IMPORTANT]
> Please let me know which items (if any) you'd like to act on:

1. **`r0_simulation.cpp` bugs:** Do you want to fix the `run_r0_sim` progression guard ([L244](file:///Users/lewislevin/code/epi_sim/src/r0_simulation.cpp#L244)) and `rt_sim` final-day/indexing issues ([L83-L88](file:///Users/lewislevin/code/epi_sim/src/r0_simulation.cpp#L83-L88)), and add $R_0$/$R_t$ tests?
2. **Priority 2 result-level tests:** Would you like to add the multi-variant mini-sim, sequential multi-case isolation test, and cross-scenario comparative checks in `test_runsim.cpp`?
3. **`recovery_immunity` completeness (`SESSION.md` 2026-11-14) & unused parameters (`vaxhalflifeadjust`, `shotmode`):** Should `recovery_immunity` completeness be bundled into this pass or handled separately? And are `vaxhalflifeadjust` / `shotmode` intentionally deferred features?
