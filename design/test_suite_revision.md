# Test Suite Revision — Summary (2026-11-12)

Plan: `design/test_revision_plan.md`. Guidance: `design/test_review.md`.
Edits under `test/` (and `test_output/` artifacts). The source fixes listed in §3 were made
only with explicit approval during the review.

## 1. How to run

| Command | Tier | Time | What it runs |
|---|---|---|---|
| `xmake run test` | **sniff** | ~0.3 s | traits, parameters, series, disease_modeling, social_distancing, vaccination, setup, plot, pop_serialize (incl. two short headless simulations checked with the run invariants) |
| `xmake run test runsim` | **deep** | ~5 s | 5 full 180-day scenarios through `setup_dir → use_dir → runsim → post_simulation`, output-file checks, invariants, goldens |
| `xmake run test templates` | explicit | ~1 s | project/case directory handling; touches `~/.config/epi_sim/project-dir.toml` (saved and restored) |
| `… <group> [<group>…] --artifacts` | any | | also writes artifacts to `test_output/<group>/` and keeps temp case dirs |

- Exit code is non-zero only for real failures. Known source bugs are listed separately (§3).
- No browser opens: `test_main` puts a no-op `open` first on the test process's `PATH`.
- **Goldens** live in `test/fixtures/{pop_serialize,runsim}/`. On a mismatch the test prints the first differing line and writes `test_output/<group>/actual_<file>`. After an *intended* model change, review that file and copy it over the fixture. The runsim goldens reflect the B1/B2 fixes (template base case: 864 dead).
- Deep-tier case directories (inputs, outputs, `terminal.txt`) are kept in `test_output/runsim/<scenario>/` for inspection.

## 2. What changed

### Harness (`test_utils.h`, `test_main.cpp`)
- `run_test(name, fn)`: a failed `REQUIRE`, or any exception from source code, is recorded against that test and the run continues. A crash handler prints the active group and test.
- `CHECK_EQ`, `CHECK_NEAR`, `CHECK_MSG`, `CHECK_THROWS(type, stmt, substring)` print the actual values and context.
- `KNOWN_BUG(id, expr, msg)`: listed in a separate section and not counted in the exit code. A known-bug check that starts passing is reported as "now passes — remove the marker".
- A per-group summary and a list of failed tests at the end. Several groups can be selected on one command line.

### Support (`test_support.h`, new `test_invariants.h`)
- `RuntimeNamesGuard` restores the names **and counts** of Variant, Vax, Ring and SDCase. The old guards restored names only. Histories layout depends on the counts.
- `install_names(...)` installs runtime names the same way the loaders do. `make_pop(n, rings)` creates a population; `PopData` now requires a Ring registry.
- Case helpers:
  - `make_case_dir` scaffolds a case with `setup_dir`.
  - `use_small_locale` adds a small-population locale to `geodata.csv` and selects it.
  - `move_vax_schedules` shifts schedule day ranges.
  - `build_small_case` builds the model through `use_dir`.
- `require_inputs_verify` runs `input_verify` in a forked child, so a bad scaffold fails one test rather than ending the run.
- `StdoutCapture` (quiet runs; terminal goldens), `compare_golden`, `suppress_browser`.
- **`invariants::check_run(pop, histories, label)`**: nine whole-run checks. Each failure prints the violation count, the first example, and the unit tests that isolate the cause:
  1. Status conservation each day.
  2. Σ variant(now) = infectious(now).
  3. Δnow_dead = new_dead, and Δnow_infectious = new_inf − new_recov − new_dead.
  4. No negative counts, and new_unexposed is never written.
  5. Σ new events = per-person event counts (variant_hist, recovday_hist, dead).
  6. Final-day histories = PopData counts by (status|variant, age, ring).
  7. Vaccination histories = first shots by brand, and vaxstatus agrees with vax_hist.
  8. Every total vector = brute-force sum of its atomic cells (driven by the metadata).
  9. Per-person state consistency by status.

### Groups

| Group | Before | Now |
|---|---|---|
| traits | name/lookup round trips | kept; added PopData initial state (index 0 unused, defaults, age apportionment), Ring registry required, AgentView bounds, `register_trait_name` rules (zero slot, empty, duplicate, case duplicate, reserved, 255 limit), `tr_val_*` bounds, `trait_table` references the runtime names, loaders keep names and counts consistent |
| parameters | load-error and input_verify tests | kept (fixed 4 tests that read the *mild* row as nil, and one with no config.json); added **contract tests** for variants, progression trees, vaccines, schedules, rings and social params against what spread/progression/vaccinate index into; SD case loading and rescaling; seed loading and 11 malformed-seed rejections; new sendrisk-length rule |
| series | targeted removed APIs; did not compile | rewritten: layout counts over 1–3 variants × 0–2 vaccines × 1–3 rings; sim-map bijection, metadata and storage order; totals-map metadata (multi-ring) and aliasing (one ring); day-1 seeding by ring and age; carry-forward of now, not new_; `sum_total_histories` against a brute-force total; `select_indices` with 5-field selectors, "all", "total", missing key, unknown name; CSV round trip; printing and enumeration |
| disease_modeling | used removed `Trait::`; did not compile | transitions checked through the 5-coordinate API (trait, phase, value, ring, age) and totals; decay/rise math; **recoveffect** (formula points, matrix orientation, waning, bounds); infectrisk indexing and clamp; touch_map; **spread** contact-factor row, age column, touch rows, SD factors, ring selection; **progression** outcome frequencies (N = 20,000) and immunity redistribution (breakday and terminal day) |
| social_distancing | none | new group: tagging on start/end days (eligibility, comply rate, include_ages, overlapping cases, default endday); whole-run SD vs control (identical before the start day, fewer infections after); seed cases at runtime (counts only people actually infected, recovered eligible, dead skipped, filter AND, change terms applied) |
| vaccination | did not compile | kept and fixed the delivery tests; added eligibility (filter, infectious, dead, recent recovery), 2nd-shot delay and the post-schedule window, booster delay, brand mix, alternates, **delivery over a real pattern equals the daily allotments ≈ targetpct × supply**, supply never exceeded (B4); **effects**: `vaxeffect` against the formula by day, shot and variant, protection against infection, reduced severity in progression, the `dovax` gate |
| setup | setup_sim on template paths | `use_dir` on a scaffolded case: Model fields, age apportionment, indoor season, names, seeds; rings/vax/SD switched on (ring partition, per-age ring proportions); bad config rejected; input_verify error log; **sniff macro test**: base and all-on 120-day runs pass all invariants |
| plot | used removed `Trait::`; did not compile | token rendering; written HTML has parseable data and layout; **historyplot traces are exactly the selected vectors** (x = dates, one per day; y = vector values; names = labels; stackgroup; summary annotation); an invalid selection writes nothing. Reference HTML written with `--artifacts` |
| pop_serialize | exact-line asserts | golden files for pretty, multi-value and all-column CSV output (reviewed by hand); row selection and order; CSV escaping; bad rows and columns rejected; per-column cell rendering |
| runsim | artificial fixture, did not compile | deep tier as described in §1: 5 scenarios (base, rings, vax, sd, all_on); totals CSV equals histories; all-series column count; population CSV status counts; 4 plots whose cumulative traces end at PopData counts; seed lines in terminal output; goldens |
| templates | scaffolding and project directory | kept; child processes no longer return into the runner; added the managed-case path (`init_case` → `use_managed_case` → run) |

Size: 5,126 test lines (+2,940 / −2,106); fixtures 272 KB.

## 3. Source bugs

### Fixed during the review (approved)

| ID | Fix | Effect |
|---|---|---|
| B1 | `contactfactors[spr_cond - 2]` in `spread.cpp` and `r0_simulation.cpp` | Mild spreaders had used the sick row, sick the severe row, and severe read out of bounds. Template case: 70,322 → 80,813 infected; 847 → 864 dead. |
| B2 | `sendrisk` virtual 1-indexed: exactly `DURATIONLIM + 1` (26) values, checked by the loader and `input_verify` | Removes the out-of-bounds read on illness day 25 (it always read 0, so results were unchanged). Templates were updated by the user. |
| B3 | `SeedCase` constructor enforces the seed rules: infectious only, real variant, cond nil..severe, duration 1..25, no duplicate terms, count ≥ 1, triggerday ≥ 1 | Malformed seeds are rejected with messages. |
| B5 | `setup_model_params` requires a rings path only when `do_rings` is true | With `do_rings=false` the rings file is optional and never read (template present, unreadable, no tag, blank tag, or `{"rings": []}`); ring 1 = population. `do_rings=true` without ring parameters is still rejected. Covered by `setup/rings_config_rules`. |
| G1–G4 | Checks added in **both** `input_verify` (short messages that point at the parameter file) and the loaders (a backstop for direct use): `recvrisk` has 5 values; every `filtervec` entry is an age group, with `unknown` rejected too; SD `comply` is in [0, 1]; SD `1 <= startday < endday` (`startday == endday` never cleared, because tagging and clearing are if/else-if) | Tested in `parameters/*` for both layers. The user's cases pass. |
| G5 | `runsim` now runs end-of-day seeds after spread and progression; the report line reads "Seed day N (end of day) count: …" | `social_distancing/end_of_day_seeds_run` |
| — | `SeedCase::operator()` counts only people it actually infects | Mid-run seeds reach `count` even when early matches are already infected or dead. |

### Known, still present (listed by the runner, not failures)

| ID | Where | Issue |
|---|---|---|
| B4 | `vaccination.cpp` | Daily doses = `floor(spreadfunc × starting_doses)`, never capped by remaining doses. A pattern with area 3 delivered 3,000 shots from a supply of 1,000. `input_verify` does not check `pattern`. Deferred to the vaccination work. |

Not marked in the tests: B9, where every vaccine prints 6 "exhausted … missing" warnings on each `dovax` load (the status is never assigned). `input_verify.h` says it exits the process, but it throws. In the pretty view, ring names are cut to 4 characters (`popu`), and `deadday`/`quarday` show `0` where other day columns show `-`.

## 4. Testing the tests

**First run of the new suite.** The diagnostics found 6 test-side defects, all fixed:
- Old progression tests read the *mild* row as nil (`zidx(NIL)`): the same mix-up as B1, in test code.
- A duplicate-key test never wrote a `config.json`.
- Dangling references in `CHECK_EQ`, and later in `CHECK_THROWS`: an expected message built with `fmt::format` was kept as a `string_view`.
- JSON was printed through its implicit numeric conversion.
- Forked children returned into the runner.
- One guessed expectation was wrong.

**No abends.** Every one of these surfaced as a named failure while the run continued. That includes running the suite from a directory outside `HOME`, where five setup tests reported the exception and the run finished.

**Mutation testing** on a scratchpad copy of the repo (`src/` in the project untouched). Each injected bug was rebuilt and both tiers were run:

| ID | Injected bug | Result | Sniff tier failures | Deep tier failures |
|---|---|---|---|---|
| M01 | spread: contact-factor row off by one (reintroduce B1) | killed (sniff) | disease_modeling/spread_uses_spreader_condition_row | runsim/scenario_base, runsim/scenario_rings, runsim/scenario_vax, runsim/scenario_sd, runsim/scenario_all_on |
| M02 | make_well: drop the now-variant decrement | killed (sniff) | disease_modeling/state_transitions_update_person_and_histories, social_distancing/sd_effect_on_simulation, setup/short_runs_hold_invariants | runsim/scenario_base, runsim/scenario_rings, runsim/scenario_vax, runsim/scenario_sd, runsim/scenario_all_on |
| M03 | make_sick: reinfection does not leave now_recovered | killed (sniff) | disease_modeling/state_transitions_update_person_and_histories, social_distancing/sd_effect_on_simulation, setup/short_runs_hold_invariants | runsim/scenario_base, runsim/scenario_rings, runsim/scenario_vax, runsim/scenario_sd, runsim/scenario_all_on |
| M04 | progression: outcome -> condition off by one | killed (sniff) | disease_modeling/progression_outcome_frequencies, social_distancing/sd_effect_on_simulation, setup/short_runs_hold_invariants | runsim/scenario_base, runsim/scenario_rings, runsim/scenario_vax, runsim/scenario_sd, runsim/scenario_all_on |
| M05 | progression: immunity redistribution inverted | killed (sniff) | disease_modeling/progression_outcome_frequencies, disease_modeling/progression_immunity_redistribution, vaccination/vaccine_reduces_progression_severity | runsim/scenario_base, runsim/scenario_rings, runsim/scenario_vax, runsim/scenario_sd, runsim/scenario_all_on |
| M06 | recoveffect: immunity matrix transposed | killed (sniff) | disease_modeling/recoveffect | - |
| M07 | vaxeffect: always uses first-shot effectiveness | killed (sniff) | vaccination/vaxeffect_follows_parameters, vaccination/vaccine_protects_against_infection, vaccination/vaccine_reduces_progression_severity | runsim/scenario_vax, runsim/scenario_all_on |
| M08 | vaccination: no 14-day wait after recovery | killed (sniff) | vaccination/eligibility_filter_status_recovery | runsim/scenario_vax, runsim/scenario_all_on |
| M09 | sum_total_histories: ring filter dropped (ring totals sum all rings) | killed (sniff) | series/sum_total_histories_brute_force, disease_modeling/state_transitions_update_person_and_histories, setup/short_runs_hold_invariants | runsim/scenario_rings, runsim/scenario_all_on |
| M10 | init_history_newday: carries new_ instead of now | killed (sniff) | series/newday_carries_now_not_new, disease_modeling/state_transitions_update_person_and_histories, social_distancing/sd_effect_on_simulation, setup/short_runs_hold_invariants | runsim/scenario_base, runsim/scenario_rings, runsim/scenario_vax, runsim/scenario_sd, runsim/scenario_all_on |
| M11 | SD tagging: sick instead of mild people eligible | killed (sniff) | social_distancing/sd_tagging_on_start_and_end_days | runsim/scenario_sd, runsim/scenario_all_on |
| M12 | vaccination: daily first-shot allotment not decremented | killed (deep only) → after adding `per_brand_daily_allotment`: killed (sniff) | vaccination/per_brand_daily_allotment (added after this run) | runsim/scenario_vax, runsim/scenario_all_on |
| M13 | touch_map: recovered use unexposed touch row | killed (sniff) | disease_modeling/touch_map, disease_modeling/spread_touch_factors_by_contact | - |
| M14 | spread: rings ignored (global mixing) | killed (sniff) | disease_modeling/spread_ring_selection | runsim/scenario_rings, runsim/scenario_all_on |
| M15 | SeedCase: counts ineligible matches again (seed counting bug) | killed (sniff) | social_distancing/seed_case_counts_only_people_it_infects | - |
| M16 | infectrisk: recvrisk ignores contact age | killed (sniff) | disease_modeling/infectrisk_indexing_and_clamp, social_distancing/sd_effect_on_simulation, setup/short_runs_hold_invariants | runsim/scenario_base, runsim/scenario_rings, runsim/scenario_vax, runsim/scenario_sd, runsim/scenario_all_on |
| M17 | SD load: endday ignored | killed (sniff) | parameters/social_distancing_cases_load | runsim/scenario_sd, runsim/scenario_all_on |
| M18 | progression: duration not advanced on non-breakdays | killed (sniff) | disease_modeling/progression_outcome_frequencies | runsim/scenario_base, runsim/scenario_rings, runsim/scenario_vax, runsim/scenario_sd, runsim/scenario_all_on |

**Result: 18 of 18 injected bugs caught.** 17 were caught by the sniff tier on the first run. M12 was caught only by the deep tier, which exposed a gap: no sniff test had two brands where one runs short within a day. `per_brand_daily_allotment` was added and now catches M12 in the sniff tier. The script is kept at `test_output/mutation/mutate.py` (gitignored). Usage: `python3 -I mutate.py <repo copy outside the project> <project root> <out.md>`. Run it from the project root, because case directories must be inside `HOME`.

## 5. Assessment

**Strengths**
- Fast sniff tier (< 1 s) that exercises the real loaders, the real setup path and short real runs, suitable for an AI agent after any edit.
- The macro checks are exact invariants, not tolerances, so a pass is meaningful. Each failure points to a unit test.
- Contract tests read the template JSON directly, so recalibrating parameters does not break them. Changing an index convention does.
- Statistical tests (progression frequencies, comply rate, brand mix, infection rates) use fixed seeds and tolerances of 4 standard deviations or more, so they do not flake.

**Limits**
- Goldens are exact. Any change to RNG consumption order or to the model changes them by design. Review and refresh deliberately (§1).
- The plot check covers the Plotly data contract, not rendering. A PNG comparison would need a headless browser. `--artifacts` writes a reference HTML for a visual check.
- The TUI's `run_dir` is private, so the deep tier calls the same public steps (`use_dir`, `runsim`, `post_simulation`). The menus themselves are not driven.
- Not covered: `r0_sim`/`rt_sim`, help text, and calibration (death rates by age against observed data). Single runs differ by about ±30 deaths between seeds. Compare averages over seeds, not single runs, when judging calibration.
- `templates` writes to `~/.config/epi_sim` (restored afterwards), so it is not part of the sweep.

**Suggested next steps**
- Fix B4 (cap the daily doses at the remaining supply; validate `pattern` in `input_verify` and in `load_vax_sched`), then turn its `KNOWN_BUG` into `CHECK`, as was done for B5 and G1–G5.
- Fix `design/running_tests.md` (stale) by pointing it at §1.
