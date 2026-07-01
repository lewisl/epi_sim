#pragma once

#include "test_utils.h"

#include "../src/lib_includes.h"

#include "../src/parameters.h"
#include "../src/population.h"
#include "../src/pop_serialize.h"
#include "../src/traits.h"

#include <cctype>
#include <fstream>
#include <sstream>

using std::string;
using std::string_view;
using std::vector;

namespace test_support {

namespace fs = std::filesystem;

struct TestRunOptions {
  bool write_artifacts{false};
  fs::path artifact_root{"test_output"};
};

struct VariantNamesGuard {
  vector<string> saved_names = Variant::names;
  ~VariantNamesGuard() { Variant::names = saved_names; }
};

struct VaxNamesGuard {
  vector<string> saved_names = Vax::names;
  ~VaxNamesGuard() { Vax::names = saved_names; }
};

struct SDCaseNamesGuard {
  vector<string> saved_names = SDCase::names;
  ~SDCaseNamesGuard() { SDCase::names = saved_names; }
};

struct RingNamesGuard {
  vector<string> saved_names = Ring::names;
  ~RingNamesGuard() { Ring::names = saved_names; }
};

inline string rtrim_copy(string value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}

inline vector<string> split_trimmed_lines(const string& text) {
  std::istringstream input(text);
  vector<string> lines;
  string line;
  while (std::getline(input, line)) lines.push_back(rtrim_copy(line));
  return lines;
}

inline string read_file_text(const fs::path& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot read file");
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

inline fs::path artifact_group_dir(const TestRunOptions& options, std::string_view group) {
  const fs::path dir = options.artifact_root / std::string(group);
  fs::create_directories(dir);
  return dir;
}

inline void write_artifact_text(const TestRunOptions& options, std::string_view group,
                                const fs::path& filename, std::string_view content) {
  if (!options.write_artifacts) return;
  const fs::path out_path = artifact_group_dir(options, group) / filename;
  std::ofstream out(out_path);
  if (!out) {
    throw std::runtime_error(fmt::format("cannot write artifact '{}'", out_path.string()));
  }
  out << content;
}

struct SampleParamPaths {
  string geodata;
  string variants;
  string social;
  string vaccines;
  string vax_sched_dir;
};

inline fs::path project_dir() {
  const fs::path cwd = fs::current_path();
  if (fs::exists(cwd / "sample_parameters")) return cwd;
  return fs::path(std::getenv("HOME")) / "code" / "epi_sim";
}

inline fs::path home_dir() {
  const char* home = std::getenv("HOME");
  if (!home) throw std::runtime_error("HOME is not set");
  return fs::path(home);
}

inline std::string unique_name(std::string_view prefix) {
  return fmt::format("{}{}", prefix, std::random_device{}());
}

inline SampleParamPaths sample_paths() {
  const fs::path root = project_dir();
  return {
      .geodata = (root / "sample_parameters" / "geo2data.csv").string(),
      .variants = (root / "sample_parameters" / "variants.json").string(),
      .social = (root / "sample_parameters" / "socialparams.json").string(),
      .vaccines = (root / "sample_parameters" / "vaccines.json").string(),
      .vax_sched_dir = (root / "sample_parameters" / "vaccine_100k").string(),
  };
}

inline size_t require_locale_index(const GeoData& geodata, int locale) {
  const auto it = std::find(geodata.fips.begin(), geodata.fips.end(), locale);
  REQUIRE(it != geodata.fips.end());
  return static_cast<size_t>(std::distance(geodata.fips.begin(), it));
}

inline const VaxParams& require_vax(const VaxSet& vaxset, string_view name) {
  const auto it = std::find(Vax::names.begin(), Vax::names.end(), name);
  REQUIRE(it != Vax::names.end());
  return vaxset.at(Vax{static_cast<uint8_t>(std::distance(Vax::names.begin(), it))});
}

inline const PerVaxSpec& require_sched_vax(const VaxSched& sched, string_view name) {
  const auto it = std::find_if(sched.vaxesincluded.begin(), sched.vaxesincluded.end(),
                               [name](const auto& entry) { return entry.vax.show() == name; });
  REQUIRE(it != sched.vaxesincluded.end());
  return *it;
}

inline const VaxSched& require_sched(const VaxSchedSet& schedset, string_view name) {
  const auto it = std::find_if(schedset.schedules.begin(), schedset.schedules.end(),
                               [name](const auto& entry) { return entry.first == name; });
  REQUIRE(it != schedset.schedules.end());
  return it->second;
}

inline float require_named_factor(const vector<std::pair<string, float>>& entries, string_view name) {
  const auto it = std::find_if(entries.begin(), entries.end(),
                               [name](const auto& entry) { return entry.first == name; });
  REQUIRE(it != entries.end());
  return it->second;
}

inline PopData make_popdata_fixture() {
  Variant::names = {"none", "alpha", "delta"};
  Vax::names = {"none", "pfizer", "moderna"};

  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});

  pop.agegrp[1] = AGE20_39;
  pop.agegrp[2] = AGE40_59;
  pop.agegrp[3] = AGE80_UP;

  pop.status[1] = RECOVERED;
  pop.cond[1] = UNINFECTED;
  pop.variant[1] = Variant{1};
  pop.variant_hist[1].arr[0] = Variant{1};
  pop.variant_hist[1].count = 1;
  pop.sickday[1] = 2;
  pop.sickday_hist[1].arr[0] = 2;
  pop.sickday_hist[1].count = 1;
  pop.recovday[1] = 9;
  pop.recovday_hist[1].arr[0] = 9;
  pop.recovday_hist[1].count = 1;

  pop.status[2] = INFECTIOUS;
  pop.cond[2] = MILD;
  pop.duration[2] = 5;
  pop.ring[2] = 3;
  pop.quar[2] = 1;
  pop.quarday[2] = 8;
  pop.variant[2] = Variant{2};
  pop.variant_hist[2].arr[0] = Variant{1};
  pop.variant_hist[2].arr[1] = Variant{2};
  pop.variant_hist[2].count = 2;
  pop.sickday[2] = 11;
  pop.sickday_hist[2].arr[0] = 4;
  pop.sickday_hist[2].arr[1] = 11;
  pop.sickday_hist[2].count = 2;
  pop.testday[2] = 12;
  pop.testday_hist[2].arr[0] = 6;
  pop.testday_hist[2].arr[1] = 12;
  pop.testday_hist[2].count = 2;
  pop.vaxstatus[2] = Vaxstat::booster;
  pop.vax[2] = Vax{2};
  pop.vax_hist[2].arr[0] = Vax{1};
  pop.vax_hist[2].arr[1] = Vax{2};
  pop.vax_hist[2].count = 2;
  pop.vaxday[2] = 14;
  pop.vaxday_hist[2].arr[0] = 5;
  pop.vaxday_hist[2].arr[1] = 14;
  pop.vaxday_hist[2].count = 2;

  pop.status[3] = UNEXPOSED;
  pop.cond[3] = UNINFECTED;

  return pop;
}

}  // namespace test_support
