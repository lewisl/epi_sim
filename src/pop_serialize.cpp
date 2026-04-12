#include "pop_serialize.h"

#include <algorithm>
#include <array>
#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <stdexcept>
#include <vector>

namespace {

std::string bool_text(uint8_t value) {
  return value == 0 ? std::string{"false"} : std::string{"true"};
}

using RenderLines = std::vector<std::string>;
using RenderFn = RenderLines (*)(AgentView, bool);


struct ColumnSpec {
  ColumnName name;
  std::string_view key;
  size_t width;
  RenderFn render;
};

template <typename T, size_t N>
const T *stored_value(const std::array<T, N> &values, uint8_t count, size_t entry_idx) {
  const size_t stored_entries = std::min<size_t>(count, N);
  if (entry_idx >= stored_entries) return nullptr;
  return &values[entry_idx];
}

template <typename T, size_t N, typename Formatter>
RenderLines render_history(const std::array<T, N> &values, uint8_t count,
                           bool multi_values, Formatter formatter,
                           std::string scalar_value) {
  const size_t entry_count = std::min<size_t>(count, N);
  if (!multi_values || entry_count == 0) return {std::move(scalar_value)};

  RenderLines lines;
  lines.reserve(entry_count);
  for (size_t entry_idx = 0; entry_idx < entry_count; ++entry_idx) {
    const T *value = stored_value(values, count, entry_idx);
    lines.push_back(value == nullptr ? "" : formatter(*value));
  }
  return lines;
}

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

RenderLines render_duration(AgentView person, bool) {
  return {person.duration().show()};
}

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

RenderLines render_deadday(AgentView person, bool) {
  return {person.deadday().show()};
}

RenderLines render_ring(AgentView person, bool) {
  return {fmt::format("{}", static_cast<unsigned int>(person.ring()))};
}

RenderLines render_sdcase(AgentView person, bool) {
  return {bool_text(person.sdcase())};
}

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

RenderLines render_quar(AgentView person, bool) {
  return {bool_text(person.quar())};
}

RenderLines render_quarday(AgentView person, bool) {
  return {person.quarday().show()};
}

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
note:  bool multi_values arg provided by outer function print_agent_pop_table,
       if not false (default value)
struct ColumnSpec key is string label, width, serial_fn       ? multi_value_func ?
*/
constexpr std::array<ColumnSpec, 22> COLUMN_SPECS{{
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

std::vector<ColumnSpec> resolve_columns(std::vector<std::string_view> col_names) {
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

std::vector<ColumnSpec> resolve_columns(std::initializer_list<std::string_view> col_names) {
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

void print_serialized_line(std::ostream &out, std::string_view row_label,
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

void print_serialized_header(std::ostream &out, std::span<const ColumnSpec> columns,
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

} // namespace

std::string render_agent_pop_cell(std::string_view col_name, AgentView person) {
  const ColumnSpec *spec = find_column(col_name);
  if (spec == nullptr) {
    throw std::invalid_argument(fmt::format("Unknown PopData column '{}'", col_name));
  }

  const RenderLines lines = spec->render(person, false);
  if (lines.empty()) return {};
  return lines.front();
}

void write_agent_pop_data(PopData &pop, std::span<const size_t> rows,
                          vector<std::string_view> col_names, std::ostream &out,
                          PopOutputLayout layout, bool multi_values,
                          std::string_view serialized_separator,
                          bool escape_serialized_cells) {
  validate_rows(pop, rows);
  const auto columns = resolve_columns(col_names);
  const bool pretty_layout = layout == PopOutputLayout::pretty;
  const auto header_format = pretty_layout ? build_line_format(columns, false) : std::string{};
  const auto row_format = pretty_layout ? build_line_format(columns, true) : std::string{};

  if (pretty_layout) {
    print_header_line(out, header_format, columns);
    fmt::println(out, "{:-<{}}", "", separator_width(columns));
  } else {
    print_serialized_header(out, columns, serialized_separator, escape_serialized_cells);
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
        print_row_line(out, row_format, row_label, columns, rendered_columns, line_idx);
      } else {
        print_serialized_line(out, row_label, columns, rendered_columns, line_idx,
                              serialized_separator, escape_serialized_cells);
      }
    }
  }
}

void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           vector<std::string_view> col_names,
                           std::ostream &out, bool multi_values) {
  write_agent_pop_data(pop, rows, std::move(col_names), out, PopOutputLayout::pretty,
                       multi_values);
}

void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           std::initializer_list<std::string_view> col_names,
                           std::ostream &out, bool multi_values) {
  write_agent_pop_data(pop, rows,
                       std::vector<std::string_view>{col_names.begin(), col_names.end()}, out,
                       PopOutputLayout::pretty, multi_values);
}


void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           std::string col_name, std::ostream &out,
                           bool multi_values) {
  if (col_name == "all") {
    std::vector<std::string_view> all_names;
    all_names.reserve(COLUMN_SPECS.size());
    for (const auto &spec : COLUMN_SPECS) all_names.push_back(spec.key);
    write_agent_pop_data(pop, rows, all_names, out, PopOutputLayout::pretty, multi_values);
    return;
  }

  throw std::invalid_argument(
      fmt::format("'{}' is not supported here; use \"all\" or an initializer_list of columns",
                  col_name));
}
