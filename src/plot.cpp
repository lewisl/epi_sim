#include "plot.h"
#include "helpers.h"
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <ctime>
using json = nlohmann::json;

namespace {

std::filesystem::path find_plot_template_path() {
    const std::array relative_candidates{
        std::filesystem::path{"src/plot_template.html"},
        std::filesystem::path{"plot_template.html"}};

    std::error_code ec;
    auto search_dir = std::filesystem::current_path(ec);
    if (ec) {
        throw std::runtime_error("Could not determine current working directory");
    }

    while (true) {
        for (const auto& candidate : relative_candidates) {
            const auto full_path = search_dir / candidate;
            if (std::filesystem::exists(full_path)) {
                return full_path;
            }
        }

        const auto parent_dir = search_dir.parent_path();
        if (parent_dir == search_dir) {
            break;
        }
        search_dir = parent_dir;
    }

    throw std::runtime_error(
        fmt::format("Could not find plot template file starting from '{}'",
                    std::filesystem::current_path().string()));
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error(
            fmt::format("Could not open plot template '{}'", path.string()));
    }

    return std::string(
        std::istreambuf_iterator<char>(in),
        std::istreambuf_iterator<char>());
}

std::string slugify_filename_component(std::string_view text) {
    std::string slug;
    slug.reserve(text.size());

    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            slug.push_back(static_cast<char>(std::tolower(ch)));
        } else if ((ch == ' ' || ch == '-' || ch == '_') &&
                   (slug.empty() || slug.back() != '_')) {
            slug.push_back('_');
        }
    }

    while (!slug.empty() && slug.back() == '_') {
        slug.pop_back();
    }

    if (slug.empty()) {
        slug = "plot";
    }

    return slug;
}

std::string make_timestamp_string() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    const std::tm local_tm = *std::localtime(&now_time);

    return fmt::format("{:02}_{:02}_{:04}_{:02}_{:02}_{:02}",
                       local_tm.tm_mon + 1,
                       local_tm.tm_mday,
                       local_tm.tm_year + 1900,
                       local_tm.tm_hour,
                       local_tm.tm_min,
                       local_tm.tm_sec);
}

std::filesystem::path make_plot_output_dir(const std::filesystem::path& template_path) {
    return template_path.parent_path() / "plot_output";
}

std::filesystem::path make_plot_output_path(const std::filesystem::path& template_path,
                                            std::string_view title) {
    const auto output_dir = make_plot_output_dir(template_path);
    const std::string stem = fmt::format("{}_{}",
                                         slugify_filename_component(title),
                                         make_timestamp_string());

    std::filesystem::path output_path = output_dir / fmt::format("{}.html", stem);
    int duplicate_counter = 2;

    while (std::filesystem::exists(output_path)) {
        output_path = output_dir /
                      fmt::format("{}_{}.html", stem, duplicate_counter);
        ++duplicate_counter;
    }

    return output_path;
}

std::filesystem::path write_plot_file(const std::filesystem::path& output_path,
                                      const std::string& html) {
    std::filesystem::create_directories(output_path.parent_path());

    std::ofstream out(output_path);
    if (!out) {
        throw std::runtime_error(
            fmt::format("Could not write rendered plot to '{}'", output_path.string()));
    }

    out << html;
    return output_path;
}

} // namespace

json make_plot_payload(const std::string& title, const std::vector<double>& x, const std::vector<double>& y,  const std::string& trace_name) {
    return {
        {"data", json::array({
            {
                {"x", x},
                {"y", y},
                {"type", "scatter"},
                {"mode", "lines"},
                {"name", trace_name}
            }
        })},
        {"layout", {
            {"title", title}
        }}
    };
}

std::string render_plot_html(
    std::string template_html,
    const std::string& title,
    const std::string& heading,
    const json& payload
) {
    replace_all(template_html, "{{TITLE}}", title);
    replace_all(template_html, "{{HEADING}}", heading);
    replace_all(template_html, "{{PAYLOAD_JSON}}", payload.dump());
    return template_html;
}

bool open_with_default_app(const std::filesystem::path& path) {
    std::string cmd = "open \"" + path.string() + "\"";
    return std::system(cmd.c_str()) == 0;
}

std::filesystem::path write_plot(
    const std::string& title,
    const std::string& heading,
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::string& trace_name
) {
    const json payload = make_plot_payload(title, x, y, trace_name);
    const auto template_path = find_plot_template_path();
    const std::string template_html = read_text_file(template_path);
    const std::string rendered_html =
        render_plot_html(template_html, title, heading, payload);
    return write_plot_file(make_plot_output_path(template_path, title), rendered_html);
}

std::filesystem::path do_plot(
    const std::string& title,
    const std::string& heading,
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::string& trace_name
) {
    const auto output_path = write_plot(title, heading, x, y, trace_name);

    if (!open_with_default_app(output_path)) {
        throw std::runtime_error(
            fmt::format("Failed to open plot file '{}'", output_path.string()));
    }

    return output_path;
}

std::filesystem::path do_plot() {
    return do_plot(
        "Quadratic",
        "Quadratic demo plot",
        {0.0, 1.0, 2.0, 3.0},
        {0.0, 1.0, 4.0, 9.0},
        "series");
}
