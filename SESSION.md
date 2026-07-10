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
- Menu cancellation, `/help -> /back`, `/q`, type-to-jump, prompt Esc handling,
  and VS Code cursor-position traffic have dedicated handling.

This approach deliberately avoids capturing stdout/stderr or changing existing
command functions to return display strings.

### Current validation

- `xmake build epi_sim` passed.
- `xmake run test parameters` passed: 146 checks, 0 failed.
- `xmake run epi_sim --help` passed.
- TTY smokes passed for menu rendering and `/q`, prompt Esc, injected `[47;1R`
  traffic, `/help -> /back`, and top-menu Esc.
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

## Next Steps

Work in small, independently testable changes:

1. Consume terminal cursor-position reports without cancelling the active text
   prompt. Both FTXUI cursor-position events and parsed `[row;colR` traffic
   should be ignored as control traffic while the prompt remains open.
2. Make run-state updates transactional: build and run a local `Model`, then
   replace `TerminalAppState::active_model` and its metadata only after the run
   succeeds. A failed run should leave the previous successful state coherent.
3. Decide whether retaining the full `Model` is required by a near-term
   interactive command. Current follow-ups use only output metadata; retaining
   `Model::pop`, parameters, and ring data may hold substantial memory without a
   current consumer.
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
