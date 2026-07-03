#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

// Scans a case's input/ directory, verifies structure and critical value
// invariants for all required parameter files, accumulates ALL errors, prints
// them (via {fmt}), writes "input-error-log.txt" to the current working
// directory, and calls std::exit(EXIT_FAILURE) if any errors were found.
// On success, prints a single confirmation line and returns so the caller can
// proceed. Invoked from build_model() before any loading.
void input_verify(const std::filesystem::path& input_dir);

// Pure, throw-free check helpers exposed for unit testing. Each appends
// human-readable messages to an Errors accumulator instead of throwing, so a
// single pass can report every problem. input_verify orchestrates these and
// owns the process exit / log-file side effects.
namespace input_verify_detail {

struct Errors {
  std::vector<std::string> msgs;
  void add(std::string m) { msgs.push_back(std::move(m)); }
  bool any() const { return !msgs.empty(); }
};

// |sum - 1.0| < 1e-6
bool near_one(double sum);

// Writes the validation report and closes the stream before returning. This is
// important because input_verify exits the process immediately after logging.
bool write_error_log(const std::filesystem::path& log_path, std::string_view report);

// Validates config.json keys/types/values. Writes the simulation-control flags
// and locale it parsed into the out-params (defaulting to false / 0 when a key
// is missing or the wrong type) so the orchestrator can gate conditional files.
void check_config(const json& cfg, Errors& e,
                  bool& dovax, bool& do_rings, bool& do_social_distancing,
                  int& locale);

void check_variants(const json& j, Errors& e);
void check_vaccines(const json& j, Errors& e);
void check_vax_sched(const json& j, std::string_view label, Errors& e);
void check_socialparams(const json& j, Errors& e);
void check_seed(const json& j, Errors& e);
void check_soc_dist(const json& j, Errors& e);
void check_rings(const json& j, Errors& e);

// Reads the CSV header and rows directly; verifies the required columns are
// present and that at least one row's fips matches the config locale.
void check_geodata_csv(const std::filesystem::path& p, int locale, Errors& e);

}  // namespace input_verify_detail
