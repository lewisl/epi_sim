# Printing PopData Tables with String Representations

## Goal
Add ability to print PopData tables with string representations of enum values instead of raw integers.

## Current Behavior
- `print_table()` prints raw `uint8_t` values: `1, 2, 3, 4...`
- Example: `status` column shows `1` instead of `"unexposed"`

## Desired Behavior
- New `print_table_strings()` prints human-readable strings
- Example: `status` column shows `"unexposed"` instead of `1`

## Design Principles
1. **No redundant switches** - Use the existing switch in `apply_to_columns`
2. **Uniform action interface** - Action receives function pointer + vector
3. **Handle both static enums and RuntimeEnums** - Use lambdas to unify interface

---

## Implementation

### 1. Modify `apply_to_columns` signature

Add optional RuntimeEnum parameters for `variant` and `vaxrcvd` columns:

```cpp
template <typename Action>
void apply_to_columns(const vector<Column> &cols, 
                     Action &&action,
                     const RuntimeEnum* variants = nullptr,
                     const RuntimeEnum* vaxlist = nullptr) {
    for (auto col : cols) {
        switch (col) {
            case Column::status: {
                auto to_str = Status::to_str;
                action(to_str, status);
                break;
            }
            case Column::agegrp: {
                auto to_str = Agegrp::to_str;
                action(to_str, agegrp);
                break;
            }
            case Column::cond: {
                auto to_str = Condition::to_str;
                action(to_str, cond);
                break;
            }
            case Column::duration: {
                auto to_str = nullptr;  // plain int, no conversion
                action(to_str, duration);
                break;
            }
            case Column::variant: {
                auto to_str = [variants](uint8_t v) -> const char* { 
                    static std::string temp;
                    temp = variants ? variants->to_string(v) : "?";
                    return temp.c_str();
                };
                action(to_str, variant);
                break;
            }
            case Column::variant_count: {
                auto to_str = nullptr;
                action(to_str, variant_count);
                break;
            }
            case Column::sickday: {
                auto to_str = nullptr;
                action(to_str, sickday);
                break;
            }
            case Column::sickday_count: {
                auto to_str = nullptr;
                action(to_str, sickday_count);
                break;
            }
            case Column::recovday: {
                auto to_str = nullptr;
                action(to_str, recovday);
                break;
            }
            case Column::recovday_count: {
                auto to_str = nullptr;
                action(to_str, recovday_count);
                break;
            }
            case Column::deadday: {
                auto to_str = nullptr;
                action(to_str, deadday);
                break;
            }
            case Column::ring: {
                auto to_str = nullptr;
                action(to_str, ring);
                break;
            }
            case Column::sdcase: {
                auto to_str = nullptr;
                action(to_str, sdcase);
                break;
            }
            case Column::tested: {
                auto to_str = nullptr;
                action(to_str, tested);
                break;
            }
            case Column::tested_count: {
                auto to_str = nullptr;
                action(to_str, tested_count);
                break;
            }
            case Column::quar: {
                auto to_str = nullptr;
                action(to_str, quar);
                break;
            }
            case Column::quarday: {
                auto to_str = nullptr;
                action(to_str, quarday);
                break;
            }
            case Column::vaxstatus: {
                auto to_str = nullptr;  // TODO: add VaxStatus enum if needed
                action(to_str, vaxstatus);
                break;
            }
            case Column::vaxrcvd: {
                auto to_str = [vaxlist](uint8_t v) -> const char* { 
                    static std::string temp;
                    temp = vaxlist ? vaxlist->to_string(v) : "?";
                    return temp.c_str();
                };
                action(to_str, vaxrcvd);
                break;
            }
            case Column::vax_count: {
                auto to_str = nullptr;
                action(to_str, vax_count);
                break;
            }
            case Column::vaxday: {
                auto to_str = nullptr;
                action(to_str, vaxday);
                break;
            }
        }  // switch
    }  // for
}
```

### 2. Add `CellPrinterString` functor

New functor that uses the `to_str` function pointer to convert values to strings:

```cpp
struct CellPrinterString {
    int current_row;

    // For vector<uint8_t> columns
    void operator()(auto to_str, const vector<uint8_t> &vec) const {
        if (to_str) {
            std::cout << "   " << to_str(vec[current_row]) << "\t|";
        } else {
            std::cout << "   " << (int)vec[current_row] << "\t|";
        }
    }

    // For vector<array<uint8_t, 16>> columns
    void operator()(auto to_str, const vector<array<uint8_t, 16>> &vec) const {
        if (to_str) {
            std::cout << "   " << to_str(vec[current_row][0]) << "...\t|";
        } else {
            std::cout << "   " << (int)vec[current_row][0] << "...\t|";
        }
    }
};
```

### 3. Add `print_table_strings()` method

```cpp
void print_table_strings(const vector<int> &rows,
                        const vector<Column> &cols,
                        const RuntimeEnum* variants = nullptr,
                        const RuntimeEnum* vaxlist = nullptr) {
    for (int r : rows) {
        std::cout << r << ":\t";
        CellPrinterString printer{r};
        apply_to_columns(cols, printer, variants, vaxlist);
        std::cout << "\n";
    }
}
```

---

## Usage Example

```cpp
// In test.cpp or wherever you use PopData
PopData pop(100);

// ... populate data ...

// Print with raw integers (existing behavior)
pop.print_table({0, 1, 2}, {PC::status, PC::agegrp, PC::variant});
// Output: 0:   1  |   2  |   0...  |

// Print with strings (new behavior)
pop.print_table_strings({0, 1, 2},
                       {PC::status, PC::agegrp, PC::variant},
                       &mp.variants,  // RuntimeEnum for variants
                       &mp.vaxlist);  // RuntimeEnum for vaccines
// Output: 0:   unexposed  |   age20_39  |   base...  |
```

---

## Notes

### Lambda Lifetime Issue
The lambdas for RuntimeEnum columns use `static std::string temp` to ensure the returned `c_str()` pointer remains valid. This is **not thread-safe** but works for single-threaded printing.

**Alternative**: Return `std::string` instead of `const char*` and modify the action to handle both:

```cpp
// Option: Use std::string instead
void operator()(auto to_str, const vector<uint8_t> &vec) const {
    if (to_str) {
        auto result = to_str(vec[current_row]);
        if constexpr (std::is_same_v<decltype(result), const char*>) {
            std::cout << "   " << result << "\t|";
        } else {
            std::cout << "   " << result << "\t|";  // std::string
        }
    } else {
        std::cout << "   " << (int)vec[current_row] << "\t|";
    }
}
```

### Backward Compatibility
The existing `print_table()` and `CellPrinter` remain unchanged. The new functionality is purely additive.

### Future Enhancements
- Add `VaxStatus` enum for the `vaxstatus` column
- Consider using `fmt::print` instead of `std::cout` for better formatting
- Add column headers to the output

---

## Summary

**Key insight**: The switch statement in `apply_to_columns` already knows which column it's processing. By setting the `to_str` function pointer in each case block, we avoid redundant switches in the action functor.

**Result**:
- One switch (in `apply_to_columns`)
- Zero switches in the action
- Uniform interface for all column types
- Both static enums and RuntimeEnums handled seamlessly
