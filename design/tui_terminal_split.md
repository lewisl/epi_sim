# Split `tui_terminal.cpp` Into UI Helpers and Command Behavior

## Summary

Refactor only the source organization. Preserve current behavior, command
names, state semantics, and TerminalOutput flow. The split should separate the
stable FTXUI prompt/event handling from the command layer where future TUI
commands will be added.

## Files

### `src/tui_commands.h`

Keep this as the public app entrypoint header:

```cpp
#pragma once

int run_terminal_tui();
```

No new command/UI internals should be exposed here.

### New `src/tui_app.h`

Add a small internal TUI helper header used only by TUI implementation files.

Declare:

```cpp
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ftxui/dom/elements.hpp"

std::optional<int> choose_menu(std::string_view prompt,
                               const std::vector<std::string>& entries,
                               std::string_view case_label = {});

std::optional<std::string> ask_text(std::string_view prompt);

void clear_terminal_lines(int line_count, bool leave_blank_line = true);
void clear_menu_widget(std::size_t entry_count);

void print_element(ftxui::Element element);
void print_command_card(std::string_view command_text);
void print_output_boundary(bool leading_blank);
```

Use `print_command_card(std::string_view command_text)` instead of exposing the
`Command` type to the UI helper layer.

### New `src/tui_app.cpp`

Move the UI/prompt/event implementation here:

- `MenuState`
- `TextPromptState`
- `PANEL_WIDTH`
- `MENU_FOOTER_TEXT`
- `CASE_FOOTER_PREFIX`
- `selection_bg()`
- `rule_color()` if still used; delete only if confirmed unused after the move
- `faint_color()`
- `output_rule_color()`
- `clear_terminal_lines()`
- `menu_widget_line_count()`
- `clear_menu_widget()`
- `flush_pending_prompt_text()`
- `trim()`
- `print_element()`
- `print_command_card(std::string_view command_text)`
- `print_output_boundary()`
- `render_menu_panel()`
- `is_cancel_event()`
- `contains_cancel_sequence()`
- `CursorReportMatch`
- `match_cursor_position_report()`
- `append_prompt_text()`
- `jump_key()`
- `jump_menu_selection()`
- `cancel_menu()`
- `cancel_text_prompt()`
- `is_backspace_event()`
- `handle_menu_event()`
- `render_text_prompt()`
- `handle_text_prompt_event()`
- `choose_menu()`
- `ask_text()`

These remain implementation details except for the declarations listed in
`tui_app.h`.

### `src/tui_commands.cpp`

Keep command behavior and the main TUI loop here:

- includes `tui_commands.h`
- includes `tui_app.h`
- `CommandAction`
- `Command`
- `AppState`
- `commands`
- `command_menu_entries()`
- `print_state_summary()`
- `run_help_topics()`
- `show_plot_files()`
- `run_case()`
- `r0sim()`
- `run_dir()`
- `dispatch_command()`
- `run_terminal_tui()`

Adjust `dispatch_command()` so it builds the display string before calling the
UI card helper:

```cpp
std::string command_text = command.name;
if (!arg.empty()) command_text = fmt::format("{} {}", command.name, arg);
print_command_card(command_text);
```

## Implementation Notes

- Keep all moved helpers in anonymous namespaces where possible.
- Do not change command behavior, prompts, menu entries, output text, or retained
  `AppState` semantics.
- Do not introduce classes, virtual interfaces, or a command framework.
- No `xmake.lua` change should be needed because it already compiles
  `src/*.cpp`.
- Preserve the current `TerminalOutput` helper model: `choose_menu()` and
  `ask_text()` remain the only blocking FTXUI prompt functions.

## Test Plan

Run:

```bash
xmake build epi_sim
```

Then smoke test the TUI manually:

```bash
xmake run epi_sim --tui
```

Check:

- Main command menu displays.
- `/help` opens the help-topic submenu and `/back` returns.
- `Esc` still cancels a menu or text prompt.
- Text prompt commands still accept input.
- `/q` exits and clears the menu area.

If source moves touch shared declarations unexpectedly, also run:

```bash
xmake build test
```

## Assumptions

- This is a mechanical source-organization change only.
- The command layer lives in `src/tui_commands.cpp`.
- The new helper header is internal to TUI files and should not become a general
  app API.
- Future user-visible commands should usually be added in `src/tui_commands.cpp`,
  not in the UI helper file.
