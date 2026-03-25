#pragma once

#include "population.h"

#include <initializer_list>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>

void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           std::initializer_list<std::string_view> col_names,
                           std::ostream &out = std::cout,
                           bool multi_values = false);

void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           std::string col_name,
                           std::ostream &out = std::cout,
                           bool multi_values = false);

void print_agent_pop_table(PopData &pop, std::span<const size_t> rows,
                           vector<std::string_view> col_names,
                           std::ostream &out, bool multi_values);
