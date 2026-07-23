# Session Notes

## Current State: Retained Normal-Run Results

Runtime trait-name maintenance is complete for the normal simulation path.
Runtime wrappers still store compact numeric IDs in `PopData`; their
human-readable names come from model-owned sources of truth:

- `ModelParams::variant_names`
- `VaxSet::names`
- `RingTraits::ring_names`
- `Model::sd_cases[*].name` for `SDCase`

`install_runtime_trait_names(const Model&)` reconstructs the active static
registries. `runsim(Model&)` calls it before sizing or using `AllSeries`, so a
retained model is interpreted using its own variant, vaccine, ring, and social
distancing names.

Sentinel conventions are explicit and index-aligned:

- Variant and Vax: index 0 is `"none"`; Variant index 1 is required to be
  `"base"`.
- Ring: index 0 is the empty-string sentinel; real rings begin at index 1.
- SDCase: index 0 is `"none"`; remaining names follow `model.sd_cases`.

`setup_model_params()` remains aggregate initialization. In particular,
`GeoData` must continue to be moved into `ModelParams`; omitting that member
leaves `mp.geodata` empty and makes valid locales appear invalid.

`runsim(Model&)` now returns the completed `AllSeries` by value. The terminal
TUI state is now `AppState`, with `active_model` retaining the completed
normal-run `Model` and `result_series` retaining the matching completed
`AllSeries`. `/run-case` and `/run-dir` reset the prior retained result before
starting a new main run and replace it with the returned `AllSeries` when the
run succeeds.

`/r0_sim` is detached from retained TUI state. It constructs a local model from
case inputs, runs the academic R0 estimate, prints the scalar result, and does
not mutate `AppState`.

## Completed-Run Architecture Decision

`Model` remains the runnable configuration plus current population state.
`AllSeries` is the completed-run result and should not become a `Model` member.

`runsim(Model&)` returns `AllSeries` by value. The return is moved/copy-elided;
the collected vector allocations are not deep-copied. `AppState` retains the
completed normal-run `Model` and `AllSeries` together. Starting a new
`/run-case` or `/run-dir` intentionally discards the prior retained normal-run
result.

The first reuse command should inspect, select, serialize, or plot the retained
completed result. It must not rerun the completed `Model`: its `PopData` is
already at the final simulation state, and replay/extension/reset semantics
are not implemented.

The retained model-owned name tables plus `install_runtime_trait_names()` are
enough to reconstruct valid `SeriesColSpec` selections for a retained
`AllSeries`; no separate stored series-column-name list is needed yet.

## Validation

- `xmake run test` passed after the runtime-name and aggregate-construction
  repairs. Test-related plot files do not reliably load in a browser; that is
  separate from the simulation/test result.
- `xmake run test runsim` passed.
- Parameter tests cover the Variant `"none"`/`"base"` indexing and the
  model-owned `variant_names` vector.
- `project_cases_help` now documents project and standalone case commands,
  input/output locations, and the `parameters` help topic. `xmake build epi_sim`
  and `xmake run epi_sim --help project` passed.
- `socialparameters_help` now documents every `socialparams.json` key and its
  template value. `xmake build epi_sim` and `xmake run epi_sim --help socialparameters`
  passed.
- `socialparams_help` now contains the same documented values from the
  `case-1` social-parameters template. `xmake build epi_sim` passed.
- `variants_help` now contains an underscored title, a description of
  progression transitions, and the complete commented variants template.
  `xmake build epi_sim` and `xmake run epi_sim --help variants` passed.
- The TUI help map now exposes `socialparams`, wired to the existing
  `socialparams_help` text. A subsequent `xmake build epi_sim` could not run
  because its configured LLVM executable
  `/opt/homebrew/Cellar/llvm/22.1.6/bin/clang++` no longer exists.
- `xmake f --check` refreshed the stale per-target linker cache while retaining
  the configured vcpkg path. Both compiler and linker now use Homebrew LLVM
  22.1.8; `xmake build epi_sim` passed.
- Removed the duplicate `socialparameters` help topic; `socialparams` is now
  the sole topic for `socialparams.json` in the TUI and CLI help maps.
- Human developer changed `runsim(Model&)` to return `AllSeries` and wired
  `/run-case` and `/run-dir` to store the result in `AppState::result_series`.
  `xmake build epi_sim` and `xmake build test` passed.
- Human developer detached `/r0_sim` from retained `AppState`: it now uses a
  local model from case inputs, prints the academic R0 estimate, and leaves the
  retained normal-run model/result untouched.
- Human developer ran `xmake run test`; all tests passed. For future test
  coverage around `runsim`, tests can set `Model::headless = true` before
  calling `runsim()` to skip plot/browser output.

## Next Steps

1. Add a TUI command that uses the retained completed run for series
   introspection/output; restore its runtime trait names before selection.
2. Extend `test runsim` with a case that enables vaccination and rings. The
   current runsim fixture does not cover either path.
3. Diagnose the browser plot-loading behavior separately from simulation
   correctness.

## Working Constraints

- Use xmake only and always name the target.
- `xmake run test` intentionally excludes `runsim`; invoke
  `xmake run test runsim` explicitly.
- `Model::headless = true` lets tests call `runsim()` without writing normal
  run outputs or opening browser plots.
- Preserve compact numeric trait IDs in population data and direct indexed
  access in hot simulation paths.
