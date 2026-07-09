# Session Notes

## Latest Session: TerminalOutput TUI Prototype

Implemented a new sequential `ScreenInteractive::TerminalOutput()` path for
`epi_sim --tui`, while preserving the full-screen prototype as `run_tui()`.

### What changed

- Added `src/tui_terminal.cpp` / `src/tui_terminal.h` with `run_terminal_tui()`.
- Switched `src/epi_sim.cpp` so `--tui` calls `run_terminal_tui()`.
- The new runner keeps C++ mainline control:
  - every command/help/text prompt opens a short-lived TerminalOutput widget;
  - after the widget exits, real command functions run normally and may print to
    stdout/stderr;
  - the outer loop keeps `TerminalAppState` alive until `/q`.
- `TerminalAppState` retains the active `Model`, current case label/path, and
  last output directory after `/run-case` or `/run-dir`.
- Help browsing uses `hlptxt::help_map` and allows repeated topic selection
  before returning to the command menu.
- `/list-output-files` and `/plot` are first-pass state follow-ups: they report
  files/HTML plot paths from the last run output directory.
- Updated the old full-screen `src/tui_example.cpp` only for compatibility with
  the active `show_cases()` signature, which now prints and returns `void`.

### Validation

- `xmake build epi_sim` passed.
- `xmake run test parameters` passed: 146 checks, 0 failed.
- `xmake run epi_sim --help` passed.
- TTY smoke: launched `./build/macosx/arm64/release/epi_sim --tui`, rendered the
  inline command menu, selected `/q`, and exited cleanly.

### Notes

- This deliberately does not capture stdout/stderr and does not refactor command
  functions to return strings.
- `xmake run test runsim` was not run in this pass because it opens browser tabs
  by design; simulation code itself was not changed.

### Follow-up visual cleanup

- TerminalOutput widgets now use horizontal separators only, with no side box
  borders, to reduce broken line drawing in normal shell scrollback.
- Command and text-input panels are fixed at 76 columns.
- Selected menu rows use black text over FTXUI's `Grey84` palette color, which
  maps to the terminal's normal light-gray background slot in the current TTY
  smoke.
- Text-entry prompts use the same black-on-light-gray styling for typed values,
  avoiding FTXUI's default inverted prompt highlight.
- Text-entry prompts now use a small custom single-line component instead of
  FTXUI `Input`, so Esc is handled before any character can be echoed into the
  field. The prompt also handles raw ESC bytes and the `^[` representation.
- VS Code terminal cursor-position replies such as `[47;1R` are treated as
  terminal control traffic in text prompts. They are consumed incrementally even
  when delivered as ordinary character events, so they cannot become command
  arguments.
- `/q` now exits without echoing `/q` to the terminal after the menu closes.
- On exit, the runner clears upward through the rendered main-menu block,
  including the TerminalOutput cursor row, and emits a blank line. It no longer
  walks the cursor back down through the erased block, which could scroll the
  terminal by the full menu height.
- Menu-to-menu transitions that do not print normal output now clear the
  just-closed widget before redrawing. This is used for `/back` from help topics
  and menu Esc cancellation, while command selections and help-topic selections
  keep their menu in scrollback because stdout output follows.
- Menu type-to-jump is restored: pressing the first letter of a command/topic
  advances the highlighted row, ignoring a leading slash.
- Validation after cleanup: `xmake build epi_sim`, `xmake run test parameters`,
  a prompt Esc TTY smoke, an injected `[47;1R` prompt smoke, `/help -> /back`
  and top-menu Esc TTY smokes, and a `/q` TTY smoke all passed.

## Previous Session: FTXUI Bootstrap TUI (scratch/tui)

Built a working bootstrap TUI per `scratch/tui/tui-prompt.md`. `src/` untouched.

### FTXUI 7 / vcpkg compatibility note

- Switched `xmake.lua` to use `vcpkg::ftxui` for the main and scratch TUI
  targets, and added it to the test target because the test target compiles
  `src/tui_example.cpp`.
- Renamed the local TUI state struct from `App` to `TuiApp` in both
  `src/tui_example.cpp` and `scratch/tui/tui_example.cpp` because FTXUI 7 adds
  `ftxui::App`, which collides under `using namespace ftxui`.
- Replaced namespace-scope `Color::RGB(...)` constants with lazy color helper
  functions because FTXUI 7 calls terminal color-support code during color
  construction, which crashed before `main` when those colors were static
  initializers.
- While removing `using namespace ftxui`, keep foreground and background
  decorators distinct: `ftxui::color(...)` is foreground text color, while
  `ftxui::bgcolor(...)` is needed for filled panels, selected menu rows, and
  the left accent bar.
- Validation: `xmake b -r epi_sim`, `xmake b -r this`, direct
  `./build/macosx/arm64/release/epi_sim --help`, and `xmake run epi_sim --tui`
  all work.

### What exists now

- `xmake.lua`: `add_requires("vcpkg::ftxui")`; the `this` target builds
  `scratch/tui/tui_example.cpp` + `scratch/tui/stubs.cpp` with
  `add_packages("vcpkg::ftxui")` and `vcpkg::fmt`.
  Build/run: `xmake run this`.
- `scratch/tui/stubs.cpp`: command stubs refactored to **return** strings
  (no stdout printing, which would corrupt the full-screen TUI).
- `scratch/tui/tui_example.cpp` (~510 lines): all TUI logic.

### Feature summary

- Opening screen: centered title + welcome box; output screen: scrolling log,
  sidebar (Case/Population/Output/Plots), prompt box pinned at bottom.
- Footer below the shaded panels: cwd left, case tag / "No case selected..."
  right, one blank row of separation.
- Styling: flat shaded panels (`kBoxBg` gray, no borders), purple left accent
  rule (`kAccent` RGB(140,100,230)) on welcome box, prompt box, and command
  echo lines in the log; bold `>` input marker.
- `/` menu (only when input is empty): custom-rendered rows — fixed 20-col
  command name + `paragraphAlignLeft` description with hanging indent (FTXUI's
  stock `Menu` render is bypassed; the component is kept for arrows/Enter).
  Menu is centered on the opening screen, inline above the prompt box on the
  output screen. `/help` opens a help-topics submenu (Esc steps back).
- Type-to-jump in menus: a letter moves the highlight to the next match
  (cycles on repeat, `/` prefix ignored); Enter required to select.
- Quit is a menu command `/q` only — no bare `q`, no Esc-to-quit (deliberate).
- Real text input in the prompt box (FTXUI `Input`, single-line; long text
  scrolls horizontally — soft-wrap while typing deliberately skipped as
  cursor math isn't worth it for a bootstrap). Enter echoes typed text to the
  log. Output log lines word-wrap via `paragraphAlignLeft`.
- Argument-taking commands (`/set-project-dir`, `/init-case`, `/use-case`,
  `/setup-dir`, `/use-dir`): selecting them logs an ask prompt, pre-fills the
  input with the command name, cursor after it; Enter dispatches the stub with
  the argument (`finish_pending_command`); Esc cancels (`app.pending_cmd`).
- Scrolling: `app.scroll` + `focusPositionRelative`; arrows/PageUp/PageDown/
  mouse wheel scroll the log (Home/End belong to the input); resets to bottom
  on new output.

### Next steps for the TUI track

1. Wire stubs to the real command implementations in `src/`.
2. Typed-command dispatch (typing `/init-case foo` + Enter should execute,
   not just echo).
3. Populate the sidebar from application state.

## Current State (prior track: input validation / runsim)

Input validation and the `runsim` integration test were the most recent focus.
Completed work that no longer needs to drive the session:

- `test/test_runsim.cpp` now uses the real `setup_dir` scaffold unchanged
  instead of shrinking `config.json` to 30 days or replacing `seed.json`.
  The explicit `runsim` group now exercises a full 180-day scaffolded run.
- `input_verify()` catches missing files and missing required config keys before
  model construction. Manual `--use-case case-1` testing confirmed the missing
  `config.json` `days` report is clear.
- `input_verify` error logging was fixed. The bug was output buffering: the code
  wrote an automatic `std::ofstream` and then called `std::exit`, so the stream
  destructor/flush did not run and `input-error-log.txt` could contain only `I`.
  It now writes through `input_verify_detail::write_error_log()`, which closes
  the file before exit. Manual testing confirms the full report is written.

## Current Validation Snapshot

```text
xmake run test runsim       -> 30 checks: 30 passed, 0 failed
xmake run test parameters   -> 146 checks: 146 passed, 0 failed
xmake run test              -> 626 checks: 626 passed, 0 failed
```

## Important Working Assumptions

- `src/template.cpp` is the single source of truth for scaffolded case inputs.
- For end-to-end/manual validation, prefer the real app path:
  `xmake run epi_sim --setup-dir <case-dir>`, perturb files under
  `<case-dir>/input`, then `xmake run epi_sim --use-dir <case-dir>` or
  `--use-case <case-label>`.
- When reviewing/fixing tests, default to changing `test/*.cpp`. If a test
  exposes a real source bug, call that out and keep the source change targeted.
- Use `xmake` only. `runsim` is excluded from the no-arg test sweep and must be
  run explicitly.

## Next Task (prior track — superseded in priority by TUI next steps above if
the user continues the TUI work)

Audit the older guards in the parameter-loading/initialization path and decide
which are still required now that `input_verify` catches many structure and
critical-value errors earlier.

Start by mapping overlap between:

1. `input_verify` structural/value checks in `src/input_verify.cpp`.
2. Loader guards in `src/parameters.cpp`.
3. Initialization/path handling in `src/param_init.cpp`.
4. Existing tests in `test/test_parameters.cpp`.

Goal: identify guards that are redundant, guards that should remain as defense
in depth, and any remaining gaps. Do not remove guards until their callers and
tests have been reviewed.

## Open Questions / Likely Follow-ups

- Some loader guards may still be needed because loaders can be called directly
  by tests or future code without going through `build_model()`/`input_verify()`.
- `load_json_params` may still produce misleading diagnostics for JSON syntax
  errors; check whether `input_verify` now makes that irrelevant for normal CLI
  case runs, and whether direct loader tests still need clearer behavior.
- Top-level CLI crash presentation for loader exceptions may still be rough in
  paths that occur after `input_verify` succeeds.
