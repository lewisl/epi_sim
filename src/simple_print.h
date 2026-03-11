#pragma once

#include "population.h"

#include <iosfwd>
#include <span>
#include <string_view>

void print_simple_pop(const PopData &pop, std::span<const size_t> rows,
                      std::span<const std::string_view> col_names,
                      std::ostream &out = std::cout);
