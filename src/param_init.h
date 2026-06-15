#pragma once
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <string>
#include "setup.h"
#include <absl/strings/str_split.h>
#include <toml++/toml.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;


Model build_model(std::filesystem::path case_dir);

std::optional<std::filesystem::path> resolve_home_path(const std::string& path_str);

std::filesystem::path resolve_config_path(const std::filesystem::path& config_dir, const json& config_json, const char* key);

std::string resolve_optional_config_path(const std::filesystem::path& config_dir, const json& config_json, const char* key);

std::filesystem::path read_project_dir();

void write_file(const std::string& content, std::string filename, std::string extension,
  std::filesystem::path path_name);

std::filesystem::path config_path_for_case_dir(const std::filesystem::path& case_dir);

std::filesystem::path resolve_explicit_case_dir(std::string path_arg);

void create_scaffold(std::filesystem::path case_dir);

void set_project_dir(std::string val);

void show_project_dir();

void init_case(std::string case_label);

void setup_dir(std::string path_arg);

Model use_managed_case(std::string case_label);

void show_cases();

Model use_dir(std::string path_arg);

std::optional<Model> r0_sim_setup(std::string path_arg);
