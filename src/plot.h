#pragma once

#include "lib_includes.h"
#include "series.h"
#include "sim.h"

std::string render_plot_html(
    std::string template_html,
    const std::string& title,
    const std::string& endmessage,
    const json& data,
    const json& layout
);

bool open_plot_in_browser(const std::filesystem::path& path);

void seriesplot(SeriesColSpec spec, const AllSeries& series,
    const std::vector<absl::CivilDay>& caldays, SummaryData sumstruct,
    std::string plot_title, const bool dostack=false,
    std::filesystem::path output_path={});

void produce_plot(std::string base_fname, std::string end_message, json data, json layout);
void produce_plot(std::filesystem::path output_path, std::string end_message, json data, json layout);
