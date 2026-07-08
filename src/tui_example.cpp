// scratch/tui/tui_example.cpp
// A minimal FTXUI TUI that bootstraps the epi_sim application shell:
// an opening screen, a "/" command menu, a help-topics submenu, and a
// scrolling output view with a sidebar. Commands are wired to stubs.cpp.

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/Color.hpp"

#include "stubs.h"
#include "show_help.h"

// namespace fu = ftxui;
using namespace ftxui;

namespace {

// ---- commands: name + one-line description, from commands.md --------------
struct Command {
  std::string name;   // slash command shown in the menu
  std::string desc;   // one-line description
  std::string ask;    // non-empty => prompt shown to collect an argument
};

const std::vector<Command> commands = {
    {"/help", "Get help on using this application. Tip: start with \"getting started\"", ""},
    {"/set-project-dir", "Create a directory to hold simulation cases",
     "Enter the project directory..."},
    {"/init-case", "Create a case folder in the project with template input files",
     "Enter a name for the new case..."},
    {"/use-case", "Run a simulation using the case from the project folder",
     "Enter the case name..."},
    {"/show-cases", "Show names of case folders in the current project", ""},
    {"/setup-dir", "Create a single-case folder with template input files",
     "Enter the case directory..."},
    {"/use-dir", "Run the simulation using the inputs for the case folder",
     "Enter the case directory..."},
    {"/list-output-files", "List all the output files for the current case", ""},
    {"/plot", "Show plots for the current case as tabs in your browser", ""},
    {"/q", "Quit epi_sim", ""},
};

}  // namespace

namespace {

enum class AppScreen { Opening, Output };
enum class MenuMode { Commands, HelpTopics };

// The caption line shown beneath the menu; changes with the menu level.
struct MenuDisplay {
  std::string caption = "Available commands...";
};

// A single line of scrolling output. Command echoes are flagged so they can
// be marked with a left accent rule as the user scrolls.
struct OutputLine {
  std::string text;
  bool command = false;
};

struct TuiApp {
  AppScreen screen = AppScreen::Opening;
  bool menu_open = false;
  bool quit = false;                 // set by the /q command
  MenuMode menu_mode = MenuMode::Commands;
  MenuDisplay menu_display;
  std::vector<std::string> menu_entries;
  int menu_selected = 0;
  std::vector<OutputLine> output;    // scrolling log lines
  float scroll = 1.0f;               // 0 = top of log, 1 = bottom (newest)
  std::string input;                 // text typed into the prompt box
  std::string prompt = "Press slash for commands to get started";  // placeholder
  int pending_cmd = -1;              // command index awaiting an argument
  std::string case_tag;              // empty => "No case selected..."
  std::string cwd = std::filesystem::current_path().string();
};

// Append text (splitting on newlines) to the scrolling output log.
void append_output(TuiApp& app, std::string_view text, bool command = false) {
  size_t start = 0;
  while (true) {
    size_t nl = text.find('\n', start);
    if (nl == std::string_view::npos) {
      app.output.push_back({std::string(text.substr(start)), command});
      break;
    }
    app.output.push_back({std::string(text.substr(start, nl - start)), command});
    start = nl + 1;
  }
}

void open_command_menu(TuiApp& app) {
  app.menu_mode = MenuMode::Commands;
  app.menu_display.caption = "Available commands...";
  app.menu_entries.clear();
  for (const auto& c : commands)
    app.menu_entries.push_back(fmt::format("{:<20}{}", c.name, c.desc));
  app.menu_selected = 0;
  app.menu_open = true;
}

void open_help_menu(TuiApp& app) {
  app.menu_mode = MenuMode::HelpTopics;
  app.menu_display.caption = "Available help topics...";
  app.menu_entries.clear();
  for (const auto& t : hlptxt::help_map) app.menu_entries.emplace_back(t.key);
  app.menu_selected = 0;
}

// Call the stub for the command at the given index with the given argument.
std::string call_stub(int idx, const std::string& arg) {
  switch (idx) {
    case 1: return set_project_dir_stub(arg);
    case 2: return init_case_stub(arg);
    case 3: return use_case_stub(arg);
    case 4: return show_cases_stub();
    case 5: return setup_dir_stub(arg);
    case 6: return use_dir_stub(arg);
    case 7: return list_output_files_stub(arg);
    case 8: return plot_stub();
    default: return "";
  }
}

// Execute the top-level command at the given index. Commands that need an
// argument echo themselves into the prompt box and wait for the user's input.
void run_command(TuiApp& app, int idx) {
  if (idx == 0) { open_help_menu(app); return; }  // stay in the menu, show topics
  if (idx == 9) { app.quit = true; return; }      // /q — quit the application
  if (idx < 0 || idx >= static_cast<int>(commands.size())) return;
  app.menu_open = false;
  app.screen = AppScreen::Output;
  app.scroll = 1.0f;   // jump to the newest output
  if (!commands[idx].ask.empty()) {
    // await the argument: show the ask prompt in the log and pre-fill the
    // prompt line with the command, cursor positioned after it
    app.pending_cmd = idx;
    app.input = commands[idx].name + " ";
    append_output(app, commands[idx].ask);
    return;
  }
  append_output(app, commands[idx].name, true);
  append_output(app, call_stub(idx, ""));
  append_output(app, "");
}

// Type-to-jump: highlight the next menu item starting with the letter,
// cycling through matches on repeated presses. Enter still selects.
void menu_jump_to_letter(TuiApp& app, char ch) {
  ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  auto first_letter = [&](int i) -> char {
    std::string_view name = app.menu_mode == MenuMode::Commands
                                ? std::string_view(commands[i].name)
                                : std::string_view(hlptxt::help_map[i].key);
    if (!name.empty() && name.front() == '/') name.remove_prefix(1);
    if (name.empty()) return '\0';
    return static_cast<char>(std::tolower(static_cast<unsigned char>(name.front())));
  };
  int n = app.menu_mode == MenuMode::Commands ? static_cast<int>(commands.size())
                                              : static_cast<int>(hlptxt::help_map.size());
  for (int step = 1; step <= n; ++step) {   // scan forward from the highlight, wrapping
    int i = (app.menu_selected + step) % n;
    if (first_letter(i) == ch) {
      app.menu_selected = i;
      return;
    }
  }
}

// Complete a pending command with the argument taken from the prompt line.
void finish_pending_command(TuiApp& app) {
  int idx = app.pending_cmd;
  app.pending_cmd = -1;
  app.prompt = "Press slash to choose a command...";
  // strip the echoed command name from the front of the input, if present
  std::string arg = app.input;
  const std::string& name = commands[idx].name;
  if (arg.starts_with(name)) arg.erase(0, name.size());
  while (!arg.empty() && arg.front() == ' ') arg.erase(0, 1);
  while (!arg.empty() && arg.back() == ' ') arg.pop_back();
  append_output(app, fmt::format("{} {}", name, arg), true);
  append_output(app, call_stub(idx, arg));
  append_output(app, "");
  app.input.clear();
  app.scroll = 1.0f;
}

void run_help_topic(TuiApp& app, int idx) {
  if (idx < 0 || idx >= static_cast<int>(hlptxt::help_map.size())) return;
  const auto& t = hlptxt::help_map[idx];
  append_output(app, fmt::format("/help {}", t.key), true);
  append_output(app, t.text);
  append_output(app, "");
  app.screen = AppScreen::Output;
  app.menu_open = false;
  app.scroll = 1.0f;   // jump to the newest output
}

// ---- rendering -----------------------------------------------------------
ftxui::Element render_footer(const TuiApp& app) {
  std::string right = app.case_tag.empty() ? "No case selected..." : app.case_tag;
  return hbox({text(app.cwd) | dim, filler(), text(right) | dim});
}

// ---- Colors: filled panels instead of outlined boxes --------------------
Color box_bg() { return Color::RGB(238, 238, 238); }
Color box_fg() { return Color::RGB(40, 40, 40); }
Color muted() { return Color::RGB(120, 120, 120); }
Color selected_bg() { return Color::RGB(51, 133, 255); }   // highlighted menu row
Color accent() { return Color::RGB(140, 100, 230); }  // left rule accent

// A 1-column Colored bar; stretches to the full height of its hbox row.
Element accent_bar() { return text(" ") | bgcolor(accent()); }

// A filled prompt box (with a left accent rule) at the bottom of the screen.
// `input_line` is the rendered text-input component.
Element make_prompt_box(const TuiApp& app, Element input_line) {
  auto body = vbox({
                  filler(),
                  hbox({text(" "), text("> ") | color(accent()) | bold,
                        std::move(input_line) | flex}),
                  filler(),
              }) |
              bgcolor(box_bg()) | flex;
  return hbox({accent_bar(), body}) | size(HEIGHT, EQUAL, 3);
}

Element render_opening(const TuiApp& app, Element input_line) {
  auto title = vbox({
      text("epi_sim") | bold | hcenter,
      text("Epidemic Simulation Model") | hcenter,
  });
  auto prompt = vbox({
      text(""),
      hbox({text(" "), text("> ") | color(accent()) | bold,
            std::move(input_line) | flex}),
      text(""),
      hbox({text("  "),
            text("Suggestion: choose /help followed by selecting Get started...") | color(muted())}),
      text(""),
  });
  auto box = hbox({accent_bar(), prompt | bgcolor(box_bg()) | flex}) |
             size(WIDTH, EQUAL, 72);
  return vbox({
      filler(),
      title,
      text(""),
      hbox({filler(), box, filler()}),
      filler(),
      render_footer(app),
  });
}

Element render_sidebar(const TuiApp& app) {
  std::string title = app.case_tag.empty() ? std::string("Case <>")
                                           : fmt::format("Case <{}>", app.case_tag);
  auto content = vbox({
      hbox({text(" "), text(title) | bold | color(box_fg())}),
      separator(),
      hbox({text(" "), text("Population") | color(box_fg())}),  // | bold before color
      text(""),
      hbox({text(" "), text("Output") | color(box_fg())}),
      text(""),
      hbox({text(" "), text("Plots") | color(box_fg())}),
      filler(),
  });
  return content | bgcolor(box_bg());
}

// One menu row: a fixed command column plus a wrapped description whose
// continuation lines hang-indent under the description column.
Element menu_row(std::string_view name, std::string_view desc, bool selected) {
  auto name_col = text(std::string(name)) | size(WIDTH, EQUAL, 20);
  Element rest = desc.empty() ? filler() : (paragraphAlignLeft(std::string(desc)) | flex);
  auto row = hbox({text(" "), name_col, rest});
  if (selected) return row | bold | color(Color::White) | bgcolor(selected_bg());
  return row | color(box_fg());
}

// The shaded menu panel (rows + caption), without any screen placement.
Element menu_panel(const TuiApp& app) {
  Elements rows;
  if (app.menu_mode == MenuMode::Commands) {
    for (int i = 0; i < static_cast<int>(commands.size()); ++i)
      rows.push_back(menu_row(commands[i].name, commands[i].desc, i == app.menu_selected));
  } else {
    for (int i = 0; i < static_cast<int>(hlptxt::help_map.size()); ++i)
      rows.push_back(menu_row(hlptxt::help_map[i].key, "", i == app.menu_selected));
  }
  return vbox({
             text(""),
             vbox(std::move(rows)),
             text(""),
             hbox({text("  "), text("/ ") | color(muted()),
                   text(app.menu_display.caption) | color(muted())}),
             text(""),
         }) |
         bgcolor(box_bg());
}

Element render_output(const TuiApp& app, Element input_line) {
  Elements lines;
  for (const auto& l : app.output) {
    if (l.text.empty()) {
      lines.push_back(text(""));
    } else if (l.command) {
      lines.push_back(
          hbox({accent_bar(), text(" "), paragraphAlignLeft(l.text) | bold | flex}));
    } else {
      lines.push_back(paragraphAlignLeft(l.text));
    }
  }
  Element log =
      vbox(std::move(lines)) | focusPositionRelative(0, app.scroll) | yframe | flex;
  Elements main_rows;
  main_rows.push_back(log);
  main_rows.push_back(text(""));
  if (app.menu_open) {
    // Menu appears directly above the prompt box once the prompt is at the bottom.
    main_rows.push_back(hbox({text(" "), menu_panel(app) | flex, text(" ")}));
    main_rows.push_back(text(""));
  }
  main_rows.push_back(hbox(
      {text(" "), make_prompt_box(app, std::move(input_line)) | flex, text(" ")}));
  auto main = vbox(std::move(main_rows)) | flex;
  auto side = render_sidebar(app) | size(WIDTH, EQUAL, 32);
  // a little whitespace around the shaded panels, no outlines;
  // footer (cwd / case) sits below the shaded boxes with a gap above it
  return vbox({
      hbox({text(" "), main, text("  "), side, text(" ")}) | flex,
      text(""),
      hbox({text(" "), render_footer(app) | flex, text(" ")}),
  });
}

// Centered menu, used only on the opening screen (prompt box mid-screen).
Element render_menu_centered(const TuiApp& app) {
  auto panel = menu_panel(app) | size(WIDTH, EQUAL, 76);
  return vbox({
      filler(),
      hbox({filler(), panel, filler()}),
      filler(),
      render_footer(app),
  });
}

}  // namespace

int run_tui() {
  TuiApp app;
  auto screen = ScreenInteractive::Fullscreen();

  int input_cursor = 0;

  MenuOption menu_opt;
  menu_opt.on_enter = [&] {
    if (app.menu_mode == MenuMode::Commands)
      run_command(app, app.menu_selected);
    else
      run_help_topic(app, app.menu_selected);
    if (app.quit) screen.Exit();
    // a command awaiting an argument pre-fills the prompt line;
    // put the cursor after the echoed command
    input_cursor = static_cast<int>(app.input.size());
  };
  auto menu = Menu(&app.menu_entries, &app.menu_selected, menu_opt);

  // Text input in the prompt box. For now Enter just echoes the typed line
  // into the output area; direct commands can hook in here later.
  InputOption input_opt;
  input_opt.content = &app.input;
  input_opt.placeholder = &app.prompt;
  input_opt.multiline = false;
  input_opt.cursor_position = &input_cursor;
  input_opt.on_enter = [&] {
    if (app.pending_cmd >= 0) {   // argument for a command awaiting input
      finish_pending_command(app);
      return;
    }
    if (app.input.empty()) return;
    append_output(app, app.input, true);
    append_output(app, "");
    app.input.clear();
    app.screen = AppScreen::Output;
    app.scroll = 1.0f;
  };
  input_opt.transform = [](InputState state) {
    // flat styling on the shaded box: muted placeholder, dark typed text
    if (state.is_placeholder) return state.element | color(muted());
    return state.element | color(box_fg());
  };
  auto input = Input(input_opt);

  // Route events to the input normally, to the menu while it is open.
  int active_child = 0;
  auto container = Container::Tab({input, menu}, &active_child);

  auto layout = Renderer(container, [&] {
    active_child = app.menu_open ? 1 : 0;
    Element input_line = input->Render();
    // On the output screen the menu renders inline above the prompt box
    // (inside render_output); only the opening screen centers it.
    if (app.menu_open && app.screen == AppScreen::Opening)
      return render_menu_centered(app);
    return app.screen == AppScreen::Opening
               ? render_opening(app, std::move(input_line))
               : render_output(app, std::move(input_line));
  });

  auto scroll_by = [&](float delta) {
    app.scroll = std::clamp(app.scroll + delta, 0.0f, 1.0f);
  };

  auto root = CatchEvent(layout, [&](Event e) {
    if (app.menu_open) {
      if (e == Event::Escape) {
        if (app.menu_mode == MenuMode::HelpTopics)
          open_command_menu(app);   // step back to the top-level menu
        else
          app.menu_open = false;    // close the menu
        return true;
      }
      // a letter jumps the highlight to the next matching item
      if (e.is_character() && !e.character().empty()) {
        menu_jump_to_letter(app, e.character()[0]);
        return true;
      }
      return false;  // let the Menu handle arrow keys and Enter
    }
    // Esc cancels a command that is waiting for its argument.
    if (e == Event::Escape && app.pending_cmd >= 0) {
      app.pending_cmd = -1;
      app.input.clear();
      append_output(app, "(cancelled)");
      append_output(app, "");
      return true;
    }
    // "/" on an empty prompt opens the menu; mid-text it is typed as-is.
    if (e == Event::Character('/') && app.input.empty()) {
      open_command_menu(app);
      return true;
    }
    // scroll the output log; the prompt box stays pinned at the bottom
    if (app.screen == AppScreen::Output) {
      if (e == Event::ArrowUp) { scroll_by(-0.05f); return true; }
      if (e == Event::ArrowDown) { scroll_by(0.05f); return true; }
      if (e == Event::PageUp) { scroll_by(-0.25f); return true; }
      if (e == Event::PageDown) { scroll_by(0.25f); return true; }
      if (e.is_mouse()) {
        if (e.mouse().button == Mouse::WheelUp) { scroll_by(-0.05f); return true; }
        if (e.mouse().button == Mouse::WheelDown) { scroll_by(0.05f); return true; }
      }
    }
    return false;  // everything else goes to the prompt input
  });

  screen.Loop(root);
  return 0;
}
