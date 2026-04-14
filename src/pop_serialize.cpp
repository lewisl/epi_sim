#include "pop_serialize.h"

#include <algorithm>
#include <array>
#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <stdexcept>
#include <vector>


std::string bool_text(uint8_t value) {
  return value == 0 ? std::string{"false"} : std::string{"true"};
}

using RenderLines = std::vector<std::string>;
using RenderFn = RenderLines (*)(AgentView, bool);


std::string format_variant_name(Variant value) {
  const auto rendered = value.show();
  if (!rendered.empty()) return rendered;
  return idx(value) == 0 ? std::string{"none"} :
                           fmt::format("{}", static_cast<unsigned int>(static_cast<uint8_t>(value)));
}

std::string format_vax_name(Vax value) {
  const auto rendered = value.show();
  if (!rendered.empty()) return rendered;
  return idx(value) == 0 ? std::string{"none"} :
                           fmt::format("{}", static_cast<unsigned int>(static_cast<uint8_t>(value)));
}

RenderLines render_status(AgentView person, bool) { return {person.status().show()}; }

RenderLines render_agegrp(AgentView person, bool) { return {person.agegrp().show()}; }

RenderLines render_cond(AgentView person, bool) { return {person.cond().show()}; }

RenderLines render_duration(AgentView person, bool) { return {person.duration().show()}; }

RenderLines render_variant(AgentView person, bool) {
  return {format_variant_name(person.variant())};
}

RenderLines render_variant_hist(AgentView person, bool multi_values) {
  const auto& history = person.variant_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](Variant value) { return format_variant_name(value); },
      scalar_value);
}

RenderLines render_sickday(AgentView person, bool) {
  return {person.sickday() == 0 ? std::string{"-"} : person.sickday().show()};
}

RenderLines render_sickday_hist(AgentView person, bool multi_values) {
  const auto& history = person.sickday_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](int16_t value) { return fmt::format("{}", value); },
      scalar_value);
}

RenderLines render_recovday(AgentView person, bool multi_values) {
  (void)multi_values;
  return {person.recovday() == 0 ? std::string{"-"} : person.recovday().show()};
}

RenderLines render_recovday_hist(AgentView person, bool multi_values) {
  const auto& history = person.recovday_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](int16_t value) { return fmt::format("{}", value); },
      scalar_value);
}

RenderLines render_deadday(AgentView person, bool) { return {person.deadday().show()}; }

RenderLines render_ring(AgentView person, bool) {
  return {fmt::format("{}", static_cast<unsigned int>(person.ring()))};
}

RenderLines render_sdcase(AgentView person, bool) { return {bool_text(person.sdcase())}; }

RenderLines render_testday_hist(AgentView person, bool multi_values) {
  const auto& history = person.testday_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](int16_t value) { return fmt::format("{}", value); },
      scalar_value);
}

RenderLines render_testday(AgentView person, bool multi_values) {
  (void)multi_values;
  return {person.testday() == 0 ? std::string{"-"} : person.testday().show()};
}

RenderLines render_quar(AgentView person, bool) { return {bool_text(person.quar())}; }

RenderLines render_quarday(AgentView person, bool) { return {person.quarday().show()}; }

RenderLines render_vaxstatus(AgentView person, bool) { return {person.vaxstatus().show()}; }

RenderLines render_vax(AgentView person, bool) {
  return {idx(person.vax()) == 0 ? std::string{"-"} : format_vax_name(person.vax())};
}

RenderLines render_vax_hist(AgentView person, bool multi_values) {
  const auto& history = person.vax_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](Vax value) { return format_vax_name(value); },
      scalar_value);
}

RenderLines render_vaxday(AgentView person, bool) {
  return {person.vaxday() == 0 ? std::string{"-"} : person.vaxday().show()};
}

RenderLines render_vaxday_hist(AgentView person, bool multi_values) {
  const auto& history = person.vaxday_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](int16_t value) { return fmt::format("{}", value); },
      scalar_value);
}


/*
note:  bool multi_values arg provided by outer function print_pop_table,
       if not false (default value)
struct ColumnSpec Column enum,  key is string label, width, render as string function       
accessed with linear search on key:  maybe a little better than breakeven perf. with a flat_hash_map
*/
static inline constexpr std::array<ColumnSpec, 22> COLUMN_SPECS{{
    {ColumnName::status, "status", 10, render_status},
    {ColumnName::agegrp, "agegrp", 10, render_agegrp},
    {ColumnName::cond, "cond", 10, render_cond},
    {ColumnName::duration, "duration", 8, render_duration},
    {ColumnName::variant, "variant", 10, render_variant},
    {ColumnName::variant_hist, "variant_hist", 12, render_variant_hist},
    {ColumnName::sickday, "sickday", 7, render_sickday},
    {ColumnName::sickday_hist, "sickday_hist", 11, render_sickday_hist},
    {ColumnName::recovday, "recovday", 8, render_recovday},
    {ColumnName::recovday_hist, "recovday_hist", 13, render_recovday_hist},
    {ColumnName::deadday, "deadday", 7, render_deadday},
    {ColumnName::ring, "ring", 4, render_ring},
    {ColumnName::sdcase, "sdcase", 6, render_sdcase},
    {ColumnName::testday_hist, "testday_hist", 12, render_testday_hist},
    {ColumnName::testday, "testday", 7, render_testday},
    {ColumnName::quar, "quar", 5, render_quar},
    {ColumnName::quarday, "quarday", 7, render_quarday},
    {ColumnName::vaxstatus, "vaxstatus", 10, render_vaxstatus},
    {ColumnName::vax, "vax", 10, render_vax},
    {ColumnName::vax_hist, "vax_hist", 10, render_vax_hist},
    {ColumnName::vaxday, "vaxday", 6, render_vaxday},
    {ColumnName::vaxday_hist, "vaxday_hist", 11, render_vaxday_hist},
}};


constexpr size_t ROW_LABEL_WIDTH = 6;

void validate_rows(const PopData &pop, std::span<const size_t> rows) {
  for (const auto row : rows) {
    if (row == 0 || row > pop.popn) {
      throw std::invalid_argument(
          fmt::format("Row {} is out of range; valid rows are 1..{}", row, pop.popn));
    }
  }
}

const ColumnSpec *find_column(std::string_view name) {
  const auto it = std::find_if(COLUMN_SPECS.begin(), COLUMN_SPECS.end(),
                               [&](const ColumnSpec &spec) { return spec.key == name; });
  if (it == COLUMN_SPECS.end()) return nullptr;
  return &*it;
}

std::string csv_escape(std::string_view cell) {
  if (cell.find_first_of(",\"\n\r") == std::string_view::npos) {
    return std::string{cell};
  }

  std::string escaped;
  escaped.reserve(cell.size() + 2);
  escaped.push_back('"');
  for (const char ch : cell) {
    if (ch == '"') escaped.push_back('"');
    escaped.push_back(ch);
  }
  escaped.push_back('"');

  return escaped;
}


// method called with vector of column name strings
std::vector<ColumnSpec> resolve_columns(std::vector<std::string> col_names) {
  std::vector<ColumnSpec> columns;
  columns.reserve(col_names.size());
  for (const auto name : col_names) {
    const ColumnSpec *spec = find_column(name);
    if (spec == nullptr) {
      throw std::invalid_argument(fmt::format("Unknown PopData column '{}'", name));
    }
    columns.push_back(*spec);
  }
  return columns;
}

// method called with vector literal of column names
std::vector<ColumnSpec> resolve_columns(std::initializer_list<std::string> col_names) {
  return resolve_columns(col_names);
}

size_t separator_width(std::span<const ColumnSpec> columns) {
  size_t separator_width = ROW_LABEL_WIDTH;
  for (const auto &column : columns) separator_width += 2 + column.width;
  return separator_width;
}

std::string build_line_format(std::span<const ColumnSpec> columns, bool right_align_row) {
  std::string format = right_align_row ? fmt::format("{{:>{}}}", ROW_LABEL_WIDTH)
                                       : fmt::format("{{:<{}}}", ROW_LABEL_WIDTH);
  for (const auto &column : columns) {
    format += "  ";
    format += fmt::format("{{:<{}.{}}}", column.width, column.width);
  }
  format += "\n";
  return format;
}

void print_header_line(std::ostream &out, std::string_view format,
                       std::span<const ColumnSpec> columns) {
  fmt::dynamic_format_arg_store<fmt::format_context> args;
  args.push_back(std::string_view{"row"});
  for (const auto &column : columns) args.push_back(column.key);
  fmt::vprint(out, format, args);
}

void print_row_line(std::ostream &out, std::string_view format, std::string_view row_label,
                    std::span<const ColumnSpec> columns,
                    std::span<const RenderLines> rendered_columns, size_t line_idx) {
  fmt::dynamic_format_arg_store<fmt::format_context> args;
  args.push_back(row_label);
  for (size_t i = 0; i < columns.size(); ++i) {
    const std::string_view cell =
        line_idx < rendered_columns[i].size() ? std::string_view{rendered_columns[i][line_idx]}
                                              : std::string_view{};
    args.push_back(cell);
  }
  fmt::vprint(out, format, args);
}

void ensure_parent_dir(const std::filesystem::path& output_path) {
  const auto parent = output_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }
}

std::filesystem::path set_output_file(string base_fname, vector<string> path_steps) {
  std::filesystem::path fpath {};

  if (path_steps.empty()) {
    fpath = std::filesystem::current_path() / "population_output";
  } else {
    const char* home = std::getenv("HOME");
    if (!home) throw std::runtime_error("HOME not set");
    fpath = home;
    for (const auto& step : path_steps) fpath /= step;
  }
  fpath /= make_timestamped_filename(base_fname) + ".csv";
  ensure_parent_dir(fpath);

  std::ofstream out(fpath);  // move this only create the file handle after verifying we have things to write
  if (!out) {
    throw std::runtime_error(fmt::format("Could not write population CSV to '{}'",
                                         fpath.string()));
    }                                  
  return fpath;
}


// writes single line
void write_serialized_line(std::ostream &out, std::string_view row_label,
                           std::span<const ColumnSpec> columns,
                           std::span<const RenderLines> rendered_columns, size_t line_idx,
                           std::string_view separator, bool escape_cells) {
  std::vector<std::string> parts;
  parts.reserve(columns.size() + 1);

  auto serialize_cell = [&](std::string_view cell) {
    return escape_cells ? csv_escape(cell) : std::string{cell};
  };

  parts.push_back(serialize_cell(row_label));
  for (size_t i = 0; i < columns.size(); ++i) {
    const std::string_view cell =
        line_idx < rendered_columns[i].size() ? std::string_view{rendered_columns[i][line_idx]}
                                              : std::string_view{};
    parts.push_back(serialize_cell(cell));
  }
  fmt::println(out, "{}", fmt::join(parts, separator));
}

void write_serialized_header(std::ostream &out, std::span<const ColumnSpec> columns,
                             std::string_view separator, bool escape_cells) {
  std::vector<std::string> parts;
  parts.reserve(columns.size() + 1);

  auto serialize_cell = [&](std::string_view cell) {
    return escape_cells ? csv_escape(cell) : std::string{cell};
  };

  parts.push_back(serialize_cell("row"));
  for (const auto &column : columns) {
    parts.push_back(serialize_cell(column.key));
  }
  fmt::println(out, "{}", fmt::join(parts, separator));
}


std::string render_pop_cell(std::string_view col_name, AgentView person) {
  const ColumnSpec *spec = find_column(col_name);
  if (spec == nullptr) {
    throw std::invalid_argument(fmt::format("Unknown PopData column '{}'", col_name));
  }

  const RenderLines lines = spec->render(person, false);
  if (lines.empty()) return {};
  return lines.front();
}


vector<std::string> get_all_column_names() {
  vector<std::string> names;
  for (auto spec : COLUMN_SPECS) {
    names.push_back(spec.key);
  }
  return names;
}


void write_pop_data(
        PopData &pop,                  // instance of trait vectors for the population
        std::span<const size_t> rows,  // row indices
        ColSpec col_names,             // vector of string column labels, literal vector of labels, or "all" 
        OutSpec target,                // file path or stdout
        Style layout,                  // serialize or pretty
        bool multi_values,             // true nicer for pretty; false more compact for serialize
        std::string_view serialized_separator,   // typically ","
        bool escape_serialized_cells )           // whatever
{
  validate_rows(pop, rows);
  const auto columns = resolve_columns(col_names.names);

  std::ofstream fpath;
  std::ostream *out; 
  if (target.kind == OutSpec::Kind::Path) {
    fpath.open(target.path);
    out = &fpath;
} else {
    out = target.out;
}

  const bool pretty_layout = layout == Style::pretty;
  const auto header_format = pretty_layout ? build_line_format(columns, false) : std::string{};
  const auto row_format = pretty_layout ? build_line_format(columns, true) : std::string{};

  if (pretty_layout) {
    print_header_line(*out, header_format, columns);
    fmt::println(*out, "{:-<{}}", "", separator_width(columns));
  } else {
    write_serialized_header(*out, columns, serialized_separator, escape_serialized_cells);
  }

  for (const auto row : rows) {
    auto person = pop.agent(row);
    std::vector<RenderLines> rendered_columns;
    rendered_columns.reserve(columns.size());

    size_t line_count = 1;
    for (const auto &column : columns) {
      rendered_columns.push_back(column.render(person, multi_values));
      line_count = std::max(line_count, rendered_columns.back().size());
    }

    for (size_t line_idx = 0; line_idx < line_count; ++line_idx) {
      const std::string row_label = line_idx == 0 ? fmt::format("{}", row) : std::string{"*"};
      if (pretty_layout) {
        print_row_line(*out, row_format, row_label, columns, rendered_columns, line_idx);
      } else {
        write_serialized_line(*out, row_label, columns, rendered_columns, line_idx,
                              serialized_separator, escape_serialized_cells);
      }
    }
  }
}
