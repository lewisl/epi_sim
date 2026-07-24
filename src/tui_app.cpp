#include "tui_app.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/screen.hpp"

using namespace ftxui;

namespace {

struct MenuState {
  std::vector<std::string> entries;
  int selected = 0;
  bool cancelled = false;
};

struct TextPromptState {
  std::string input;
  std::string pending_control;
  bool cancelled = false;
};

constexpr int PANEL_WIDTH = 76;
constexpr std::string_view MENU_FOOTER_TEXT = "Enter selects. Esc cancels.";
constexpr std::string_view CASE_FOOTER_PREFIX = "Current case: ";

// Color selection_bg() {
//   return Color::Grey84;
// }

Color selection_bg() { return Color::Palette256(153); }  // light steel blue
Color faint_color()  { return Color::Palette256(254); }  // very light grey
Color output_rule_color() { return Color::Palette256(245); }  // mid grey

int menu_widget_line_count(size_t entry_count) {
  return static_cast<int>(entry_count) + 5;
}

void flush_pending_prompt_text(std::string& input,
                               std::string& pending_control) {
  input += pending_control;
  pending_control.clear();
}

std::string trim(std::string value) {
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

Element render_menu_panel(std::string_view prompt, const MenuState& state,
                          std::string_view case_label) {
  Elements rows;
  for (int i = 0; i < static_cast<int>(state.entries.size()); ++i) {
    Element row = text(state.entries[i]) | size(WIDTH, EQUAL, PANEL_WIDTH);
    if (i == state.selected) {
      row = row | color(Color::Black) | bgcolor(selection_bg());
    }
    rows.push_back(row);
  }

  Elements footer = {text(std::string(MENU_FOOTER_TEXT)) | dim};
  if (!case_label.empty()) {
    const int max_case_text_width =
        PANEL_WIDTH - static_cast<int>(MENU_FOOTER_TEXT.size()) - 1;
    std::string case_text = fmt::format("{}{}", CASE_FOOTER_PREFIX, case_label);
    if (static_cast<int>(case_text.size()) > max_case_text_width) {
      case_text.resize(max_case_text_width - 3);
      case_text += "...";
    }
    footer.push_back(filler());
    footer.push_back(text(case_text) | dim);
  }

  return vbox({
             text(std::string(prompt)) | bold | size(WIDTH, EQUAL, PANEL_WIDTH),
             separator(),
             vbox(std::move(rows)),
             separator(),
             hbox(std::move(footer)) | size(WIDTH, EQUAL, PANEL_WIDTH),
         }) | size(WIDTH, EQUAL, PANEL_WIDTH);
}

bool is_cancel_event(const Event& event) {
  return event == Event::Escape || event == Event::Special({27}) ||
         event.input() == "\x1b" || event.input() == "^[";
}

bool contains_cancel_sequence(std::string_view text) {
  return text.find('\x1b') != std::string_view::npos ||
         text.find("^[") != std::string_view::npos;
}

enum class CursorReportMatch {
  No,
  Partial,
  Complete,
};

CursorReportMatch match_cursor_position_report(std::string_view text) {
  if (text.empty()) return CursorReportMatch::Partial;
  if (text.front() != '[') return CursorReportMatch::No;

  size_t pos = 1;
  bool saw_row = false;
  while (pos < text.size() &&
         std::isdigit(static_cast<unsigned char>(text[pos]))) {
    saw_row = true;
    ++pos;
  }
  if (pos == text.size()) return CursorReportMatch::Partial;
  if (!saw_row || text[pos] != ';') return CursorReportMatch::No;
  ++pos;

  bool saw_col = false;
  while (pos < text.size() &&
         std::isdigit(static_cast<unsigned char>(text[pos]))) {
    saw_col = true;
    ++pos;
  }
  if (pos == text.size()) return CursorReportMatch::Partial;
  if (!saw_col || text[pos] != 'R') return CursorReportMatch::No;

  return pos + 1 == text.size() ? CursorReportMatch::Complete
                                : CursorReportMatch::No;
}

bool append_prompt_text(std::string_view value, std::string& input,
                        std::string& pending_control) {
  for (char ch : value) {
    if (ch == '\x1b') return true;

    if (pending_control.empty()) {
      if (ch == '[') {
        pending_control.push_back(ch);
        continue;
      }
      input.push_back(ch);
      continue;
    }

    pending_control.push_back(ch);
    switch (match_cursor_position_report(pending_control)) {
      case CursorReportMatch::Complete:
        pending_control.clear();
        return true;
      case CursorReportMatch::Partial:
        break;
      case CursorReportMatch::No:
        flush_pending_prompt_text(input, pending_control);
        break;
    }
  }

  return false;
}

char jump_key(std::string_view entry) {
  while (!entry.empty() &&
         std::isspace(static_cast<unsigned char>(entry.front()))) {
    entry.remove_prefix(1);
  }
  if (!entry.empty() && entry.front() == '/') entry.remove_prefix(1);
  if (entry.empty()) return '\0';
  return static_cast<char>(
      std::tolower(static_cast<unsigned char>(entry.front())));
}

void jump_menu_selection(const std::vector<std::string>& entries, int& selected,
                         char ch) {
  ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  const int count = static_cast<int>(entries.size());
  for (int step = 1; step <= count; ++step) {
    int next = (selected + step) % count;
    if (jump_key(entries[next]) == ch) {
      selected = next;
      return;
    }
  }
}

void cancel_menu(ScreenInteractive& screen, MenuState& state) {
  state.cancelled = true;
  screen.Exit();
}

void cancel_text_prompt(ScreenInteractive& screen, TextPromptState& state) {
  state.cancelled = true;
  state.input.clear();
  state.pending_control.clear();
  screen.Exit();
}

bool is_backspace_event(const Event& event) {
  return event == Event::Backspace || event == Event::Delete ||
         event == Event::CtrlH || event.input() == "\x7f" ||
         event.input() == "\b";
}

bool handle_menu_event(ScreenInteractive& screen, MenuState& state,
                       Event event) {
  if (is_cancel_event(event)) {
    cancel_menu(screen, state);
    return true;
  }
  if (event.is_character() && !event.character().empty()) {
    jump_menu_selection(state.entries, state.selected,
                        event.character().front());
    return true;
  }
  return false;
}

Element render_text_prompt(std::string_view prompt, std::string_view placeholder,
                           const TextPromptState& state) {
  Element rendered_input =
      state.input.empty() ? (text(std::string(placeholder)) | dim)
                          : text(state.input);
  rendered_input = rendered_input | color(Color::Black) |
                   bgcolor(selection_bg()) |
                   size(WIDTH, EQUAL, PANEL_WIDTH - 2);
  return vbox({
      text(std::string(prompt)) | bold |
          size(WIDTH, EQUAL, PANEL_WIDTH),
      separator(),
      hbox({text("> "), rendered_input}) |
          size(WIDTH, EQUAL, PANEL_WIDTH),
      separator(),
      text("Enter accepts. Esc cancels.") | dim |
          size(WIDTH, EQUAL, PANEL_WIDTH),
  }) | size(WIDTH, EQUAL, PANEL_WIDTH);
}

bool handle_text_prompt_event(ScreenInteractive& screen,
                              TextPromptState& state, Event event) {
  if (event.is_cursor_position()) {
    cancel_text_prompt(screen, state);
    return true;
  }
  if (is_cancel_event(event)) {
    cancel_text_prompt(screen, state);
    return true;
  }
  if (event == Event::Return) {
    flush_pending_prompt_text(state.input, state.pending_control);
    screen.Exit();
    return true;
  }
  if (is_backspace_event(event)) {
    if (!state.pending_control.empty()) {
      state.pending_control.pop_back();
    } else if (!state.input.empty()) {
      state.input.pop_back();
    }
    return true;
  }
  if (event.is_character()) {
    std::string value = event.character();
    if (contains_cancel_sequence(value)) {
      cancel_text_prompt(screen, state);
      return true;
    }
    if (append_prompt_text(value, state.input, state.pending_control)) {
      cancel_text_prompt(screen, state);
    }
    return true;
  }
  return false;
}

}  // namespace

void clear_terminal_lines(int line_count, bool leave_blank_line) {
  if (line_count <= 0) return;
  fmt::print("\r\033[2K");
  for (int i = 1; i < line_count; ++i) {
    fmt::print("\033[1A\r\033[2K");
  }
  if (leave_blank_line) {
    fmt::print("\r\n");
  } else {
    fmt::print("\r");
  }
  std::fflush(stdout);
}

void clear_menu_widget(size_t entry_count) {
  clear_terminal_lines(menu_widget_line_count(entry_count), false);
}

void print_element(Element element) {
  auto screen = Screen::Create(Dimension::Fit(element));
  Render(screen, element);
  fmt::print("{}\r\n", screen.ToString());
}

void print_command_card(std::string_view command_text) {
  const auto card_row = [](std::string_view text_value) {
    return text(text_value) | color(Color::Black) | bgcolor(faint_color()) |
           size(WIDTH, EQUAL, PANEL_WIDTH);
  };
  Element blank_line = text("") | size(WIDTH, EQUAL, PANEL_WIDTH);
  print_element(vbox({blank_line, card_row(""), card_row(command_text),
                      card_row(""), blank_line}));
}

void print_output_boundary(bool leading_blank) {
  Element output_rule = separator() | color(output_rule_color()) |
                        size(WIDTH, EQUAL, PANEL_WIDTH);
  Element blank_line = text("") | size(WIDTH, EQUAL, PANEL_WIDTH);
  if (leading_blank) {
    print_element(vbox({blank_line, std::move(output_rule), blank_line}));
  } else {
    print_element(vbox({std::move(output_rule), std::move(blank_line)}));
  }
}

std::optional<int> choose_menu(std::string_view prompt,
                               const std::vector<std::string>& entries,
                               std::string_view case_label) {
  if (entries.empty()) return std::nullopt;

  MenuState state;
  state.entries = entries;

  auto screen = ScreenInteractive::TerminalOutput();
  screen.TrackMouse(false);

  MenuOption menu_opt;
  menu_opt.on_enter = [&] { screen.Exit(); };
  auto menu = Menu(&state.entries, &state.selected, menu_opt);

  // FTXUI redraws from state, sends keys to the handler, then blocks here.
  auto render_menu = [&] { return render_menu_panel(prompt, state, case_label); };
  auto handle_event = [&](Event event) {
    return handle_menu_event(screen, state, event);
  };
  auto visual = Renderer(menu, render_menu);
  auto root = CatchEvent(visual, handle_event);

  screen.Loop(root);
  if (state.cancelled) {
    clear_menu_widget(entries.size());
    return std::nullopt;
  }
  return state.selected;
}

std::optional<std::string> ask_text(std::string_view prompt) {
  TextPromptState state;
  std::string placeholder = "Type a value and press Enter";

  auto screen = ScreenInteractive::TerminalOutput();
  screen.TrackMouse(false);

  // FTXUI redraws from state, sends keys to the handler, then blocks here.
  auto render_prompt = [&] {
    return render_text_prompt(prompt, placeholder, state);
  };
  auto handle_event = [&](Event event) {
    return handle_text_prompt_event(screen, state, event);
  };
  auto visual = Renderer(render_prompt);
  auto root = CatchEvent(visual, handle_event);

  screen.Loop(root);
  if (state.cancelled) return std::nullopt;
  return trim(state.input);
}
