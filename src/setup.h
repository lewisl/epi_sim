#pragma once

#include "lib_includes.h"

#include "parameters.h"
#include "population.h"

struct Model {
  int ndays{};
  absl::CivilDay day1{};
  int locale{};
  bool dovax{};
  ModelParams mp{};
  PopData pop;
  // later add historyseq, history_columns
};

namespace fs = std::filesystem;

ModelParams setup_model_params(bool dovax, string geo_path,
                               string variants_path, string social_path,
                               string vax_path, string vaxsched_path);

// don't put parameters defaults in the definition of the function in .cpp--only the types and names
Model setup_sim(int ndays, int locale,  // require inputs
    string date = "2020-01-01",   // all the rest have defaults...
    bool dovax = false,
    const fs::path& project_dir = fs::path(std::getenv("HOME")) / "code" / "epi_sim",
    const fs::path& paramdir = "sample_parameters",
    const fs::path& geodata_fname = "geo2data.csv",
    const fs::path& param_dir = "sample_parameters",
    const fs::path& variants_fname = "variants.json",
    const fs::path& social_fname = "SocialParams.json",
    const fs::path& vax_fname = "vaccines.json",
    const fs::path& vax_sched_dir = "vaccine_100k",
    const fs::path& vax_sched_fname = "loc38015_old.json");