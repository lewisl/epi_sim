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

// same steps for every plot
void produce_plot(std::string base_fname, std::string end_message, json data, json layout) {
  // generate path/filename
  string fname = make_timestamped_filename(base_fname);

  // render the complete html
  std::string cum_plot_html = render_plot_html(html_template, fname, end_message, data, layout);

  // output the finished html file
  std::filesystem::path fpath = write_plot_html_file(cum_plot_html, fname);

  // open in browser
  open_plot_in_browser(fpath);
}

//
// standard plot types for simulation history output
//
void seriesplot(std::vector<SeriesSelection> selections, const HistorySeries& series,
    const std::vector<absl::CivilDay>& caldays, SummaryData sumstruct,
    const std::string plot_title, const bool dostack) {

  // step 1: assemble data from simulation run
  vector<std::pair<SeriesName, AgeBucket>> cols;
  vector<std::string> labels;
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
    labels.emplace_back(fmt::format("{}:{}", name_text, bucket_text));
  }
  // summary totals for plot inset text box
  int died = sumstruct.dead[6];
  int recovered = sumstruct.recovered[6];
  int unexposed = sumstruct.unexposed[6];
  int infected = sumstruct.infected[6];

  // step 2: create the json objects
  json data = json::array();
  for (auto [label, col] : std::views::zip(labels, cols) ) {
    const auto& [name, bucket] = col;
    const auto& y_series = series.at(name, bucket);
    std::vector<int> y_values(y_series.begin() + 1, y_series.end());

    json trace = {
      {"x", daystrs},
      {"y", y_values},
      {"type", "scatter"},
      {"mode", "lines"},
      {"name", label}
    };

    if (dostack) {
      trace["stackgroup"] = "stk";
    }

    data.push_back(std::move(trace));
  }

  // this is the ideal and most reliable way to do this
  json layout;  // add keys just like a map
  layout["title"] = plot_title;
  layout["xaxis"]["title"]["text"] = "Simulation Days";
  layout["yaxis"]["title"]["text"] = "Count of People";
  layout["plot_bgcolor"] = "#f3f2f2";
  layout["annotations"] = json::array({
      {
        {"text", fmt::format(
            "Died: {}<br>Infected: {}<br>Recovered: {}<br>Unexposed: {}",
            died, infected, recovered, unexposed)},
        {"xref", "paper"},
        {"yref", "paper"},
        {"x", 0.02},
        {"y", 0.55},
        {"showarrow", false},
        {"font", {{"size", 13}}},
        {"align", "left"},
        {"bgcolor", "rgba(255,255,255,0.8)"},
        {"bordercolor", "#888"},
        {"borderwidth", 1},
        {"borderpad", 6}
      }
  });

  produce_plot(plot_title, "Close the tab and return to terminal.", data, layout);
}


  
