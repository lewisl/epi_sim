#include "population_print.h"

#include <algorithm>
#include <array>
#include <fmt/ostream.h>
#include <stdexcept>

namespace {

using CellFormatter = std::string (*)(const PopData &, size_t);
using MultiCellFormatter = std::string (*)(const PopData &, size_t, size_t);
using MultiCountGetter = size_t (*)(const PopData &, size_t);

struct ColumnSpec {
  PopColumn id;
  std::string_view key;
  std::string_view header;
  CellFormatter formatter;
  MultiCountGetter multi_count;
  MultiCellFormatter multi_formatter;
};

std::string format_variant_name(Variant value) {
  const auto idx_value = idx(value);
  if (!Variant::names.empty() && idx_value < Variant::names.size()) {
    return Variant::names[idx_value];
  }
  return fmt::format("{}", static_cast<unsigned int>(static_cast<uint8_t>(value)));
}

template <typename T, size_t N>
const T *latest_value(const std::vector<std::array<T, N>> &values,
                      const std::vector<uint8_t> &counts, size_t row) {
  const auto count = counts[row];
  if (count == 0) {
    return nullptr;
  }

  const size_t value_idx = count >= N ? N - 1 : zidx(count);
  return &values[row][value_idx];
}

template <typename T, size_t N>
const T *stored_value(const std::vector<std::array<T, N>> &values,
                      const std::vector<uint8_t> &counts, size_t row,
                      size_t entry_idx) {
  const size_t stored_count = std::min<size_t>(counts[row], N);
  if (entry_idx >= stored_count) {
    return nullptr;
  }
  return &values[row][entry_idx];
}

template <typename T, size_t N, typename Formatter>
std::string format_latest_or_dash(const std::vector<std::array<T, N>> &values,
                                  const std::vector<uint8_t> &counts,
                                  size_t row, Formatter formatter) {
  const T *value = latest_value(values, counts, row);
  if (value == nullptr) {
    return "-";
  }
  return formatter(*value);
}

template <typename T, size_t N, typename Formatter>
std::string format_stored_or_blank(const std::vector<std::array<T, N>> &values,
                                   const std::vector<uint8_t> &counts,
                                   size_t row, size_t entry_idx,
                                   Formatter formatter) {
  const T *value = stored_value(values, counts, row, entry_idx);
  if (value == nullptr) {
    return "";
  }
  return formatter(*value);
}

size_t stored_count(const std::vector<uint8_t> &counts, size_t row) {
  return std::min<size_t>(counts[row], 16);
}

std::string format_status(const PopData &pop, size_t row) {
  return pop.status[row].name();
}

std::string format_agegrp(const PopData &pop, size_t row) {
  return pop.agegrp[row].name();
}

std::string format_cond(const PopData &pop, size_t row) {
  return pop.cond[row].name();
}

std::string format_duration(const PopData &pop, size_t row) {
  return fmt::format("{}", static_cast<unsigned int>(pop.duration[row]));
}

std::string format_variant(const PopData &pop, size_t row) {
  return format_variant_name(Variant{pop.get_variant(row)});
}

std::string format_variant_count(const PopData &pop, size_t row) {
  return fmt::format("{}", static_cast<unsigned int>(pop.variant_count[row]));
}

std::string format_sickday(const PopData &pop, size_t row) {
  // `make_sick()` appends `variant` and `sickday` in lockstep, so `variant_count`
  // is the event count for both columns and `sickday_count` is not exposed here.
  return format_latest_or_dash(pop.sickday, pop.variant_count, row,
                               [](int16_t value) { return fmt::format("{}", value); });
}

std::string format_recovday(const PopData &pop, size_t row) {
  if (pop.recovday_count[row] == 0) {
    return "-";
  }
  return fmt::format("{}", pop.get_recovday(row));
}

std::string format_recovday_count(const PopData &pop, size_t row) {
  return fmt::format("{}", pop.recovday_count[row]);
}

std::string format_deadday(const PopData &pop, size_t row) {
  return fmt::format("{}", pop.deadday[row]);
}

std::string format_ring(const PopData &pop, size_t row) {
  return fmt::format("{}", static_cast<unsigned int>(pop.ring[row]));
}

std::string format_sdcase(const PopData &pop, size_t row) {
  return pop.true_false.to_str(pop.sdcase[row]);
}

std::string format_tested(const PopData &pop, size_t row) {
  return format_latest_or_dash(pop.tested, pop.tested_count, row, [&](uint8_t value) {
    return pop.true_false.to_str(value);
  });
}

std::string format_tested_count(const PopData &pop, size_t row) {
  return fmt::format("{}", static_cast<unsigned int>(pop.tested_count[row]));
}

std::string format_testday(const PopData &pop, size_t row) {
  return format_latest_or_dash(pop.testday, pop.tested_count, row,
                               [](int16_t value) { return fmt::format("{}", value); });
}

std::string format_quar(const PopData &pop, size_t row) {
  return pop.true_false.to_str(pop.quar[row]);
}

std::string format_quarday(const PopData &pop, size_t row) {
  return fmt::format("{}", pop.quarday[row]);
}

std::string format_vaxstatus(const PopData &pop, size_t row) {
  return pop.vaxstatus[row].name();
}

std::string format_vaxrcvd(const PopData &pop, size_t row) {
  return format_latest_or_dash(pop.vaxrcvd, pop.vax_count, row, [&](uint8_t value) {
    return pop.vax_lbl.to_str(value);
  });
}

std::string format_vax_count(const PopData &pop, size_t row) {
  return fmt::format("{}", static_cast<unsigned int>(pop.vax_count[row]));
}

std::string format_vaxday(const PopData &pop, size_t row) {
  return format_latest_or_dash(pop.vaxday, pop.vax_count, row,
                               [](int16_t value) { return fmt::format("{}", value); });
}

size_t multi_variant_count(const PopData &pop, size_t row) {
  return stored_count(pop.variant_count, row);
}

size_t multi_sickday_count(const PopData &pop, size_t row) {
  return stored_count(pop.variant_count, row);
}

size_t multi_recovday_count(const PopData &pop, size_t row) {
  return stored_count(pop.recovday_count, row);
}

size_t multi_tested_count(const PopData &pop, size_t row) {
  return stored_count(pop.tested_count, row);
}

size_t multi_testday_count(const PopData &pop, size_t row) {
  return stored_count(pop.tested_count, row);
}

size_t multi_vaxrcvd_count(const PopData &pop, size_t row) {
  return stored_count(pop.vax_count, row);
}

size_t multi_vaxday_count(const PopData &pop, size_t row) {
  return stored_count(pop.vax_count, row);
}

std::string format_variant_multi(const PopData &pop, size_t row, size_t entry_idx) {
  return format_stored_or_blank(pop.variant, pop.variant_count, row, entry_idx,
                                [](Variant value) { return format_variant_name(value); });
}

std::string format_sickday_multi(const PopData &pop, size_t row, size_t entry_idx) {
  return format_stored_or_blank(pop.sickday, pop.variant_count, row, entry_idx,
                                [](int16_t value) { return fmt::format("{}", value); });
}

std::string format_recovday_multi(const PopData &pop, size_t row, size_t entry_idx) {
  return format_stored_or_blank(pop.recovday, pop.recovday_count, row, entry_idx,
                                [](int16_t value) { return fmt::format("{}", value); });
}

std::string format_tested_multi(const PopData &pop, size_t row, size_t entry_idx) {
  return format_stored_or_blank(pop.tested, pop.tested_count, row, entry_idx,
                                [&](uint8_t value) { return pop.true_false.to_str(value); });
}

std::string format_testday_multi(const PopData &pop, size_t row, size_t entry_idx) {
  return format_stored_or_blank(pop.testday, pop.tested_count, row, entry_idx,
                                [](int16_t value) { return fmt::format("{}", value); });
}

std::string format_vaxrcvd_multi(const PopData &pop, size_t row, size_t entry_idx) {
  return format_stored_or_blank(pop.vaxrcvd, pop.vax_count, row, entry_idx,
                                [&](uint8_t value) { return pop.vax_lbl.to_str(value); });
}

std::string format_vaxday_multi(const PopData &pop, size_t row, size_t entry_idx) {
  return format_stored_or_blank(pop.vaxday, pop.vax_count, row, entry_idx,
                                [](int16_t value) { return fmt::format("{}", value); });
}

constexpr std::array<ColumnSpec, 21> COLUMN_SPECS{{
    {PopColumn::status, "status", "status", format_status, nullptr, nullptr},
    {PopColumn::agegrp, "agegrp", "agegrp", format_agegrp, nullptr, nullptr},
    {PopColumn::cond, "cond", "cond", format_cond, nullptr, nullptr},
    {PopColumn::duration, "duration", "duration", format_duration, nullptr, nullptr},
    {PopColumn::variant, "variant", "variant", format_variant, multi_variant_count, format_variant_multi},
    {PopColumn::variant_count, "variant_count", "variant_count", format_variant_count, nullptr, nullptr},
    {PopColumn::sickday, "sickday", "sickday", format_sickday, multi_sickday_count, format_sickday_multi},
    {PopColumn::recovday, "recovday", "recovday", format_recovday, multi_recovday_count, format_recovday_multi},
    {PopColumn::recovday_count, "recovday_count", "recovday_count", format_recovday_count, nullptr, nullptr},
    {PopColumn::deadday, "deadday", "deadday", format_deadday, nullptr, nullptr},
    {PopColumn::ring, "ring", "ring", format_ring, nullptr, nullptr},
    {PopColumn::sdcase, "sdcase", "sdcase", format_sdcase, nullptr, nullptr},
    {PopColumn::tested, "tested", "tested", format_tested, multi_tested_count, format_tested_multi},
    {PopColumn::tested_count, "tested_count", "tested_count", format_tested_count, nullptr, nullptr},
    {PopColumn::testday, "testday", "testday", format_testday, multi_testday_count, format_testday_multi},
    {PopColumn::quar, "quar", "quar", format_quar, nullptr, nullptr},
    {PopColumn::quarday, "quarday", "quarday", format_quarday, nullptr, nullptr},
    {PopColumn::vaxstatus, "vaxstatus", "vaxstatus", format_vaxstatus, nullptr, nullptr},
    {PopColumn::vaxrcvd, "vaxrcvd", "vaxrcvd", format_vaxrcvd, multi_vaxrcvd_count, format_vaxrcvd_multi},
    {PopColumn::vax_count, "vax_count", "vax_count", format_vax_count, nullptr, nullptr},
    {PopColumn::vaxday, "vaxday", "vaxday", format_vaxday, multi_vaxday_count, format_vaxday_multi},
}};

const ColumnSpec &column_spec(PopColumn column) {
  const auto column_idx = static_cast<size_t>(column);
  if (column_idx >= COLUMN_SPECS.size()) {
    throw std::invalid_argument("Invalid PopColumn enum value");
  }
  return COLUMN_SPECS[column_idx];
}

const absl::flat_hash_map<std::string_view, PopColumn> &column_name_map() {
  static const auto names = [] {
    absl::flat_hash_map<std::string_view, PopColumn> map;
    for (const auto &spec : COLUMN_SPECS) {
      map.emplace(spec.key, spec.id);
    }
    return map;
  }();
  return names;
}

void validate_rows(const PopData &pop, std::span<const size_t> rows) {
  for (const auto row : rows) {
    if (row == 0 || row > pop.popn) {
      throw std::invalid_argument(
          fmt::format("Row {} is out of range; valid rows are 1..{}", row, pop.popn));
    }
  }
}

} // namespace

void print_pop_table(const PopData &pop, std::span<const size_t> rows,
                     std::span<const PopColumn> cols, std::ostream &out,
                     bool multi_values) {
  validate_rows(pop, rows);

  std::vector<const ColumnSpec *> specs;
  specs.reserve(cols.size());
  for (const auto column : cols) {
    specs.push_back(&column_spec(column));
  }

  std::vector<size_t> widths;
  widths.reserve(specs.size() + 1);
  widths.push_back(std::string_view{"row"}.size());
  for (const auto *spec : specs) {
    widths.push_back(spec->header.size());
  }

  std::vector<std::vector<std::string>> rendered_rows;
  for (const auto row : rows) {
    widths[0] = std::max(widths[0], fmt::formatted_size("{}", row));
    widths[0] = std::max(widths[0], std::string_view{"*"}.size());

    size_t line_count = 1;
    if (multi_values) {
      for (const auto *spec : specs) {
        if (spec->multi_count != nullptr) {
          line_count = std::max(line_count, spec->multi_count(pop, row));
        }
      }
    }

    for (size_t line_idx = 0; line_idx < line_count; ++line_idx) {
      std::vector<std::string> rendered_cells;
      rendered_cells.reserve(specs.size() + 1);

      std::string row_label =
          line_idx == 0 ? fmt::format("{}", row) : std::string{"*"};
      widths[0] = std::max(widths[0], row_label.size());
      rendered_cells.push_back(std::move(row_label));

      for (size_t column_idx = 0; column_idx < specs.size(); ++column_idx) {
        const auto *spec = specs[column_idx];
        std::string cell;

        if (!multi_values || spec->multi_count == nullptr) {
          cell = line_idx == 0 ? spec->formatter(pop, row) : "";
        } else {
          const size_t spec_count = spec->multi_count(pop, row);
          if (spec_count == 0) {
            cell = line_idx == 0 ? spec->formatter(pop, row) : "";
          } else if (line_idx < spec_count) {
            cell = spec->multi_formatter(pop, row, line_idx);
          } else {
            cell.clear();
          }
        }

        widths[column_idx + 1] = std::max(widths[column_idx + 1], cell.size());
        rendered_cells.push_back(std::move(cell));
      }

      rendered_rows.push_back(std::move(rendered_cells));
    }
  }

  auto append_cell = [&](std::string &line, std::string_view cell, size_t width,
                         bool right_align) {
    if (right_align) {
      fmt::format_to(std::back_inserter(line), "{:>{}}", cell, width);
    } else {
      fmt::format_to(std::back_inserter(line), "{:<{}}", cell, width);
    }
  };

  std::string header_line;
  append_cell(header_line, "row", widths[0], false);
  for (size_t column_idx = 0; column_idx < specs.size(); ++column_idx) {
    header_line += "  ";
    append_cell(header_line, specs[column_idx]->header, widths[column_idx + 1], false);
  }
  fmt::print(out, "{}\n", header_line);

  size_t separator_width = widths[0];
  for (size_t column_idx = 1; column_idx < widths.size(); ++column_idx) {
    separator_width += 2 + widths[column_idx];
  }
  fmt::print(out, "{}\n", std::string(separator_width, '-'));

  for (const auto &rendered_row : rendered_rows) {
    std::string row_line;
    append_cell(row_line, rendered_row[0], widths[0], true);
    for (size_t column_idx = 0; column_idx < specs.size(); ++column_idx) {
      row_line += "  ";
      append_cell(row_line, rendered_row[column_idx + 1], widths[column_idx + 1], false);
    }
    fmt::print(out, "{}\n", row_line);
  }
}

void print_pop_table(const PopData &pop, std::span<const size_t> rows,
                     std::span<const std::string_view> col_names,
                     std::ostream &out, bool multi_values) {
  std::vector<PopColumn> resolved_cols;
  resolved_cols.reserve(col_names.size());

  for (const auto name : col_names) {
    const auto it = column_name_map().find(name);
    if (it == column_name_map().end()) {
      throw std::invalid_argument(
          fmt::format("Unknown PopData column '{}'", name));
    }
    resolved_cols.push_back(it->second);
  }

  print_pop_table(pop, rows,
                  std::span<const PopColumn>{resolved_cols.data(), resolved_cols.size()},
                  out, multi_values);
}
