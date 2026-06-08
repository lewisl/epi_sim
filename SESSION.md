# Session: case-config setup/run wiring

## Current task
- Reordered `src/param_init.h` declarations to match the current `src/param_init.cpp` function order after the user's rearrangement.

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
- Renamed `run_case_dir` to `run_case` in `src/param_init.cpp` and `src/param_init.h`; verified with `xmake run test`.
- Removed the AiDex-specific workflow/tool reference from `AGENTS.md`; navigation guidance now says to use the fastest appropriate tools and verify in actual files.
- Initialized AiDex for the repo at `.aidex/index.db`.
- Left AiDex wired in; it is no longer force-preferred by repo instructions.
- Updated `src/param_init.cpp` to remove the anonymous namespace around `resolve_home_path`, `resolve_config_path`, `resolve_optional_config_path`, `read_project_dir`, and `write_file`.
- Updated `src/param_init.h` to declare those functions in the same order as the current implementation and corrected the `write_file` declaration to match its implementation signature.
- Moved `create_scaffold` in `src/param_init.h` below `config_path_for_case_dir` and `resolve_explicit_case_dir` to match the current `src/param_init.cpp` order.

## Verification
- `xmake build epi_sim` passed.
- `xmake run epi_sim --use-case case-a/a1` completed a full simulation and applied both day-1 seed cases.
- `xmake run test runsim` passed: 7 checks passed.
- Checked `/Users/lewislevin/test_project/case-a/a1`; current generated scaffold has `input/vaccine_100k` and no top-level `vaccine_100k` directory.
- `xmake run test` passed after the `run_case` rename: 319 checks passed, 0 failed.
- `aidex_init` indexed 68 files, 7295 items, 586 methods, and 79 types.
- No tests run for the documentation-only `AGENTS.md` update.
- `xmake build epi_sim` passed after removing the anonymous namespace from `param_init.cpp`.
- `xmake run test` passed after the `param_init` declaration/linkage update: 319 checks passed, 0 failed.
- `xmake build epi_sim` passed after the header declaration order update.

## Notes
- The run still prints macOS URL open errors for generated plot HTML files; simulation completes successfully despite those messages.
- There are many unrelated dirty worktree changes; they were not reverted.
- AiDex README reference used for the AGENTS rewrite: https://github.com/CSCSoftware/AiDex
- `AGENTS.md` no longer contains AiDex-specific commentary or required session/tool usage.

## Next work
- Wire configured output paths through plots, selected series serialization, and population serialization; output should come from case config instead of hardcoded repo-relative directories.
- Implement `--setup-dir` switch actions.
- Implement `--use-dir` switch actions.
- Refactor `epi_sim.cpp` CLI branch bodies so each branch is small and delegates to appropriate helpers in `param_init.cpp`.
- `--setup-dir` and `--use-dir` should reuse the same lower-level helpers as the retained project-dir flow; the only intended difference is that the user must supply the project dir each time because it is not persisted.
- Decide and implement a new social-distancing sentinel for the scaffold/config flow. The old sentinel was omitting the separate CLI switch; now users will edit scaffolded config/files. Options to discuss: `social_dist = ""` or an explicit boolean. The implementation should allow the social-distancing file to exist in the scaffold while the configured case runs with an empty `sd_cases` vector and naturally skips the SD action path. Add tests for the chosen behavior.
