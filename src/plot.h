#pragma once

#include "lib_includes.h"
#include "series.h"

std::string render_plot_html(
    std::string template_html,
    const std::string& title,
    const std::string& endmessage,
    const nlohmann::json& data,
    const nlohmann::json& layout
);

bool open_plot_in_browser(const std::filesystem::path& path);

void cumplot(std::vector<SeriesSelection> selections, const DayData& series,
    const std::vector<absl::CivilDay>& caldays);
