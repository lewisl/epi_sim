#pragma once

#include "population.h"

#include <initializer_list>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

enum class Style : uint8_t {
  pretty,
  serialized,
};


using RenderLines = std::vector<std::string>;
using RenderFn = RenderLines (*)(AgentView, bool);

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


struct ColumnSpec {
  ColumnName name;
  std::string key;
  size_t width;
  RenderFn render;
};


std::filesystem::path set_output_file(string base_fname, vector<string> path_steps);

std::string render_pop_cell(std::string_view col_name, AgentView person);

vector<std::string> get_all_column_names();

struct ColSpec {
    std::vector<std::string> names;

    ColSpec(std::vector<std::string> v) : names(std::move(v)) {}
    ColSpec(std::initializer_list<std::string> v) : names(v) {}
    ColSpec(const char* s)  { // ColSpec(std::string_view s)
        if (std::string_view(s) == "all") names = std::move(get_all_column_names());
        else throw std::invalid_argument("only valid sentinel value is \"all\""); }
};

struct OutSpec {
    enum class Kind { Stream, Path } kind;
    std::ostream* out = nullptr;
    std::filesystem::path path;

    OutSpec(std::ostream& s) : kind(Kind::Stream), out(&s) {}
    OutSpec(std::filesystem::path p) : kind(Kind::Path), path(std::move(p)) {}
};

void write_pop_data(
        PopData &pop, 
        std::span<const size_t> rows,
        ColSpec col_names, 
        OutSpec target,
        Style layout, 
        bool multi_values,
        std::string_view serialized_separator,
        bool escape_serialized_cells); 
