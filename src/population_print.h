#pragma once

#include "population.h"

#include <iosfwd>
#include <span>
#include <string_view>

enum class PopColumn {
  status,
  agegrp,
  cond,
  duration,
  variant,
  variant_count,
  sickday,
  recovday,
  recovday_count,
  deadday,
  ring,
  sdcase,
  tested,
  tested_count,
  testday,
  quar,
  quarday,
  vaxstatus,
  vaxrcvd,
  vax_count,
  vaxday
};

void print_pop_table(const PopData &pop, std::span<const size_t> rows,
                     std::span<const PopColumn> cols,
                     std::ostream &out = std::cout,
                     bool multi_values = false);

void print_pop_table(const PopData &pop, std::span<const size_t> rows,
                     std::span<const std::string_view> col_names,
                     std::ostream &out = std::cout,
                     bool multi_values = false);
