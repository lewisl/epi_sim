#include <string>
#include "stubs.h"

// Trivial command stubs for the epi_sim TUI bootstrap.
// Each returns a message string that the TUI appends to its output log,
// instead of printing to stdout (which would corrupt the full-screen TUI).
// The real application will replace these with calls into src/.

// set-project-dir: use set_project_dir(<val>) in the real app.
std::string set_project_dir_stub(const std::string& val) {
  return "set-project-dir: would create project directory '" + val +
         "' to hold simulation cases.";
}

// init-case: use init_case(<val>) in the real app.
std::string init_case_stub(const std::string& val) {
  return "init-case: would create case folder '" + val +
         "' in the project and generate template input files.";
}

// use-case: use-case then run the simulation in the real app.
std::string use_case_stub(const std::string& val) {
  return "use-case: would run a simulation using case '" + val +
         "' from the project folder.";
}

// show-cases: use show_cases() in the real app.
std::string show_cases_stub() {
  return "show-cases: would list the names of case folders in the current project.";
}

// setup-dir: use setup_dir(<val>) in the real app.
std::string setup_dir_stub(const std::string& val) {
  return "setup-dir: would create a single-case folder '" + val +
         "' and generate template input files.";
}

// use-dir: use the case folder inputs and run the simulation in the real app.
std::string use_dir_stub(const std::string& val) {
  return "use-dir: would run the simulation using the inputs in case folder '" + val + "'.";
}

// list-output-files: prompt for a case, then list its output files.
std::string list_output_files_stub(const std::string& val) {
  return "list-output-files: would list the output files for the current case.";
}

// plot: show plots for the current case as tabs in your browser.
std::string plot_stub() {
  return "plot: would show plots for the current case as tabs in your browser.";
}