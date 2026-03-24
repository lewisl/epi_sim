#include "plot.h"
#include "helpers.h"
#include "series.h"
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <ctime>

namespace {

std::string html_template = R"TAG(<!doctype html>
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

} // namespace

// data has to be a JSON array of series (what Plotly stupidly calls traces)
// layout supplies title and more...
/* example:
    const data = [{"mode":"lines","name":"quadratic","type":"scatter","x":[0.0,1.0,2.0,3.0,4.0],"y":[0.0,1.0,4.0,9.0,16.0]}]
    const layout = {"title":"Plotly Browser Test"}
*/



std::string make_timestamped_filename(std::string plotname = "") {
    if (plotname.empty()) {
        plotname = "plot";
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    const std::tm local_tm = *std::localtime(&now_time);

    return fmt::format("{}_{:02}_{:02}_{:04}_{:02}_{:02}_{:02}",
                       plotname,
                       local_tm.tm_mon + 1,
                       local_tm.tm_mday,
                       local_tm.tm_year + 1900,
                       local_tm.tm_hour,
                       local_tm.tm_min,
                       local_tm.tm_sec);
}


std::filesystem::path write_plot_html_file(const std::string& html, std::string filename,
  const std::filesystem::path& output_path = 
    std::filesystem::path(std::getenv("HOME")) / "code" / "epi_sim" / "plot_output" ) {

    filename.append(".html");
    std::ofstream out(output_path / filename);
    if (!out) {
        throw std::runtime_error(
            fmt::format("Could not write rendered plot to '{}'", output_path.string()));
    }

    out << html;

    return output_path / filename;
}



std::string render_plot_html(
    std::string template_html,
    const std::string& title,
    const std::string& endmessage,
    const json& data,
    const json& layout
) {
    replace_all(template_html, "{{TITLE}}", title);
    replace_all(template_html, "{{ENDMESSAGE}}", endmessage);
    replace_all(template_html, "{{DATA}}", data.dump());
    replace_all(template_html, "{{LAYOUT}}", layout.dump());
    return template_html;
}

bool open_plot_in_browser(const std::filesystem::path& path) {
    std::string cmd = "open \"" + path.string() + "\"";
    return std::system(cmd.c_str()) == 0;
}

//
// standard plot types for simulation history output
//
void cumplot(std::vector<SeriesSelection> selections, const DayData& series,
    const std::vector<absl::CivilDay>& caldays) {

  // step 1: assemble data from simulation run
  vector<std::pair<SeriesName, AgeBucket>> cols;
  vector<std::string_view> labels;
  cols.reserve(selections.size());
  labels.reserve(selections.size());
  vector<string> invalid_selections;
  // x axis values    
  std::vector<std::string> daystrs;
  daystrs.reserve(caldays.size());
  for (const auto& day :  caldays)
      daystrs.push_back(absl::FormatCivilTime(day));
  // y axis values
  for (const auto& [name_text, bucket_text] : selections) {
    auto name = series_name_from_string(name_text);
    auto bucket = age_bucket_from_string(bucket_text);
    if (!name || !bucket) {
      invalid_selections.push_back(fmt::format("{}:{}", name_text, bucket_text));
      continue;
    }
    cols.emplace_back(*name, *bucket);
    labels.push_back(to_string(*name));
  }

  // step 2: create the json objects
  json data = json::array();
  for (auto [label, col] : std::views::zip(labels, cols) ) {
    const auto& [name, bucket] = col;
    const auto& y_series = series.at(name, bucket);
    std::vector<size_t> y_values(y_series.begin() + 1, y_series.end());

    data.push_back({
      {"x", daystrs},
      {"y", y_values},
      {"type", "scatter"},
      {"mode", "lines"},
      {"name", label}
    });
  }

  json layout = {{"title", "Cumulative Covid Outcome"}};

  // step 3: render the complete html
  std::string cum_plot_html = render_plot_html(html_template, "CumPlot","hey, did it work", data, layout);

  // step 4: generate path/filename
  string fname = make_timestamped_filename("cumplot");

  // step 5: output the finished html file
  std::filesystem::path fpath = write_plot_html_file(cum_plot_html, fname);

  //step 6: open in browser
  open_plot_in_browser(fpath);
}
