#pragma once

#include "setup.h"


using json = nlohmann::ordered_json;
namespace fs = std::filesystem;

Model build_model(std::filesystem::path case_dir);

void set_project_dir(std::string val);

void show_project_dir();

fs::path read_project_dir();

void init_case(std::string case_label);

void setup_dir(std::string path_arg);

Model use_managed_case(std::string case_label);

void show_cases();

Model use_dir(std::string path_arg);

std::optional<Model> r0_sim_setup(std::string path_arg);
