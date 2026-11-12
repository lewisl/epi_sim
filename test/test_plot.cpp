// plot: HTML rendering and the Plotly data contract -- each trace is one selected
// history vector, x has one calendar date per simulated day, y has one number per day.
// (The browser launch is suppressed by test_main via a no-op `open` on PATH.)
#include "test_support.h"

#include "../src/plot.h"

namespace {

constexpr std::string_view GROUP = "plot";
using test_support::RuntimeNamesGuard;

json extract_json_assignment(const string& html, string_view variable_name) {
  const string prefix = fmt::format("const {} = ", variable_name);
  const size_t start = html.find(prefix);
  REQUIRE(start != string::npos);
  const size_t json_start = start + prefix.size();
  const size_t json_end = html.find('\n', json_start);
  REQUIRE(json_end != string::npos);
  return json::parse(html.substr(json_start, json_end - json_start));
}

json sample_data() {
  return json::array({{{"x", vector<double>{0, 1, 2, 3}}, {"y", vector<double>{0, 1, 4, 9}},
                       {"type", "scatter"}, {"mode", "lines"}, {"name", "quadratic"}}});
}

void test_render_plot_html_replaces_every_token() {
  const json layout = {{"title", "Unit Test Plot"}};
  const string html = render_plot_html("<t>{{TITLE}}</t><h2>{{ENDMESSAGE}}</h2>\nconst data = {{DATA}}\n"
                                       "const layout = {{LAYOUT}}\n",
                                       "T1", "end", sample_data(), layout);
  CHECK(html.contains("<t>T1</t><h2>end</h2>"));
  CHECK(!html.contains("{{"));
  CHECK_EQ(extract_json_assignment(html, "data"), sample_data());
  CHECK_EQ(extract_json_assignment(html, "layout"), layout);
}

void test_produce_plot_writes_loadable_html() {
  const auto dir = test_support::fs::temp_directory_path() / test_support::unique_name("epi_plot_");
  const auto out = dir / "plots" / "unit_plot.html";
  const json layout = {{"title", "Unit Test Plot"}};
  produce_plot(out, "done", sample_data(), layout);
  REQUIRE(test_support::fs::is_regular_file(out));
  const string html = test_support::read_file_text(out);
  CHECK(html.contains("<title>unit_plot</title>"));
  CHECK(html.contains("https://cdn.plot.ly/plotly-"));
  CHECK(html.contains("Plotly.newPlot(\"plot\", data, layout)"));
  CHECK(!html.contains("{{"));
  CHECK_EQ(extract_json_assignment(html, "data"), sample_data());
  test_support::fs::remove_all(dir);
}

// historyplot: traces are exactly the selected vectors (5-field selectors), in order
void test_historyplot_traces_match_selected_histories(const test_support::TestRunOptions& options) {
  RuntimeNamesGuard guard;
  test_support::install_names({"base"}, {}, {"ring_1", "ring_2"});
  auto pop = test_support::make_pop(20, 2);
  const size_t days = 6;
  Histories h(days, pop);
  for (size_t d = 1; d <= days; ++d) {
    h.at(TraitID::status, Phase::now, INFECTIOUS.v, AGE0_19, 1)[d] = static_cast<HistoryValue>(d);
    h.at(TraitID::status, Phase::now, INFECTIOUS.v, AGE20_39, 2)[d] = static_cast<HistoryValue>(10 * d);
    h.at(TraitID::status, Phase::now, DEAD.v, AGE80_UP, 2)[d] = static_cast<HistoryValue>(d / 2);
  }
  h.sum_total_histories();
  const HistorySelectorSet sel{{"status", "now", "infectious", "total", "total"},
                               {"status", "now", "infectious", "age0_19", "ring_1"},
                               {"status", "now", "dead", "age80_up", "total"}};
  const auto idxs = h.select_indices(sel);
  REQUIRE(idxs.size() == 3);
  vector<absl::CivilDay> caldays;
  for (size_t d = 0; d < days; ++d) caldays.push_back(absl::CivilDay(2020, 3, 1) + d);
  SummaryData sum{};
  sum.dead[6] = 7;
  const auto dir = test_support::fs::temp_directory_path() / test_support::unique_name("epi_hplot_");
  const auto out = dir / "h.html";
  historyplot(sel, h, caldays, sum, "History Plot Test", true, out);
  const string html = test_support::read_file_text(out);
  const json data = extract_json_assignment(html, "data");
  const json layout = extract_json_assignment(html, "layout");
  REQUIRE(data.is_array() && data.size() == 3);
  for (size_t t = 0; t < 3; ++t) {
    const auto& tr = data[t];
    REQUIRE(tr.contains("x") && tr.contains("y"));
    CHECK_EQ(tr["name"].get<string>(), string(h.label(idxs[t])));
    CHECK_EQ(tr["x"].size(), days);
    CHECK_EQ(tr["y"].size(), days);
    CHECK_EQ(tr["x"][0].get<string>(), string("2020-03-01"));
    CHECK_EQ(tr["stackgroup"].get<string>(), string("stk"));
    vector<HistoryValue> want(h.data(idxs[t]).begin() + 1, h.data(idxs[t]).end());
    CHECK_MSG(tr["y"].get<vector<HistoryValue>>() == want, "trace {} y {} != history {}", t, tr["y"].dump(),
              want);
  }
  CHECK_EQ(data[0]["y"][5].get<int>(), 66);  // 6 + 60
  CHECK_EQ(layout["title"].get<string>(), string("History Plot Test"));
  CHECK(layout["annotations"][0]["text"].get<string>().contains("Died: 7"));
  test_support::write_artifact_text(options, GROUP, "history_plot_reference.html", html);
  test_support::fs::remove_all(dir);
}

void test_historyplot_with_no_valid_selection_writes_nothing() {
  RuntimeNamesGuard guard;
  test_support::install_names({"base"});
  auto pop = test_support::make_pop(5);
  Histories h(2, pop);
  const auto dir = test_support::fs::temp_directory_path() / test_support::unique_name("epi_hplot_");
  test_support::StdoutCapture quiet;
  historyplot(HistorySelectorSet{{"status", "now", "none", "total", "total"}}, h, {}, SummaryData{}, "empty",
              false, dir / "none.html");
  (void)quiet.finish();
  CHECK(!test_support::fs::exists(dir / "none.html"));
}

}  // namespace

void run_plot_tests(const test_support::TestRunOptions& options) {
  using tst::run_test;
  run_test("render_plot_html_replaces_every_token", test_render_plot_html_replaces_every_token);
  run_test("produce_plot_writes_loadable_html", test_produce_plot_writes_loadable_html);
  run_test("historyplot_traces_match_histories", [&] { test_historyplot_traces_match_selected_histories(options); });
  run_test("historyplot_no_valid_selection", test_historyplot_with_no_valid_selection_writes_nothing);
  if (options.write_artifacts)
    fmt::println("Open '{}' to check the reference plot visually.",
                 (test_support::artifact_group_dir(options, GROUP) / "history_plot_reference.html").string());
}
