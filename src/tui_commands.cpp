#include "tui_app.h"
#include "tui_commands.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "param_init.h"
#include "r0_simulation.h"
#include "show_help.h"
#include "sim.h"

namespace {

enum class CommandAction {
  Help,
  SetProjectDir,
  InitCase,
  RunCase,
  ShowCases,
  SetupDir,
  RunDir,
  R0Sim,
  Plot,
  Quit,
};

struct Command {
  std::string name;
  std::string desc;
  std::string ask;
  CommandAction action;
};

struct AppState {
  bool quit = false;
  std::optional<Model> active_model;
  std::optional<AllSeries> result_series;
  std::string current_case_label;
  std::filesystem::path current_case_dir;
  std::filesystem::path last_output_dir;
};

const std::vector<Command> commands = {
    {"/help", "Browse help topics", "", CommandAction::Help},
    {"/set-project-dir", "Create or activate a project directory",
      "Project directory path:", CommandAction::SetProjectDir},
    {"/init-case", "Create a case folder in the active project",
      "New case name:", CommandAction::InitCase},
    {"/run-case", "Run a project case and keep the model alive",
      "Case name:", CommandAction::RunCase},
    {"/show-cases", "Show case folders in the active project", "",
      CommandAction::ShowCases},
    {"/setup-dir", "Create a standalone case folder",
      "Case directory path:", CommandAction::SetupDir},
    {"/run-dir", "Run a standalone case directory and keep the model alive",
      "Case directory path:", CommandAction::RunDir},
    {"/r0_sim", "Simulate academic definition of R0", "Case name or case directory: ",
      CommandAction::R0Sim},
    {"/plot", "Show where the last run plot files were written", "",
      CommandAction::Plot},
    {"/q", "Quit epi_sim", "", CommandAction::Quit},
};



std::vector<std::string> command_menu_entries() {
  std::vector<std::string> entries;
  entries.reserve(commands.size());
  for (const auto& command : commands) {
    entries.push_back(fmt::format("{:<20}{}", command.name, command.desc));
  }
  return entries;
}

void print_state_summary(const AppState& state) {
  if (!state.current_case_label.empty()) {
    fmt::println("Current case: {}", state.current_case_label);
  }
  if (!state.current_case_dir.empty()) {
    fmt::println("Current case directory: {}", state.current_case_dir.string());
  }
  if (!state.last_output_dir.empty()) {
    fmt::println("Last output directory: {}", state.last_output_dir.string());
  }
}


// 
// menu invoked user commands
//

bool run_help_topics() {
  bool printed_help = false;

  while (true) {
    std::vector<std::string> entries;
    entries.reserve(hlptxt::help_map.size() + 1);
    for (const auto& topic : hlptxt::help_map) {
      entries.emplace_back(topic.key);
    }
    entries.emplace_back("/back");

    auto selected = choose_menu("Help topics", entries);
    if (!selected) return printed_help;
    if (*selected == static_cast<int>(entries.size()) - 1) {
      clear_menu_widget(entries.size());
      return printed_help;
    }

    fmt::println("");
    show_help(hlptxt::help_map[*selected].key);
    fmt::println("");
    printed_help = true;
  }
}


// TODO:  do we need this?  maybe we can do a run plot that lists available plots from the current case's output dir
void show_plot_files(const AppState& state) {
  if (state.last_output_dir.empty()) {
    fmt::println("No run output directory is available yet.");
    return;
  }
  if (!std::filesystem::exists(state.last_output_dir)) {
    fmt::println("Last output directory does not exist: {}",
                 state.last_output_dir.string());
    return;
  }

  fmt::println("Plot files from the last run:");
  int count = 0;
  for (const auto& entry : std::filesystem::directory_iterator(state.last_output_dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".html") continue;
    fmt::println("  {}", entry.path().string());
    ++count;
  }
  if (count == 0) fmt::println("  No plot HTML files found.");
}


void run_case(AppState& state, const std::string& case_label) {
  state.active_model.reset();
  state.result_series.reset();
  state.active_model.emplace(use_managed_case(case_label));
  state.result_series = runsim(*state.active_model);   // should this be a move or will elision happen?   make sure to test for existence before using
  state.current_case_label = case_label;
  state.current_case_dir.clear();
  state.last_output_dir = state.active_model->output_dir;
  print_state_summary(state);
}

// do not use current AppState here:  this is a detached read-only analysis that uses case inputs
//      but does NOT mutate any case state
void r0sim(const std::string& case_label) {   
  auto model = r0_sim_setup(case_label);
  if (!model) return;
  double r0 = r0_sim(*model);  // *state.active_model
  fmt::println("r0 estimate: {:.2f}", r0);
}

void run_dir(AppState& state, const std::string& path_arg) {
  state.active_model.reset();
  state.result_series.reset();
  state.active_model.emplace(use_dir(path_arg));
  state.result_series = runsim(*state.active_model);
  state.current_case_label = state.active_model->case_label;
  state.current_case_dir = path_arg;
  state.last_output_dir = state.active_model->output_dir;
  print_state_summary(state);
}

//
// dispatch command from user's menu choice
//
void dispatch_command(AppState& state, const Command& command) {
  if (command.action == CommandAction::Quit) {
    state.quit = true;
    return;
  }

  std::string arg;
  if (!command.ask.empty()) {
    auto text_arg = ask_text(command.ask);
    if (!text_arg) {
      fmt::println("{} cancelled.", command.name);
      return;
    }
    arg = *text_arg;
    if (arg.empty()) {
      fmt::println("No value provided for {}.", command.name);
      return;
    }
  }

  std::string command_text = command.name;
  if (!arg.empty()) command_text = fmt::format("{} {}", command.name, arg);
  print_command_card(command_text);
  print_output_boundary(false);

  try {
    switch (command.action) {
      case CommandAction::Help:
        run_help_topics();
        break;
      case CommandAction::SetProjectDir:
        set_project_dir(arg);
        break;
      case CommandAction::InitCase:
        init_case(arg);
        break;
      case CommandAction::RunCase:
        run_case(state, arg);
        break;
      case CommandAction::ShowCases:
        show_cases();
        break;
      case CommandAction::SetupDir:
        setup_dir(arg);
        state.current_case_dir = arg;
        break;
      case CommandAction::RunDir:
        run_dir(state, arg);
        break;
      case CommandAction::R0Sim:
        r0sim(arg);    
        break;
      case CommandAction::Plot:
        show_plot_files(state);
        break;
      case CommandAction::Quit:
        break;
    }
  } catch (const std::exception& e) {
    fmt::println(stderr, "Command failed: {}", e.what());
  }

  print_output_boundary(true);
}

}  // namespace

int run_terminal_tui() {
  AppState state;
  fmt::println("");

  while (!state.quit) {
    auto selected = choose_menu("Choose a command", command_menu_entries(),
                                state.current_case_label);
    if (!selected) continue;

    dispatch_command(state, commands[*selected]);
  }

  clear_terminal_lines(static_cast<int>(commands.size()) + 6);
  return 0;
}
