#pragma once

#include "lib_includes.h"

nlohmann::json make_plot_payload(
    const std::string& title,
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::string& trace_name = "series"
);

std::string render_plot_html(
    std::string template_html,
    const std::string& title,
    const std::string& heading,
    const nlohmann::json& payload
);

bool open_with_default_app(const std::filesystem::path& path);

std::filesystem::path write_plot(
    const std::string& title,
    const std::string& heading,
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::string& trace_name = "series"
);

std::filesystem::path do_plot(
    const std::string& title,
    const std::string& heading,
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::string& trace_name = "series"
);

std::filesystem::path do_plot();
