# Session: social distancing wiring audit

## Current task
- User asked to check whether social distancing cases are loaded properly and wired to actually run.
- This was an inspection/audit only; no source code changes were made.

## Findings
- `--sd_seed` is accepted in `src/epi_sim.cpp` and loaded via `load_sd_cases(load_json_params(sd_seed_path), model.mp.socialdata)`.
- `load_sd_cases` parses cases, validates `contact_delta`/`touch_delta` sizes, shifts per-case contact/touch matrices, and registers `SDCase::names` as `"none"` plus loaded case names.
- `runsim` calls `apply_sd_cases_for_day` near the start of each simulation day and passes `sd_cases` into `spread`.
- `spread` uses a nonzero spreader `sdcase` to select per-case `contactfactors`, and a nonzero contacted agent `sdcase` to select per-case `touchfactors`.
- Runtime check: `xmake build epi_sim` passed. Running `epi_sim` with absolute `--sd_seed sample_parameters/soc_dist.json` completed and produced different final totals than the same run without `--sd_seed`, confirming the path affects simulation behavior.

## Issues / risks
- `apply_sd_cases_for_day` currently tags compliers only on `startday` and clears only on `endday`; between those days marks persist. This conflicts with `design/sdcase_implementation_plan.md`, which says active cases should reset and redraw compliance every active day.
- `sd_seed_path` is not resolved relative to the config directory, unlike config-owned paths. Relative paths work only relative to the process working directory.
- Existing tests do not cover SD case loading, day application, or the spread factor override path.
