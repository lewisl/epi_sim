#include "agent_pop_print.h"

#include <algorithm>
#include <array>
#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <stdexcept>
#include <vector>

namespace {

std::string bool_text(uint8_t value) {
  return value == 0 ? std::string{"false"} : std::string{"true"};
}

using RowView = AgentView;
using RenderLines = std::vector<std::string>;
using RenderFn = RenderLines (*)(RowView, bool);

struct ColumnSpec {
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

RenderLines render_status(RowView person, bool) { return {person.status().show()}; }

RenderLines render_agegrp(RowView person, bool) { return {person.agegrp().show()}; }

RenderLines render_cond(RowView person, bool) { return {person.cond().show()}; }

RenderLines render_duration(RowView person, bool) {
  return {person.duration().show()};
}

RenderLines render_variant(RowView person, bool) {
  return {format_variant_name(person.variant())};
}

RenderLines render_variant_hist(RowView person, bool multi_values) {
  const auto& history = person.variant_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](Variant value) { return format_variant_name(value); },
      scalar_value);
}

RenderLines render_sickday(RowView person, bool) {
  return {person.sickday() == 0 ? std::string{"-"} : person.sickday().show()};
}

RenderLines render_sickday_hist(RowView person, bool multi_values) {
  const auto& history = person.sickday_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](int16_t value) { return fmt::format("{}", value); },
      scalar_value);
}

RenderLines render_recovday(RowView person, bool multi_values) {
  (void)multi_values;
  return {person.recovday() == 0 ? std::string{"-"} : person.recovday().show()};
}

RenderLines render_recovday_hist(RowView person, bool multi_values) {
  const auto& history = person.recovday_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](int16_t value) { return fmt::format("{}", value); },
      scalar_value);
}

RenderLines render_deadday(RowView person, bool) {
  return {person.deadday().show()};
}

RenderLines render_ring(RowView person, bool) {
  return {fmt::format("{}", static_cast<unsigned int>(person.ring()))};
}

RenderLines render_sdcase(RowView person, bool) {
  return {bool_text(person.sdcase())};
}

RenderLines render_tested(RowView person, bool multi_values) {
  (void)multi_values;
  return {person.tested() == 0 ? std::string{"-"} : bool_text(person.tested())};
}

RenderLines render_testday_hist(RowView person, bool multi_values) {
  const auto& history = person.testday_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](int16_t value) { return fmt::format("{}", value); },
      scalar_value);
}

RenderLines render_testday(RowView person, bool multi_values) {
  (void)multi_values;
  return {person.testday() == 0 ? std::string{"-"} : person.testday().show()};
}

RenderLines render_quar(RowView person, bool) {
  return {bool_text(person.quar())};
}

RenderLines render_quarday(RowView person, bool) {
  return {person.quarday().show()};
}

RenderLines render_vaxstatus(RowView person, bool) { return {person.vaxstatus().name()}; }

RenderLines render_vaxrcvd(RowView person, bool) {
  return {idx(person.vaxrcvd()) == 0 ? std::string{"-"} : format_vax_name(person.vaxrcvd())};
}

RenderLines render_vax_hist(RowView person, bool multi_values) {
  const auto& history = person.vax_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](Vax value) { return format_vax_name(value); },
      scalar_value);
}

RenderLines render_vaxday(RowView person, bool) {
  return {person.vaxday() == 0 ? std::string{"-"} : person.vaxday().show()};
}

RenderLines render_vaxday_hist(RowView person, bool multi_values) {
  const auto& history = person.vaxday_hist();
  const auto scalar_value = history.count == 0 ? std::string{"-"} : history.show();
  return render_history(
      history.arr, history.count, multi_values,
      [](int16_t value) { return fmt::format("{}", value); },
      scalar_value);
}

constexpr std::array<ColumnSpec, 23> COLUMN_SPECS{{
    {"status", 10, render_status},
    {"agegrp", 10, render_agegrp},
    {"cond", 10, render_cond},
    {"duration", 8, render_duration},
    {"variant", 10, render_variant},
    {"variant_hist", 12, render_variant_hist},
    {"sickday", 7, render_sickday},
    {"sickday_hist", 11, render_sickday_hist},
    {"recovday", 8, render_recovday},
    {"recovday_hist", 13, render_recovday_hist},
    {"deadday", 7, render_deadday},
    {"ring", 4, render_ring},
    {"sdcase", 6, render_sdcase},
    {"tested", 6, render_tested},
    {"testday_hist", 12, render_testday_hist},
    {"testday", 7, render_testday},
    {"quar", 5, render_quar},
    {"quarday", 7, render_quarday},
    {"vaxstatus", 10, render_vaxstatus},
    {"vaxrcvd", 10, render_vaxrcvd},
    {"vax_hist", 10, render_vax_hist},
    {"vaxday", 6, render_vaxday},
    {"vaxday_hist", 11, render_vaxday_hist},
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

} // namespace


void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           vector<std::string_view> col_names,
                           std::ostream &out, bool multi_values) {
  validate_rows(pop, rows);
  const auto columns = resolve_columns(col_names);
  const auto header_format = build_line_format(columns, false);
  const auto row_format = build_line_format(columns, true);

  print_header_line(out, header_format, columns);
  fmt::println(out, "{:-<{}}", "", separator_width(columns));

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
      print_row_line(out, row_format, row_label, columns, rendered_columns, line_idx);
    }
  }
}

void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           std::initializer_list<std::string_view> col_names,
                           std::ostream &out, bool multi_values) {
  print_agent_pop_table(pop, rows,
                        std::vector<std::string_view>{col_names.begin(), col_names.end()},
                        out, multi_values);
}


void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           std::string col_name, std::ostream &out,
                           bool multi_values) {
  if (col_name == "all") {
    std::vector<std::string_view> all_names;
    all_names.reserve(COLUMN_SPECS.size());
    for (const auto &spec : COLUMN_SPECS) all_names.push_back(spec.key);
    // call the span-based overload (you may need to add/adjust one)
    print_agent_pop_table(pop, rows, all_names, out, multi_values);
    return;
  }

  throw std::invalid_argument(
      fmt::format("'{}' is not supported here; use \"all\" or an initializer_list of columns",
                  col_name));
}
