# `print_pop_table`

`print_pop_table` prints selected rows from `PopData` as an aligned table. It is declared in [src/population_print.h](/Users/lewislevin/code/epi_sim/src/population_print.h).

## Typed overload

Use this when you are writing normal C++ inside the project and know the columns at compile time.

```cpp
#include "population_print.h"

using PC = PopColumn;

std::vector<size_t> rows = {1, 25, 50};
std::vector<PC> cols = {
    PC::status,
    PC::agegrp,
    PC::variant,
    PC::variant_count,
    PC::sickday
};

print_pop_table(pop, rows, cols);
```

Optional arguments:

```cpp
print_pop_table(pop, rows, cols, std::cout, true);
```

- Fourth argument: output stream, default `std::cout`
- Fifth argument: `multi_values`, default `false`

## Runtime string overload

Use this when column names are chosen at runtime.

```cpp
std::vector<std::string_view> cols = {
    "status",
    "agegrp",
    "vaxstatus",
    "vaxrcvd",
    "vaxday"
};

print_pop_table(pop, rows, cols);
```

If a string does not match a known column name, the function throws `std::invalid_argument`.

## `multi_values`

Array-backed columns such as `variant`, `sickday`, `tested`, `testday`, `vaxrcvd`, and `vaxday` can hold multiple event values per person.

- `multi_values = false`: print the most recent value only
- `multi_values = true`: print one line per stored event value, with continuation lines marked by `*` in the row column

Example:

```text
  25  alpha  2  4   false  6
   *  delta     11  true   12
```

Scalar columns are printed only on the first line for that person and left blank on continuation lines.

## Practical notes

- Rows must follow the project’s 1-based indexing convention: valid rows are `1..pop.popn`
- Row `0` is invalid and will throw
- `sickday` uses `variant_count` as its event count because `make_sick()` updates `variant` and `sickday` in lockstep
- The available runtime string names are the keys listed in the `COLUMN_SPECS` table in [src/population_print.cpp](/Users/lewislevin/code/epi_sim/src/population_print.cpp)
