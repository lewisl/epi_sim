#pragma once

#include "lib_includes.h"

#include "parameters.h"
#include "population.h"
#include "sim.h"

using series_type = absl::flat_hash_map<string, vector<float>>;

struct Model {
  int ndays{};
  absl::CivilDay day1{};
  vector<absl::CivilDay> caldays;
  series_type series;
  vector<float> indoor_seq;
  int locale{};
  bool dovax{};
  bool debug{};
  ModelParams mp{};
  PopData pop;
};


series_type build_series(int n_days, absl::CivilDay day1,
                         vector<string> series_colnames); 

vector < absl::CivilDay >build_caldays(int n_days, absl::CivilDay day1);

ModelParams setup_model_params(bool dovax, string geo_path,
                               string variants_path, string social_path,
                               string vax_path, string vaxsched_dir);

// don't put parameters defaults in the definition of the function in .cpp--only the types and names
// Model setup_sim(int ndays, int locale,  // require inputs
//     string date = "2020-01-01",   // all the rest have defaults...
//     bool dovax = false,

Model setup_sim(Config config);
