#include "lib_includes.h"
#include "epi_sim.h"
#include <absl/strings/str_split.h>
#include "param_init.h"
#include "show_help.h"
#include <cstdlib>

int main(int argc, char** argv) {

  std::string config_path;
  std::string seed_path;
  std::string sd_seed_path;
  
  std::string flag;
  std::string val;


  if (argc == 1) show_help(); 

  
  for (int i = 1; i < argc; i += 2) {
      flag = argv[i];
      val  = (i + 1) < argc ? argv[i + 1] : "";

      if (flag == "--set-project-dir") {
        if (val.empty()) { std::fprintf(stderr, "No value for project dir provided.\n"); std::exit(EXIT_FAILURE); }
        set_project_dir(val);
        exit(0);
      } 

      else if (flag == "--show-project-dir") {
        show_project_dir();
        exit(0);
      }

      else if (flag == "--init-case") {
        if (val.empty()) { std::fprintf(stderr, "No value for case dir provided.\n"); std::exit(EXIT_FAILURE); }
        init_case(val);
        exit(0);
      }

      else if (flag == "--use-case") {
        if (val.empty()) { std::fprintf(stderr, "No value for case dir provided.\n"); std::exit(EXIT_FAILURE); }
        run_managed_case(val);
        exit(0);
      }

      else if (flag == "--setup-dir") {
        if (val.empty()) { std::fprintf(stderr, "No value for case dir provided.\n"); std::exit(EXIT_FAILURE); }
        setup_dir(val);
        exit(0);
      }

      else if (flag == "--use-dir") {
        if (val.empty()) { std::fprintf(stderr, "No value for case dir provided.\n"); std::exit(EXIT_FAILURE); }
        use_dir(val);
        exit(0);
      }

      else if (flag == "--help") {show_help(); exit(0);}

      else {
          fmt::println(stderr, "Unknown flag: {}", flag);
          exit(1);
      }
  }
  return 0;
}

  
