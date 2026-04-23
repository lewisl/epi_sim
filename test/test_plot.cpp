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

void test_render_plot_html_replaces_template_tokens() {
  const json data = sample_plot_data();
  const json layout = sample_plot_layout();

  const string html = render_plot_html(plot_html_template(), "Unit Test Plot 2026",
                                       "Simple plot render test", data, layout);

  assert(html.find("<title>Unit Test Plot 2026</title>") != string::npos);
  assert(html.find("<h2>Simple plot render test</h2>") != string::npos);
  assert(html.find("https://cdn.plot.ly/plotly-2.35.2.min.js") != string::npos);
  assert(html.find("<div id=\"plot\"") != string::npos);
  assert(html.find("\"name\":\"quadratic\"") != string::npos);
  assert(html.find("\"title\":\"Unit Test Plot 2026\"") != string::npos);
  assert(html.find("Plotly.newPlot(\"plot\", data, layout)") != string::npos);
  assert(html.find("{{TITLE}}") == string::npos);
  assert(html.find("{{ENDMESSAGE}}") == string::npos);
  assert(html.find("{{DATA}}") == string::npos);
  assert(html.find("{{LAYOUT}}") == string::npos);
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
  write_plot_artifacts(options);
  if (options.write_artifacts) {
    fmt::println("plot artifacts written under '{}'",
                 test_support::artifact_group_dir(options, GROUP).string());
    fmt::println("Open '{}' in your browser to visually inspect the Plotly preview.",
                 (test_support::artifact_group_dir(options, GROUP) / "rendered_plot_preview.html").string());
  }
  fmt::println("plot tests passed.");
}