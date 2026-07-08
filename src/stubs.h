#include <string>

// Trivial command stubs for the epi_sim TUI bootstrap.
// Each returns a message string that the TUI appends to its output log,
// instead of printing to stdout (which would corrupt the full-screen TUI).
// The real application will replace these with calls into src/.

// set-project-dir: use set_project_dir(<val>) in the real app.
std::string set_project_dir_stub(const std::string& val); 

// init-case: use init_case(<val>) in the real app.
std::string init_case_stub(const std::string& val); 

// use-case: use-case then run the simulation in the real app.
std::string use_case_stub(const std::string& val); 

// show-cases: use show_cases() in the real app.
std::string show_cases_stub(); 

// setup-dir: use setup_dir(<val>) in the real app.
std::string setup_dir_stub(const std::string& val); 

// use-dir: use the case folder inputs and run the simulation in the real app.
std::string use_dir_stub(const std::string& val); 

// list-output-files: prompt for a case, then list its output files.
std::string list_output_files_stub(const std::string& val); 

// plot: show plots for the current case as tabs in your browser.
std::string plot_stub(); 