#pragma once

#include "lib_includes.h"

#include "parameters.h"
#include "population.h"
#include "sim.h"

struct Model {
  int ndays{};
  absl::CivilDay day1{};
  vector<absl::CivilDay> caldays;
  vector<float> indoor_seq;
  int locale{};
  bool dovax{};
  bool debug{};
  bool headless{false};  // when true, runsim skips disk writes and browser plots; set by tests
  ModelParams mp{};
  PopData pop;
  std::vector<std::vector<size_t>> ring_members;  // [ring][i] -> 1-based person id; outer 1-indexed
};


vector < absl::CivilDay >build_caldays(int n_days, absl::CivilDay day1);

ModelParams setup_model_params(bool dovax, string geo_path,
                               string variants_path, string social_path,
                               string vax_path, string vaxsched_dir,
                               string rings_path = "");

void assign_rings(PopData& pop, const std::vector<float>& pct_of_population);

std::vector<std::vector<size_t>> build_ring_members(const PopData& pop, size_t ring_count);

Model setup_sim(Config config);
