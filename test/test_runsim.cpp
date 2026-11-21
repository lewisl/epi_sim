// runsim (deep tier, run explicitly): full scenarios through the same path as the TUI's
// /run-dir and the CLI's --use-dir: setup_dir -> input_verify -> use_dir -> runsim ->
// post_simulation. Each scenario's case directory is kept under test_output/runsim/<name>.
//
// Checks per scenario:
//   - whole-run invariants (test_invariants.h)
//   - the output files exist and agree with the in-memory histories / PopData
//   - golden comparison of the terminal summary table and the totals CSV
//     (test/fixtures/runsim/<name>_*). After an intended model change, copy
//     test_output/runsim/actual_* over the fixtures.
#include "test_support.h"
#include "test_invariants.h"

#include "../src/random.h"

namespace {

constexpr std::string_view GROUP = "runsim";
namespace fs = std::filesystem;
using test_support::RuntimeNamesGuard;

vector<string> split(const string& s, char sep) {
  vector<string> out;
  std::stringstream ss(s);
  string item;
  while (std::getline(ss, item, sep)) out.push_back(item);
  return out;
}

// the single output file whose name contains `tag`
fs::path one_output(const fs::path& out_dir, std::string_view tag, std::string_view ext) {
  vector<fs::path> hits;
  for (const auto& e : fs::directory_iterator(out_dir))
    if (e.path().filename().string().contains(tag) && e.path().extension() == ext) hits.push_back(e.path());
  CHECK_MSG(hits.size() == 1, "expected one '{}{}' output in {}, found {}", tag, ext, out_dir.string(), hits.size());
  return hits.empty() ? fs::path{} : hits.front();
}

// lines of print_summary's table, from the header through the "Remaining" note
string summary_block(const string& terminal) {
  const auto lines = test_support::split_trimmed_lines(terminal);
  string out;
  bool in = false;
  for (const auto& l : lines) {
    if (l.starts_with("Age Group")) in = true;
    if (in) out += l + "\n";
    if (in && l.starts_with("(Note:")) break;
  }
  return out;
}

struct ScenarioStats {
  long infected{0};
  long dead{0};
  double cfr_0_19{0.0};
  double cfr_80_up{0.0};
};

ScenarioStats compute_stats(const PopData& pop) {
  ScenarioStats s;
  long inf_young = 0, dead_young = 0, inf_old = 0, dead_old = 0;
  for (size_t p = 1; p <= pop.popn; ++p) {
    const bool ever_inf = pop.variant_hist[p].count > 0;
    const bool is_dead = pop.status[p] == DEAD;
    s.infected += ever_inf;
    s.dead += is_dead;
    if (pop.agegrp[p] == AGE0_19) { inf_young += ever_inf; dead_young += is_dead; }
    if (pop.agegrp[p] == AGE80_UP) { inf_old += ever_inf; dead_old += is_dead; }
  }
  s.cfr_0_19 = inf_young > 0 ? double(dead_young) / double(inf_young) : 0.0;
  s.cfr_80_up = inf_old > 0 ? double(dead_old) / double(inf_old) : 0.0;
  return s;
}

void check_output_files(const Model& m, const Histories& h, std::string_view name) {
  const fs::path out = m.output_dir;
  // totals CSV equals the total history vectors, column by column (single split per row)
  const fs::path tot = one_output(out, "_total_series_", ".csv");
  if (!tot.empty()) {
    const auto lines = test_support::split_trimmed_lines(test_support::read_file_text(tot));
    REQUIRE(lines.size() == size_t(m.ndays) + 1);
    const auto header = split(lines[0], ',');
    REQUIRE(header.size() == h.total_history_count);
    int bad = 0;
    for (size_t c = 0; c < header.size(); ++c)
      bad += header[c] != h.label(h.sim_history_count + c);
    for (size_t d = 1; d <= size_t(m.ndays); ++d) {
      const auto row = split(lines[d], ',');
      if (row.size() != header.size()) { ++bad; continue; }
      for (size_t c = 0; c < header.size(); ++c)
        bad += std::stol(row[c]) != h.data(h.sim_history_count + c)[d];
    }
    CHECK_MSG(bad == 0, "[{}] totals CSV: {} labels/values differ from histories", name, bad);
  }
  const fs::path all = one_output(out, "_all_series_", ".csv");
  if (!all.empty()) {
    const auto lines = test_support::split_trimmed_lines(test_support::read_file_text(all));
    CHECK_EQ(lines.size(), size_t(m.ndays) + 1);
    if (!lines.empty()) CHECK_EQ(split(lines[0], ',').size(), h.all_history_count);
  }
  // selected_series CSV: every header exists in Histories and values match day-by-day
  const fs::path sel = one_output(out, "_selected_series_", ".csv");
  if (!sel.empty()) {
    const auto lines = test_support::split_trimmed_lines(test_support::read_file_text(sel));
    REQUIRE(lines.size() == size_t(m.ndays) + 1);
    const auto header = split(lines[0], ',');
    REQUIRE(!header.empty());
    std::map<string, size_t> label_to_idx;
    for (size_t i = 0; i < h.all_history_count; ++i) label_to_idx[string{h.label(i)}] = i;
    vector<size_t> col_idx;
    col_idx.reserve(header.size());
    int bad = 0;
    for (const auto& lbl : header) {
      auto it = label_to_idx.find(lbl);
      if (it == label_to_idx.end()) { ++bad; col_idx.push_back(size_t(-1)); }
      else col_idx.push_back(it->second);
    }
    for (size_t d = 1; d <= size_t(m.ndays); ++d) {
      const auto row = split(lines[d], ',');
      if (row.size() != header.size()) { ++bad; continue; }
      for (size_t c = 0; c < header.size(); ++c)
        if (col_idx[c] != size_t(-1)) bad += std::stol(row[c]) != h.data(col_idx[c])[d];
    }
    CHECK_MSG(bad == 0, "[{}] selected_series CSV: {} labels/values differ from histories", name, bad);
  }
  // population CSV: one row per person; verify status, agegrp, ring, variant, vax, sdcase
  const fs::path popcsv = one_output(out, "_pop_", ".csv");
  if (!popcsv.empty()) {
    const auto lines = test_support::split_trimmed_lines(test_support::read_file_text(popcsv));
    REQUIRE(lines.size() == m.pop.popn + 1);
    const auto header = split(lines[0], ',');
    auto find_col = [&](std::string_view col_name) {
      const auto it = std::ranges::find(header, col_name);
      REQUIRE(it != header.end());
      return static_cast<size_t>(it - header.begin());
    };
    const size_t c_status = find_col("status"), c_age = find_col("agegrp"), c_ring = find_col("ring"),
                 c_variant = find_col("variant"), c_vax = find_col("vax"), c_sd = find_col("sdcase");
    int bad_rows = 0;
    for (size_t p = 1; p <= m.pop.popn; ++p) {
      const auto row = split(lines[p], ',');
      if (row.size() != header.size()) { ++bad_rows; continue; }
      const string want_vax = m.pop.vax[p].v == 0 ? "-" : string{m.pop.vax[p].show()};
      const string want_sd = m.pop.sdcase[p].v != 0 ? "true" : "false";
      const bool ok = row[c_status] == m.pop.status[p].show() && row[c_age] == m.pop.agegrp[p].show() &&
                      row[c_ring] == m.pop.ring[p].show() && row[c_variant] == m.pop.variant[p].show() &&
                      row[c_vax] == want_vax && row[c_sd] == want_sd;
      if (!ok) ++bad_rows;
    }
    CHECK_MSG(bad_rows == 0, "[{}] population CSV: {} row(s) differ from PopData", name, bad_rows);
  }
  // plots: 4 files; the cumulative outcome traces end at the final PopData counts
  int html = 0;
  for (const auto& e : fs::directory_iterator(out)) html += e.path().extension() == ".html";
  CHECK_MSG(html == 4, "[{}] {} plot files, want 4", name, html);
  const fs::path cum = one_output(out, "Cumulative_Covid_Outcome", ".html");
  if (!cum.empty()) {
    const string text = test_support::read_file_text(cum);
    const auto pos = text.find("const data = ");
    REQUIRE(pos != string::npos);
    const auto start = pos + 13;
    const auto end = text.find('\n', start);
    REQUIRE(end != string::npos);
    const json data = json::parse(text.substr(start, end - start));
    REQUIRE(data.size() == 4);
    for (const auto& tr : data) {
      const string label = tr["name"];
      const auto parts = split(label, '|');
      REQUIRE(parts.size() == 5);
      size_t want = 0;
      for (size_t p = 1; p <= m.pop.popn; ++p) want += m.pop.status[p].show() == parts[2];
      CHECK_MSG(tr["y"].size() == size_t(m.ndays) && tr["y"].back().get<size_t>() == want,
                "[{}] plot trace {} ends at {} (len {}), PopData has {}", name, label, tr["y"].back().dump(),
                tr["y"].size(), want);
    }
  }
}

ScenarioStats run_scenario(std::string_view name, const std::function<void(const fs::path&)>& edit,
                           const test_support::TestRunOptions& options) {
  RuntimeNamesGuard guard;
  const fs::path dir = test_support::project_dir() / "test_output" / "runsim" / name;
  fs::remove_all(dir);
  setup_dir(dir.string());
  if (edit) edit(dir);
  test_support::require_inputs_verify(dir);

  xo::seed(424242);  // setup shuffles ring membership; runsim reseeds itself
  test_support::StdoutCapture cap;
  Model m = use_dir(dir.string());
  Histories h = runsim(m);
  post_simulation(m, h);
  const string terminal = cap.finish();
  test_support::write_file_text(dir / "terminal.txt", terminal);

  CHECK_MSG(invariants::check_run(m.pop, h, name) == 0, "[{}] run invariants failed", name);
  int seed_lines = 0;
  for (const auto& l : test_support::split_trimmed_lines(terminal)) seed_lines += l.starts_with("Seed day");
  CHECK_EQ(seed_lines, int(m.seedcases.size()));
  check_output_files(m, h, name);

  const string summary = summary_block(terminal);
  CHECK_MSG(!summary.empty(), "[{}] no summary table in terminal output", name);
  test_support::compare_golden(summary, fmt::format("runsim/{}_summary.txt", name), options, GROUP);
  const fs::path tot = one_output(m.output_dir, "_total_series_", ".csv");
  if (!tot.empty())
    test_support::compare_golden(test_support::read_file_text(tot), fmt::format("runsim/{}_total_series.csv", name),
                                 options, GROUP);
  return compute_stats(m.pop);
}

void set_flags(const fs::path& d, bool vax, bool rings, bool sd) {
  test_support::edit_config(d, [&](json& c) {
    c["dovax"] = vax;
    c["do_rings"] = rings;
    c["do_social_distancing"] = sd;
  });
  if (vax) test_support::move_vax_schedules(d, 30, 150);
}

}  // namespace

void run_runsim_tests(const test_support::TestRunOptions& options) {
  using tst::run_test;
  ScenarioStats base, rings, vax, sd, all_on;
  run_test("scenario_base", [&] { base = run_scenario("base", {}, options); });
  run_test("scenario_rings", [&] { rings = run_scenario("rings", [](const fs::path& d) { set_flags(d, false, true, false); }, options); });
  run_test("scenario_vax", [&] { vax = run_scenario("vax", [](const fs::path& d) { set_flags(d, true, false, false); }, options); });
  run_test("scenario_sd", [&] { sd = run_scenario("sd", [](const fs::path& d) { set_flags(d, false, false, true); }, options); });
  run_test("scenario_all_on", [&] { all_on = run_scenario("all_on", [](const fs::path& d) { set_flags(d, true, true, true); }, options); });
  run_test("cross_scenario_behavioral_guardrails", [&] {
    CHECK_MSG(vax.infected < base.infected, "vax infected ({}) should be < base ({})", vax.infected, base.infected);
    CHECK_MSG(vax.dead < base.dead, "vax dead ({}) should be < base ({})", vax.dead, base.dead);
    CHECK_MSG(sd.infected < base.infected, "sd infected ({}) should be < base ({})", sd.infected, base.infected);
    CHECK_MSG(all_on.infected < base.infected, "all_on infected ({}) should be < base ({})", all_on.infected, base.infected);
    CHECK_MSG(base.cfr_80_up > base.cfr_0_19, "base age80_up CFR ({:.4f}) should exceed age0_19 ({:.4f})",
              base.cfr_80_up, base.cfr_0_19);
    (void)rings;
  });
  fmt::println("runsim case directories and terminal output: {}",
               (test_support::project_dir() / "test_output" / "runsim").string());
}
