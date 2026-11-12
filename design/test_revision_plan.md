# Test Suite Revision — Plan (2026-11-12)

Scope: edits only under `test/` and `test_output/`; notes in `design/`. No `src/` changes.

## Current state

| Item | Finding |
|---|---|
| Build | `test` binary does not compile: `test_disease_modeling`, `test_vaccination`, `test_plot` use removed `Trait::`; `test_runsim` uses removed `HistorySelectorSet("all")` / `serialize_selected_histories`; `test_series` uses removed `create_history_set`, `valid_history_coordinates`, `validate_history_indexing` and 3-field selectors. |
| Harness | `REQUIRE` or any src exception aborts the whole run; `CHECK` prints only expr + line (no values, no test name); no per-group summary. |
| Fixtures | `test_support` name guards restore `names` but not `count` statics; `PopData` now requires `Ring::names` ≥ 2 (default `Ring{1}`), so most hand-built fixtures throw. |
| Coverage shape | Mostly happy-path atomic tests on hand-built fixtures; little on real loaded structures, spread, progression statistics, vaccine *effects*, social distancing, or end-to-end invariants. |
| Speed | Full default case (95,626 people × 180 days) runs in ~0.7 s → a deep tier can run several full scenarios cheaply. |

## Likely source bugs surfaced during review (tests will confirm, not fix)

| ID | Location | Issue |
|---|---|---|
| B1 | `spread.cpp` | `contactfactors[zidx(spr_cond)]`: rows are nil..severe (0..3) but `NIL.v==2` → nil spreader uses the *mild* row; severe spreader reads past the 4-row array. Same for SD-case factors. |
| B2 | `sim.cpp`/`spread`/`infectrisk` | `sendrisk[duration]` with duration up to `DURATIONLIM`(25) but template sendrisk has 25 entries (0..24) → out-of-bounds read on day 25 of illness. |
| B3 | `cases.cpp apply_change` | status `recovered`/`dead` change on a non-infectious person calls `make_well`/`make_dead` unguarded → negative infectious counts and `.at()` throw on variant 0. |
| B4 (TODO, address during vaccination tests) | `vaccination.cpp` | doses issued from `starting_doses × spreadfunc`, never bounded by remaining `doses`; an un-normalized `pattern` over-delivers (doses go negative). `input_verify` does not check `pattern`. |
| B5 | `setup.cpp` | rings path required even when `do_rings=false` (existing TODO). |
| G* | loaders / `input_verify` | validation gaps: sendrisk length, recvrisk length ≠ 5, unknown `filtervec` age silently → UNKNOWN (warning), SD `comply` range, startday>endday. |

## Approach

1. **Harness (test_utils.h / test_main.cpp)** — minimal additions:
   - `run_test(name, fn)` catches `REQUIRE` aborts and any exception → recorded as a failure with test name + `what()`; run continues (no abend on buggy code). SIGSEGV/SIGABRT handler prints the active test name.
   - `CHECK_EQ`, `CHECK_NEAR`, `CHECK_MSG(expr, fmt, ...)` print actual values, test name, and a hint.
   - `KNOWN_BUG(id, expr)`: failing known-issue checks are listed separately and don't fail the exit code; a passing one prints "now passes — remove marker".
   - Per-group pass/fail tally and a final list of failed tests.
2. **Two tiers, no new flags**
   - *Sniff* = `xmake run test` (all groups except `runsim`, `templates`): unit + contract tests + one small headless mini-simulation checked with the invariant checker. Target < 5 s.
   - *Deep* = `xmake run test runsim`: real TUI flow (`setup_dir` → `use_dir` → `runsim` → `post_simulation`) for several scenarios (base, vax, rings, SD, all-on), full invariant checks, golden comparison of terminal summary + total-series CSV, output-file checks, larger statistical samples. Browser launch suppressed via a fake `open` on `PATH` (test-process only).
3. **Macro ↔ unit linkage** — `test/test_invariants.h`: a reusable checker over (`PopData`, `Histories`) run after any simulation. Each invariant prints a pointer to the unit tests that isolate it. Invariants: per-day status conservation; Σvariant(now) = infectious(now); Δnow_dead = new_dead; Δnow_infectious = new_inf − new_recov − new_dead; Σnew_inf = Σ variant_hist.count; end-of-run history = PopData counts (by age, ring); totals = Σ atomic cells; per-person state consistency (cond/duration/days by status); vax histories vs `vaxstatus`.
4. **Per group** (tracking design/test_review.md):
   - `traits`: keep; add bad-constructor and zero-slot tests, `register_trait_name` rules, `tr_val_*` bounds, `PopData` default initialization, name/count consistency after loaders.
   - `parameters`: keep load-error tests; add **contract tests** on loaded structures (variants, progression, vaccines, schedules, rings, social params, SD, seeds) against the requirements of spread/progression/vaccinate; add bad-input-should-throw tests (gaps above).
   - `series`: rewrite. Histories layout/order, meta-to-key agreement for both maps, ring=1 aliasing, `select_indices` (5-field, "all", "total", unknown), `sum_total_histories` against a brute-force total, carry-forward, CSV round-trip.
   - `disease_modeling`: fix to the 5-coordinate total API; add `recoveffect` tests, decay/rise math, `infectrisk`, `touch_map`, spread contact-row selection (B1), statistical progression tests (outcome frequencies), immunity redistribution.
   - `vaccination`: keep delivery tests; add supply totals vs schedule (B4), filter/eligibility, 2nd-shot timing, **effects**: `vaxeffect` vs params by shot/variant, effect on infection and on progression.
   - `social_distancing` (new group name, code placed in an existing .cpp because `xmake.lua` is off-limits): loading/rescaling, tagging on start/end days, compliance rate, effect on spread, seed cases (B3).
   - `setup`: `use_dir` flow on a scaffolded case; Model validity (pop, age apportion, rings, indoor_seq, names); bad config via `setup_sim` throws; `input_verify` exit checked in a forked child.
   - `plot`: Plotly JSON contract (x/y lengths = days, numeric y, labels) and traces equal the selected history vectors; reference HTML artifact for visual check (PNG comparison would need a headless browser — out of scope).
   - `pop_serialize`: golden-file comparison (fixtures in `test/fixtures/pop_serialize/`).
   - `runsim`: as in tier 2.
5. **Testing the tests** — mutation runs on a repo *copy* in the scratchpad (never the project `src/`): inject ~10 representative bugs (index shifts, sign flips, dropped history updates, wrong variant index, supply bound) and confirm each is caught; report the kill matrix.
6. **Deliverables** — revised tests, fixtures, `design/test_suite_revision.md` (changes, assessment, known-bug list, mutation results, how to refresh goldens).
