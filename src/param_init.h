#pragma once
#include "helpers.h"
#include <cctype>
#include <cstdlib>
#include <vector>
#include <string>
#include "cases.h"
#include "epi_sim.h"
#include "setup.h"
#include "sim.h"
#include <absl/strings/str_split.h>
#include <toml++/toml.hpp>
#include "parameters.h"



std::filesystem::path write_file(const std::string& content, std::string filename, std::string extension,
  std::vector<std::string> path_steps);
  
void create_scaffold(fs::path case_dir);

void set_project_dir(std::string val);

void show_project_dir();

void init_case(std::string case_label);

fs::path use_case(std::string case_label);