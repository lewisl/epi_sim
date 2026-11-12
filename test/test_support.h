#pragma once

#include "test_utils.h"

#include "../src/param_init.h"
#include "../src/parameters.h"
#include "../src/population.h"
#include "../src/series.h"
#include "../src/setup.h"
#include "../src/sim.h"
#include "../src/template.h"
#include "../src/traits.h"

#include "../src/input_verify.h"

#include <cctype>
#include <cstdlib>
#include <fcntl.h>
#include <functional>
#include <sys/wait.h>
#include <fstream>
#include <random>
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

//
// runtime trait registries: tests change them, so save and restore names AND counts
//
struct RuntimeNamesGuard {
  vector<string> variant_names = Variant::names;
  uint16_t variant_count = Variant::count;
  vector<string> vax_names = Vax::names;
  uint16_t vax_count = Vax::count;
  vector<string> ring_names = Ring::names;
  uint16_t ring_count = Ring::count;
  vector<string> sdcase_names = SDCase::names;
  ~RuntimeNamesGuard() {
    Variant::names = variant_names;
    Variant::count = variant_count;
    Vax::names = vax_names;
    Vax::count = vax_count;
    Ring::names = ring_names;
    Ring::count = ring_count;
    SDCase::names = sdcase_names;
  }
};
// older names kept so existing tests read naturally
using VariantNamesGuard = RuntimeNamesGuard;
using VaxNamesGuard = RuntimeNamesGuard;
using SDCaseNamesGuard = RuntimeNamesGuard;
using RingNamesGuard = RuntimeNamesGuard;

// Installs runtime names the way the loaders do: zero slot first, counts = real entries.
inline void install_names(const vector<string>& variants, const vector<string>& vaxes = {},
                          const vector<string>& rings = {"population"},
                          const vector<string>& sdcases = {}) {
  Variant::names = {"none"};
  Variant::names.insert(Variant::names.end(), variants.begin(), variants.end());
  Variant::count = static_cast<uint16_t>(variants.size());
  Vax::names = {"none"};
  Vax::names.insert(Vax::names.end(), vaxes.begin(), vaxes.end());
  Vax::count = static_cast<uint16_t>(vaxes.size());
  Ring::names = {"error"};
  Ring::names.insert(Ring::names.end(), rings.begin(), rings.end());
  Ring::count = static_cast<uint16_t>(rings.size());
  SDCase::names = {"none"};
  SDCase::names.insert(SDCase::names.end(), sdcases.begin(), sdcases.end());
}

// equal age split; with ring_count > 1 people are assigned round-robin to rings
inline PopData make_pop(size_t n, size_t ring_count = 1) {
  PopData pop(n, {0.2, 0.2, 0.2, 0.2, 0.2});
  for (size_t p = 1; p <= n; ++p) pop.ring[p] = Ring{(p - 1) % ring_count + 1};
  return pop;
}

inline string rtrim_copy(string value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
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
  if (!in) throw std::runtime_error(fmt::format("cannot read file '{}'", path.string()));
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

inline void write_file_text(const fs::path& path, std::string_view text) {
  if (path.has_parent_path()) fs::create_directories(path.parent_path());
  std::ofstream out(path);
  if (!out) throw std::runtime_error(fmt::format("cannot write file '{}'", path.string()));
  out << text;
}

inline fs::path project_dir() {
  const fs::path cwd = fs::current_path();
  if (fs::exists(cwd / "xmake.lua")) return cwd;
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

inline fs::path artifact_group_dir(const TestRunOptions& options, std::string_view group) {
  const fs::path dir = options.artifact_root / std::string(group);
  fs::create_directories(dir);
  return dir;
}

inline void write_artifact_text(const TestRunOptions& options, std::string_view group,
                                const fs::path& filename, std::string_view content) {
  if (!options.write_artifacts) return;
  write_file_text(artifact_group_dir(options, group) / filename, content);
}

//
// plots call `open` to launch a browser; put a no-op `open` first on PATH for this process
//
inline void suppress_browser() {
  static const bool done = [] {
    const fs::path bin = fs::temp_directory_path() / unique_name("epi_sim_test_bin_");
    fs::create_directories(bin);
    write_file_text(bin / "open", "#!/bin/sh\nexit 0\n");
    fs::permissions(bin / "open", fs::perms::owner_all);
    const char* old = std::getenv("PATH");
    const std::string path = fmt::format("{}:{}", bin.string(), old ? old : "");
    ::setenv("PATH", path.c_str(), 1);
    return true;
  }();
  (void)done;
}

//
// stdout capture (fmt and iostream both write to fd 1)
//
struct StdoutCapture {
  fs::path file = fs::temp_directory_path() / unique_name("epi_sim_stdout_");
  int saved{-1};
  StdoutCapture() {
    std::fflush(stdout);
    saved = ::dup(STDOUT_FILENO);
    const int fd = ::open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ::dup2(fd, STDOUT_FILENO);
    ::close(fd);
  }
  std::string finish() {
    if (saved >= 0) {
      std::fflush(stdout);
      ::dup2(saved, STDOUT_FILENO);
      ::close(saved);
      saved = -1;
    }
    std::string text = read_file_text(file);
    std::error_code ec;
    fs::remove(file, ec);
    return text;
  }
  ~StdoutCapture() {
    if (saved >= 0) (void)finish();
  }
};

//
// golden fixtures under test/fixtures/<rel>. On mismatch the actual text is written to
// test_output/<group>/actual_<file> and the first differing line is printed. To accept a
// new golden after an intended change, copy that actual file over the fixture.
//
inline bool compare_golden(const std::string& actual, const fs::path& rel_fixture,
                           const TestRunOptions& options, std::string_view group) {
  const fs::path fixture = project_dir() / "test" / "fixtures" / rel_fixture;
  const fs::path actual_path =
      artifact_group_dir(options, group) / fmt::format("actual_{}", rel_fixture.filename().string());
  if (!fs::exists(fixture)) {
    write_file_text(actual_path, actual);
    CHECK_MSG(false, "golden fixture '{}' missing; actual output written to '{}'",
              fixture.string(), actual_path.string());
    return false;
  }
  const auto want = split_trimmed_lines(read_file_text(fixture));
  const auto got = split_trimmed_lines(actual);
  size_t i = 0;
  while (i < want.size() && i < got.size() && want[i] == got[i]) ++i;
  const bool same = i == want.size() && i == got.size();
  if (!same || options.write_artifacts) write_file_text(actual_path, actual);
  CHECK_MSG(same,
            "output differs from golden '{}' at line {}:\n         want: {}\n         got:  {}\n"
            "       actual written to '{}' (copy it over the fixture if the change is intended)",
            rel_fixture.string(), i + 1, i < want.size() ? want[i] : "<end>",
            i < got.size() ? got[i] : "<end>", actual_path.string());
  return same;
}

// Materializes the scaffold template strings (src/template.cpp) into a temp directory.
inline const fs::path& template_params_dir() {
  static const fs::path dir = [] {
    const fs::path root = fs::temp_directory_path() / unique_name("epi_sim_test_params_");
    const fs::path vax_dir = root / "vaccine_100k";
    fs::create_directories(vax_dir);
    write_file_text(root / "geodata.csv", geodata_csv);
    write_file_text(root / "variants.json", variants_json);
    write_file_text(root / "socialparams.json", socialparams_json);
    write_file_text(root / "vaccines.json", vaccines_json);
    write_file_text(root / "seed.json", seed_json);
    write_file_text(root / "soc_dist.json", social_dist_json);
    write_file_text(root / "rings.json", rings_json);
    write_file_text(vax_dir / "loc38015_old.json", vaxsched::loc38015_old_json);
    write_file_text(vax_dir / "loc38015_young.json", vaxsched::loc38015_young_json);
    return root;
  }();
  return dir;
}

struct SampleParamPaths {
  string geodata, variants, social, vaccines, vax_sched_dir, seed, social_dist, rings;
};

inline SampleParamPaths sample_paths() {
  const fs::path& root = template_params_dir();
  return {
      .geodata = (root / "geodata.csv").string(),
      .variants = (root / "variants.json").string(),
      .social = (root / "socialparams.json").string(),
      .vaccines = (root / "vaccines.json").string(),
      .vax_sched_dir = (root / "vaccine_100k").string(),
      .seed = (root / "seed.json").string(),
      .social_dist = (root / "soc_dist.json").string(),
      .rings = (root / "rings.json").string(),
  };
}

// Config built directly from the template files (no input_verify, no case dir)
inline Config sample_config() {
  const auto paths = sample_paths();
  Config config;
  config.days = 180;
  config.locale = 38015;
  config.calendar_start = "2020-01-01";
  config.dovax = false;
  config.age_dist = {0.251, 0.271, 0.255, 0.184, 0.039};
  config.seed = paths.seed;
  config.geodata = paths.geodata;
  config.variants = paths.variants;
  config.social_params = paths.social;
  config.social_dist = paths.social_dist;
  config.vaccines = paths.vaccines;
  config.vax_sched_dir = paths.vax_sched_dir;
  config.rings = paths.rings;
  return config;
}

//
// scaffolded case directories (the real TUI/CLI flow). Created under
// test_output/tmp so they are inside HOME (required by resolve_home_path).
//
inline fs::path make_case_dir(std::string_view name) {
  const fs::path dir = project_dir() / "test_output" / "tmp" / unique_name(fmt::format("{}_", name));
  setup_dir(dir.string());
  return dir;
}

inline json read_case_json(const fs::path& case_dir, const fs::path& rel) {
  return load_json_params((case_dir / "input" / rel).string());
}

inline void write_case_json(const fs::path& case_dir, const fs::path& rel, const json& j) {
  write_file_text(case_dir / "input" / rel, j.dump(2));
}

inline void edit_config(const fs::path& case_dir, const std::function<void(json&)>& fn) {
  json cfg = read_case_json(case_dir, "config.json");
  fn(cfg);
  write_case_json(case_dir, "config.json", cfg);
}

// Adds a locale with the Bismarck density (so the density index is unchanged) and a
// small population, and points config.locale at it.
inline constexpr int SMALL_LOCALE = 99001;
inline void use_small_locale(const fs::path& case_dir, int popn) {
  std::ofstream out(case_dir / "input" / "geodata.csv", std::ios::app);
  out << fmt::format("{},Testcounty,Testville,TS,3,{},2157,2020-02-01,0001-09-15,0002-05-30\n",
                     SMALL_LOCALE, popn);
  out.close();
  edit_config(case_dir, [](json& c) { c["locale"] = SMALL_LOCALE; });
}

// Moves every vaccine schedule's dayrange to [first, last] so short runs vaccinate.
inline void move_vax_schedules(const fs::path& case_dir, int first, int last) {
  for (const auto& entry : fs::directory_iterator(case_dir / "input" / "vaccine_100k")) {
    json s = load_json_params(entry.path().string());
    s["dayrange"] = {first, last};
    write_file_text(entry.path(), s.dump(2));
  }
}

// Runs fn in a forked child and returns its exit code (for code that calls std::exit).
template <typename Fn>
int child_exit_code(Fn fn) {
  std::fflush(stdout);
  std::fflush(stderr);
  const pid_t pid = fork();
  if (pid < 0) throw std::runtime_error("fork failed");
  if (pid == 0) {
    try { fn(); } catch (...) { _exit(70); }
    _exit(EXIT_SUCCESS);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) != pid) throw std::runtime_error("waitpid failed");
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
  return 255;
}

// input_verify exits the process on bad input; check it in a child first so a broken
// case fails this test instead of ending the whole test run.
inline void require_inputs_verify(const fs::path& case_dir) {
  const int rc = child_exit_code([&] {
    fs::current_path(case_dir);  // input-error-log.txt is written to the cwd
    StdoutCapture quiet;
    input_verify(case_dir / "input");
  });
  if (rc != 0)
    throw std::runtime_error(fmt::format("input_verify rejected '{}' (see {}/input-error-log.txt)",
                                         case_dir.string(), case_dir.string()));
}

// Scaffold -> small locale -> config/input edits -> use_dir (the TUI/CLI path).
// The model is headless (no files, no plots). The case dir is removed after setup.
inline Model build_small_case(std::string_view name, int popn, int days,
                              const std::function<void(const fs::path&)>& edit = {}) {
  const fs::path dir = make_case_dir(name);
  use_small_locale(dir, popn);
  edit_config(dir, [&](json& c) { c["days"] = days; });
  if (edit) edit(dir);
  require_inputs_verify(dir);
  Model m = [&] {
    StdoutCapture quiet;
    return use_dir(dir.string());
  }();
  m.headless = true;
  std::error_code ec;
  fs::remove_all(dir, ec);
  return m;
}

// runsim + quiet stdout; returns histories
inline Histories run_quiet(Model& m) {
  StdoutCapture quiet;
  return runsim(m);
}

inline void remove_case_dir(const fs::path& dir, const TestRunOptions& options) {
  if (options.write_artifacts) return;  // keep for inspection
  std::error_code ec;
  fs::remove_all(dir, ec);
}

//
// lookups
//
inline size_t require_locale_index(const GeoData& geodata, int locale) {
  const auto it = std::find(geodata.fips.begin(), geodata.fips.end(), locale);
  REQUIRE(it != geodata.fips.end());
  return static_cast<size_t>(std::distance(geodata.fips.begin(), it));
}

inline const PerVaxSpec& require_sched_vax(const VaxSched& sched, string_view name) {
  const auto it = std::find_if(sched.vaxesincluded.begin(), sched.vaxesincluded.end(),
                               [name](const auto& entry) { return entry.vax.show() == name; });
  REQUIRE(it != sched.vaxesincluded.end());
  return *it;
}

// 5-coordinate history accessors
inline HistoryValue atomic(const Histories& h, TraitID trait, Phase phase, uint8_t val,
                           Agegrp age, uint8_t ring, size_t day) {
  return h.at(trait, phase, val, age, ring)[day];
}

// totals use sim_totals_map (call h.sum_total_histories() first)
inline HistoryValue total(const Histories& h, TraitID trait, Phase phase, uint8_t val,
                          size_t day, uint8_t ring = RING_ALL, uint8_t age = HISTORY_AGE_TOTAL) {
  return h.history_vectors_[h.history_vector_idx(trait, phase, val, Agegrp{age}, ring, true)]
      .data[day];
}

//
// shared PopData fixture for serialization tests
//
inline PopData make_popdata_fixture() {
  install_names({"alpha", "delta"}, {"pfizer", "moderna"}, {"population", "r2", "r3"},
                {"distancing"});

  PopData pop(5, {0.2, 0.2, 0.2, 0.2, 0.2});

  pop.agegrp[1] = AGE20_39;
  pop.agegrp[2] = AGE40_59;
  pop.agegrp[3] = AGE80_UP;

  pop.status[1] = RECOVERED;
  pop.cond[1] = UNINFECTED;
  pop.variant[1] = Variant{1};
  pop.variant_hist[1].set(Variant{1});
  pop.sickday[1] = 2;
  pop.sickday_hist[1].set(2);
  pop.recovday[1] = 9;
  pop.recovday_hist[1].set(9);

  pop.status[2] = INFECTIOUS;
  pop.cond[2] = MILD;
  pop.duration[2] = 5;
  pop.ring[2] = Ring{3};
  pop.quar[2] = 1;
  pop.quarday[2] = 8;
  pop.variant[2] = Variant{2};
  pop.variant_hist[2].set(Variant{1});
  pop.variant_hist[2].set(Variant{2});
  pop.sickday[2] = 11;
  pop.sickday_hist[2].set(4);
  pop.sickday_hist[2].set(11);
  pop.testday[2] = 12;
  pop.testday_hist[2].set(6);
  pop.testday_hist[2].set(12);
  pop.vaxstatus[2] = Vaxstat::booster;
  pop.vax[2] = Vax{2};
  pop.vax_hist[2].set(Vax{1});
  pop.vax_hist[2].set(Vax{2});
  pop.vaxday[2] = 14;
  pop.vaxday_hist[2].set(5);
  pop.vaxday_hist[2].set(14);

  pop.status[3] = UNEXPOSED;
  pop.cond[3] = UNINFECTED;

  pop.status[4] = DEAD;
  pop.deadday[4] = 22;
  pop.sdcase[4] = SDCase{1};

  return pop;
}

}  // namespace test_support
