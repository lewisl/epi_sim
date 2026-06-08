# Session: case-config setup/run wiring

## Current task
- Rewired case-config input loading so `epi_sim --use-case case-a/a1` resolves all input paths from the managed case `input/config.json` without relying on cwd.

## Changes made
- Updated `src/epi_sim.cpp` to:
  - Resolve config-owned input paths against the config file's `input/` directory.
  - Store resolved path strings in `Config`.
  - Resolve `output` against the case root.
  - Run `setup_sim(config)` followed by `runsim(model)`.
- Updated `src/setup.cpp` to:
  - Parse seed JSON before calling `load_seed_cases`.
  - Keep missing social distancing as an empty vector sentinel.
  - Move loaded `seedcases` and `sd_cases` into the returned `Model`.
- Updated tests for the new ownership/API:
  - `test/test_runsim.cpp` now checks `model.seedcases` and `model.sd_cases`, then calls `runsim(model)`.
  - `test/test_setup.cpp` now uses `Config::social_params` and supplies `sample_parameters/seed_basic.json`.

## Verification
- `xmake build epi_sim` passed.
- `xmake run epi_sim --use-case case-a/a1` completed a full simulation and applied both day-1 seed cases.
- `xmake run test runsim` passed: 7 checks passed.
- Checked `/Users/lewislevin/test_project/case-a/a1`; current generated scaffold has `input/vaccine_100k` and no top-level `vaccine_100k` directory.

## Notes
- The run still prints macOS URL open errors for generated plot HTML files; simulation completes successfully despite those messages.
- There are many unrelated dirty worktree changes; they were not reverted.

## Next work
- Wire configured output paths through plots, selected series serialization, and population serialization; output should come from case config instead of hardcoded repo-relative directories.
- Implement `--setup-dir` switch actions.
- Implement `--use-dir` switch actions.
- Refactor `epi_sim.cpp` CLI branch bodies so each branch is small and delegates to appropriate helpers in `param_init.cpp`.
- `--setup-dir` and `--use-dir` should reuse the same lower-level helpers as the retained project-dir flow; the only intended difference is that the user must supply the project dir each time because it is not persisted.
- Decide and implement a new social-distancing sentinel for the scaffold/config flow. The old sentinel was omitting the separate CLI switch; now users will edit scaffolded config/files. Options to discuss: `social_dist = ""` or an explicit boolean. The implementation should allow the social-distancing file to exist in the scaffold while the configured case runs with an empty `sd_cases` vector and naturally skips the SD action path. Add tests for the chosen behavior.
