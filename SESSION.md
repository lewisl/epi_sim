# Session Notes

## Current State: Inline TerminalOutput TUI

The `main` branch now uses a sequential `ScreenInteractive::TerminalOutput()`
menu for `epi_sim --tui`. The earlier full-screen TUI work is preserved on the
`fullscreen-tui` branch and no longer drives work on this branch.

### Implemented approach

- `src/tui_terminal.cpp` / `src/tui_terminal.h` provide
  `run_terminal_tui()`.
- `src/epi_sim.cpp` dispatches `--tui` to the inline runner.
- Each command menu or text prompt is a short-lived TerminalOutput widget.
  After it exits, existing command functions run normally and may print directly
  to stdout/stderr.
- `TerminalAppState` survives across menu iterations and currently retains the
  active `Model`, current case label/path, and last output directory.
- Help topics come from the shared `hlptxt::help_map`.
- `/list-output-files` and `/plot` report files associated with the most recent
  run output directory.
- The top-level command-menu footer shows the most recently completed run as
  `Current case: <label>` at its right edge; long labels are truncated to keep
  the fixed-width footer to one line. Help-topic menus retain their original
  footer.
- Accepted non-quit commands render a light-gray FTXUI command card and gray
  output boundaries, while command and simulation functions continue to print
  directly to the console. The command card has three highlighted rows (blank,
  command, blank), with unhighlighted blank rows before and after it; cancelled
  and empty prompts remain unframed.
- Menu cancellation, `/help -> /back`, `/q`, type-to-jump, prompt Esc handling,
  and VS Code cursor-position traffic have dedicated handling.

This approach deliberately avoids capturing stdout/stderr or changing existing
command functions to return display strings.

### Current validation

- `xmake build epi_sim` passed.
- `xmake run test parameters` passed: 146 checks, 0 failed.
- `xmake run epi_sim --help` passed.
- `xmake build epi_sim` passed after the command-menu footer update.
- TTY smoke confirmed the no-active-case command menu retains its original
  single-line footer and exits cleanly through `/q`.
- `xmake build epi_sim` passed after adding framed command output. TTY smokes
  passed for `/list-output-files`, nested `/help`, prompt cancellation, and
  `/q`.
- The static FTXUI renderer appends `\r\n` after each printed element so later
  rules and ordinary `fmt` output always start at the terminal's first column.
- TTY smokes passed for menu rendering and `/q`, prompt Esc, injected `[47;1R`
  traffic, `/help -> /back`, and top-menu Esc.
- `choose_menu()` and `ask_text()` now keep their FTXUI composition short by
  naming the rendering and event-policy helpers; the behavior remains the
  same. `xmake build epi_sim` and a TTY smoke of type-to-jump, `/help ->
  /back`, prompt typing/Backspace/Esc, empty Enter, and `/q` passed.
- `xmake run test runsim` was not rerun for the TUI change because it opens
  browser tabs by design and the simulation loop was not changed.

### 2026-07-09: Rt interval validation

- `rt_sim_interval` is a required non-negative integer in `config.json`; its
  value is validated without being passed through `input_verify()`.
- The scaffold default is `0` (disabled), and parameter tests cover zero and negative
  intervals.
- `xmake run test parameters` passed: 148 checks, 0 failed.
- `xmake build epi_sim` passed.
- `docs/input verification.md` maps the preflight validation flow, shared error
  accumulator, and file-specific checks.

### 2026-07-14: History timing scope

- `History time` now includes the `AllSeries` updates in seed cases, spread
  infections, and progression recoveries/deaths, plus `finalize_series()`.
- `sim::history_timing` is global instrumentation state, reset at each
  `runsim()` start; Rt transition updates accumulate when Rt estimation is enabled.
- `xmake run test` passed: 628 checks, 0 failed.

### Near-term architecture: completed run state and series

- `Model` is the runnable configuration plus the population at its current
  simulation tick; `AllSeries` is the separately accumulated, completed
  time-series history.
- Change `runsim(Model&)` to return its completed `AllSeries` so the terminal
  runner can retain it beside the live `Model` for post-run commands.
- The complete transition-history path is timed. Per-day history initialization
  and direct updates outside the transition methods remain outside `History time`.
- Move runtime variant, vaccine, ring, and related definitions from mutable
  static registries into per-run model-owned data. Keep compact numeric trait
  IDs and direct indexed access in hot loops; compile-time tables such as
  `Status` and `Agegrp` remain global constants.
- Retaining one active run is the initial target. If multiple completed runs are
  retained later, each must keep the definitions that interpret its `AllSeries`.
- Since all series are already collected, make future serialization and plotting
  choose from curated output bundles after a run rather than presenting
  individual-series checkboxes or deciding collection before the run starts.

## Next Steps

Work in small, independently testable changes:

1. Consume terminal cursor-position reports without cancelling the active text
   prompt. Both FTXUI cursor-position events and parsed `[row;colR` traffic
   should be ignored as control traffic while the prompt remains open.
2. Make run-state updates transactional: build and run a local `Model`, then
   replace `TerminalAppState::active_model` and its metadata only after the run
   succeeds. A failed run should leave the previous successful state coherent.
3. Retain the active `Model` and completed `AllSeries` after a successful run
   for post-run commands, replacing both together transactionally on the next
   successful run.
4. Reduce dependence on fixed 76-column rendering and hard-coded line counts.
   Test a narrow terminal and prefer a cleanup strategy that cannot erase prior
   shell output or leave wrapped menu fragments behind.
5. Decide whether menu command failures must return to the menu. Existing deep
   `std::exit()` paths bypass the runner's exception handler; changing that is a
   broader command-layer task and should be done only if recoverable TUI errors
   are now a requirement.
6. Return `run_terminal_tui()` directly from the `--tui` branch so the TUI exit
   code is propagated and argument parsing cannot continue after the menu ends.
7. Sort `/list-output-files` and `/plot` results before printing for stable,
   deterministic output.

## Working Constraints

- Use xmake only and always name the target.
- Preserve the inline architecture's direct stdout/stderr behavior unless a
  separate output-layer redesign is explicitly requested.
- Validate prompt/menu changes in a real TTY in addition to building.
- `runsim` remains excluded from the no-argument test sweep and must be invoked
  explicitly when needed.
