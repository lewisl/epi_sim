#include "simple_print.h"

#include <algorithm>
#include <functional>
#include <stdexcept>

namespace {

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

std::string format_variant_name(Variant value) {
  const auto idx_value = idx(value);
  if (!Variant::names.empty() && idx_value < Variant::names.size()) {
    return Variant::names[idx_value];
  }
  return fmt::format("{}", static_cast<unsigned int>(static_cast<uint8_t>(value)));
}

struct ConstRowView {
  const PopData &pop;
  size_t row;

  const Status &status() const { return pop.status[row]; }
  const Agegrp &agegrp() const { return pop.agegrp[row]; }
  const Condition &cond() const { return pop.cond[row]; }
  uint8_t duration() const { return pop.duration[row]; }
  uint8_t variant_count() const { return pop.variant_count[row]; }
  uint8_t recovday_count() const { return pop.recovday_count[row]; }
  int16_t deadday() const { return pop.deadday[row]; }
  uint8_t ring() const { return pop.ring[row]; }
  uint8_t sdcase() const { return pop.sdcase[row]; }
  uint8_t tested_count() const { return pop.tested_count[row]; }
  uint8_t quar() const { return pop.quar[row]; }
  int16_t quarday() const { return pop.quarday[row]; }
  const Vaxstatus &vaxstatus() const { return pop.vaxstatus[row]; }
  uint8_t vax_count() const { return pop.vax_count[row]; }

  Variant latest_variant() const {
    if (const auto *value = latest_value(pop.variant, pop.variant_count, row)) {
      return *value;
    }
    return Variant{};
  }

  const int16_t *latest_sickday() const {
    return latest_value(pop.sickday, pop.variant_count, row);
  }

  const int16_t *latest_recovday() const {
    return latest_value(pop.recovday, pop.recovday_count, row);
  }

  const uint8_t *latest_tested() const {
    return latest_value(pop.tested, pop.tested_count, row);
  }

  const int16_t *latest_testday() const {
    return latest_value(pop.testday, pop.tested_count, row);
  }

  const uint8_t *latest_vaxrcvd() const {
    return latest_value(pop.vaxrcvd, pop.vax_count, row);
  }

  const int16_t *latest_vaxday() const {
    return latest_value(pop.vaxday, pop.vax_count, row);
  }
};

using CellFn = std::function<std::string(const ConstRowView &)>;
using ColumnMap = absl::flat_hash_map<std::string_view, CellFn>;

const ColumnMap &column_map() {
  static const ColumnMap columns{
      {"status", [](const ConstRowView &row) { return row.status().name(); }},
      {"agegrp", [](const ConstRowView &row) { return row.agegrp().name(); }},
      {"cond", [](const ConstRowView &row) { return row.cond().name(); }},
      {"duration", [](const ConstRowView &row) {
         return fmt::format("{}", static_cast<unsigned int>(row.duration()));
       }},
      {"variant", [](const ConstRowView &row) {
         return format_variant_name(row.latest_variant());
       }},
      {"variant_count", [](const ConstRowView &row) {
         return fmt::format("{}", static_cast<unsigned int>(row.variant_count()));
       }},
      {"sickday", [](const ConstRowView &row) {
         if (const auto *value = row.latest_sickday()) {
           return fmt::format("{}", *value);
         }
         return std::string{"-"};
       }},
      {"recovday", [](const ConstRowView &row) {
         if (const auto *value = row.latest_recovday()) {
           return fmt::format("{}", *value);
         }
         return std::string{"-"};
       }},
      {"recovday_count", [](const ConstRowView &row) {
         return fmt::format("{}", static_cast<unsigned int>(row.recovday_count()));
       }},
      {"deadday", [](const ConstRowView &row) {
         return fmt::format("{}", row.deadday());
       }},
      {"ring", [](const ConstRowView &row) {
         return fmt::format("{}", static_cast<unsigned int>(row.ring()));
       }},
      {"sdcase", [](const ConstRowView &row) {
         return row.pop.true_false.to_str(row.sdcase());
       }},
      {"tested", [](const ConstRowView &row) {
         if (const auto *value = row.latest_tested()) {
           return row.pop.true_false.to_str(*value);
         }
         return std::string{"-"};
       }},
      {"tested_count", [](const ConstRowView &row) {
         return fmt::format("{}", static_cast<unsigned int>(row.tested_count()));
       }},
      {"testday", [](const ConstRowView &row) {
         if (const auto *value = row.latest_testday()) {
           return fmt::format("{}", *value);
         }
         return std::string{"-"};
       }},
      {"quar", [](const ConstRowView &row) {
         return row.pop.true_false.to_str(row.quar());
       }},
      {"quarday", [](const ConstRowView &row) {
         return fmt::format("{}", row.quarday());
       }},
      {"vaxstatus", [](const ConstRowView &row) { return row.vaxstatus().name(); }},
      {"vaxrcvd", [](const ConstRowView &row) {
         if (const auto *value = row.latest_vaxrcvd()) {
           return row.pop.vax_lbl.to_str(*value);
         }
         return std::string{"-"};
       }},
      {"vax_count", [](const ConstRowView &row) {
         return fmt::format("{}", static_cast<unsigned int>(row.vax_count()));
       }},
      {"vaxday", [](const ConstRowView &row) {
         if (const auto *value = row.latest_vaxday()) {
           return fmt::format("{}", *value);
         }
         return std::string{"-"};
       }},
  };
  return columns;
}

void validate_rows(const PopData &pop, std::span<const size_t> rows) {
  for (const auto row : rows) {
    if (row == 0 || row > pop.popn) {
      throw std::invalid_argument(
          fmt::format("Row {} is out of range; valid rows are 1..{}", row, pop.popn));
    }
  }
}

void append_cell(std::string &line, std::string_view cell, size_t width,
                 bool right_align) {
  if (right_align) {
    fmt::format_to(std::back_inserter(line), "{:>{}}", cell, width);
  } else {
    fmt::format_to(std::back_inserter(line), "{:<{}}", cell, width);
  }
}

} // namespace

void print_simple_pop(const PopData &pop, std::span<const size_t> rows,
                      std::span<const std::string_view> col_names,
                      std::ostream &out) {
  validate_rows(pop, rows);

  std::vector<std::pair<std::string_view, const CellFn *>> columns;
  columns.reserve(col_names.size());
  for (const auto name : col_names) {
    const auto it = column_map().find(name);
    if (it == column_map().end()) {
      throw std::invalid_argument(fmt::format("Unknown PopData column '{}'", name));
    }
    columns.emplace_back(name, &it->second);
  }

  std::vector<size_t> widths;
  widths.reserve(columns.size() + 1);
  widths.push_back(std::string_view{"row"}.size());
  for (const auto &[name, fn] : columns) {
    (void)fn;
    widths.push_back(name.size());
  }

  std::vector<std::vector<std::string>> rendered_rows;
  rendered_rows.reserve(rows.size());
  for (const auto row : rows) {
    ConstRowView view{pop, row};

    widths[0] = std::max(widths[0], fmt::formatted_size("{}", row));
    std::vector<std::string> rendered_cells;
    rendered_cells.reserve(columns.size() + 1);
    rendered_cells.push_back(fmt::format("{}", row));

    for (size_t column_idx = 0; column_idx < columns.size(); ++column_idx) {
      const auto &cell = (*columns[column_idx].second)(view);
      widths[column_idx + 1] = std::max(widths[column_idx + 1], cell.size());
      rendered_cells.push_back(cell);
    }

    rendered_rows.push_back(std::move(rendered_cells));
  }

  std::string header_line;
  append_cell(header_line, "row", widths[0], false);
  for (size_t column_idx = 0; column_idx < columns.size(); ++column_idx) {
    header_line += "  ";
    append_cell(header_line, columns[column_idx].first, widths[column_idx + 1], false);
  }
  out << header_line << '\n';

  size_t separator_width = widths[0];
  for (size_t column_idx = 1; column_idx < widths.size(); ++column_idx) {
    separator_width += 2 + widths[column_idx];
  }
  out << std::string(separator_width, '-') << '\n';

  for (const auto &rendered_row : rendered_rows) {
    std::string row_line;
    append_cell(row_line, rendered_row[0], widths[0], true);
    for (size_t column_idx = 0; column_idx < columns.size(); ++column_idx) {
      row_line += "  ";
      append_cell(row_line, rendered_row[column_idx + 1], widths[column_idx + 1], false);
    }
    out << row_line << '\n';
  }
}
