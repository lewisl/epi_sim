#include "test_support.h"

#include "../src/plot.h"

namespace {

constexpr std::string_view GROUP = "plot";

string plot_html_template() {
  return R"TAG(<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>{{TITLE}}</title>
  <script src="https://cdn.plot.ly/plotly-2.35.2.min.js"></script>
</head>
<body>
  <div id="plot" style="width:900px;height:600px;"></div>
  <h2>{{ENDMESSAGE}}</h2>
  <script>
    const data = {{DATA}}
    const layout = {{LAYOUT}}

    Plotly.newPlot("plot", data, layout)
  </script>
</body>
</html>
)TAG";
}

json sample_plot_data() {
  return json::array({{{"x", std::vector<double>{0.0, 1.0, 2.0, 3.0}},
                       {"y", std::vector<double>{0.0, 1.0, 4.0, 9.0}},
                       {"type", "scatter"},
                       {"mode", "lines"},
                       {"name", "quadratic"}}});
}

json sample_plot_layout() {
  return {{"title", "Unit Test Plot 2026"},
          {"xaxis", {{"title", {{"text", "Simulation Days"}}}}},
          {"yaxis", {{"title", {{"text", "Count of People"}}}}}};
}

string extract_json_assignment(const string& html, string_view variable_name) {
  const string prefix = fmt::format("const {} = ", variable_name);
  const size_t start = html.find(prefix);
  REQUIRE(start != string::npos);
  const size_t json_start = start + prefix.size();
  const size_t json_end = html.find('\n', json_start);
  REQUIRE(json_end != string::npos);
  return html.substr(json_start, json_end - json_start);
}

void test_render_plot_html_replaces_template_tokens() {
  const json data = sample_plot_data();
  const json layout = sample_plot_layout();

  const string html = render_plot_html(plot_html_template(), "Unit Test Plot 2026",
                                       "Simple plot render test", data, layout);

  CHECK(html.find("<title>Unit Test Plot 2026</title>") != string::npos);
  CHECK(html.find("<h2>Simple plot render test</h2>") != string::npos);
  CHECK(html.find("https://cdn.plot.ly/plotly-2.35.2.min.js") != string::npos);
  CHECK(html.find("<div id=\"plot\"") != string::npos);
  CHECK(html.find("\"name\":\"quadratic\"") != string::npos);
  CHECK(html.find("\"title\":\"Unit Test Plot 2026\"") != string::npos);
  CHECK(html.find("Plotly.newPlot(\"plot\", data, layout)") != string::npos);
  CHECK(html.find("{{TITLE}}") == string::npos);
  CHECK(html.find("{{ENDMESSAGE}}") == string::npos);
  CHECK(html.find("{{DATA}}") == string::npos);
  CHECK(html.find("{{LAYOUT}}") == string::npos);
}

void test_produce_plot_writes_valid_html_file() {
  const auto temp_dir = test_support::fs::temp_directory_path() /
                        fmt::format("epi_sim_plot_test_{}", std::random_device{}());
  const auto output_path = temp_dir / "plots" / "unit_plot.html";
  const json data = sample_plot_data();
  const json layout = sample_plot_layout();

  produce_plot(output_path, "Simple plot render test", data, layout);

  CHECK(test_support::fs::exists(output_path));
  CHECK(test_support::fs::is_regular_file(output_path));
  CHECK(output_path.parent_path() == temp_dir / "plots");

  const string html = test_support::read_file_text(output_path);
  CHECK(html.find("<title>unit_plot</title>") != string::npos);
  CHECK(html.find("<h2>Simple plot render test</h2>") != string::npos);
  CHECK(html.find("https://cdn.plot.ly/plotly-2.35.2.min.js") != string::npos);
  CHECK(html.find("<div id=\"plot\"") != string::npos);
  CHECK(html.find("Plotly.newPlot(\"plot\", data, layout)") != string::npos);
  CHECK(html.find("{{TITLE}}") == string::npos);
  CHECK(html.find("{{ENDMESSAGE}}") == string::npos);
  CHECK(html.find("{{DATA}}") == string::npos);
  CHECK(html.find("{{LAYOUT}}") == string::npos);

  CHECK(json::parse(extract_json_assignment(html, "data")) == data);
  CHECK(json::parse(extract_json_assignment(html, "layout")) == layout);

  test_support::fs::remove_all(temp_dir);
}

void write_plot_artifacts(const test_support::TestRunOptions& options) {
  if (!options.write_artifacts) return;

  const json data = sample_plot_data();
  const json layout = sample_plot_layout();
  const string html = render_plot_html(plot_html_template(), "Unit Test Plot 2026",
                                       "Simple plot render test", data, layout);
  const auto preview_path = test_support::artifact_group_dir(options, GROUP) / "rendered_plot_preview.html";

  std::ostringstream artifact;
  artifact << "Plot summary\n";
  artifact << "============\n\n";
  artifact << "render_plot_html:\n";
  artifact << "  title token replaced: " << (html.find("{{TITLE}}") == string::npos) << "\n";
  artifact << "  contains trace name: " << (html.find("\"name\":\"quadratic\"") != string::npos) << "\n";
  artifact << "  preview file: " << preview_path.string() << "\n";
  artifact << "  open that file in your browser for visual inspection\n";

  test_support::write_artifact_text(options, GROUP, "plot_summary.txt", artifact.str());
  test_support::write_artifact_text(options, GROUP, "rendered_plot_preview.html", html);
}

}  // namespace

void run_plot_tests(const test_support::TestRunOptions& options) {
  fmt::println("Running plot tests...");
  test_render_plot_html_replaces_template_tokens();
  test_produce_plot_writes_valid_html_file();
  write_plot_artifacts(options);
  if (options.write_artifacts) {
    fmt::println("plot artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
    fmt::println("Open '{}' in your browser to visually inspect the Plotly preview.",
                 (test_support::artifact_group_dir(options, GROUP) / "rendered_plot_preview.html").string());
  }
  fmt::println("plot tests passed.");
}
