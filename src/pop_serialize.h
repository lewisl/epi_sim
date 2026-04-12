#pragma once

#include "population.h"

#include <initializer_list>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

enum class PopOutputLayout : uint8_t {
  pretty,
  serialized,
};

std::string render_agent_pop_cell(std::string_view col_name, AgentView person);

void write_agent_pop_data(PopData &pop, std::span<const size_t> rows,
                          vector<std::string_view> col_names, std::ostream &out,
                          PopOutputLayout layout = PopOutputLayout::pretty,
                          bool multi_values = false,
                          std::string_view serialized_separator = ",",
                          bool escape_serialized_cells = false);

void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           std::initializer_list<std::string_view> col_names,
                           std::ostream &out = std::cout, bool multi_values = false);

void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           std::string col_name,
                           std::ostream &out = std::cout, bool multi_values = false);

void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           vector<std::string_view> col_names, 
                           std::ostream &out, bool multi_values);
