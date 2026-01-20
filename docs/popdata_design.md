# PopData Design - Agent-Based Model Storage

## Overview

`PopData` is the core data structure for the Agent-Based Model (ABM). It stores simulation state for every person in the population using a **Struct of Arrays (SoA)** pattern, where each vector represents one attribute across all people.

## Design Pattern: Struct of Arrays (SoA)

### Concept
Instead of an array of structs (one struct per person), we use a struct of arrays (one array per attribute).

```cpp
// ❌ Array of Structs (AoS) - Poor cache locality
struct Person {
    uint8_t status;
    uint8_t agegrp;
    uint8_t cond;
    // ... 20+ fields
};
vector<Person> population;  // Each person is a separate struct

// ✅ Struct of Arrays (SoA) - Excellent cache locality
class PopData {
    vector<uint8_t> status;   // All statuses together
    vector<uint8_t> agegrp;   // All age groups together
    vector<uint8_t> cond;     // All conditions together
    // ... 20+ vectors
};
```

### Why SoA is Better for Simulation

**Cache Locality:**
```cpp
// Update status for infected people (common operation)
for (size_t i : infected_indices) {
    popdata.status[i] = Status::recovered;  // Sequential memory access
}
```

With AoS, accessing `person[i].status` jumps around memory (each person is ~30 bytes apart). With SoA, all status values are contiguous in memory → better cache performance.

**SIMD Potential:**
Sequential data enables vectorization (SIMD instructions can process multiple people simultaneously).

**Selective Updates:**
Only load the vectors you need. If updating status, you don't load vaccination history into cache.

## Critical Constraint: Row Alignment

**All vectors MUST stay aligned** - index `i` represents the same person across all vectors.

```cpp
// Person at index 42:
popdata.status[42]    // Their infection status
popdata.agegrp[42]    // Their age group
popdata.vaxstatus[42] // Their vaccination status
// All refer to the SAME person
```

**Implications:**
- ✅ Can add/remove people (resize all vectors together)
- ❌ Cannot reorder one vector independently
- ✅ Can use same index across all attributes
- ❌ Must maintain alignment during updates

## Pseudo-Enums: uint8_t with String Mapping

### The Problem
C++ enums are strongly typed - each enum is a different type. Can't store different enum types in the same container or use generic code.

### Julia Solution
Julia uses **Symbols** (`:unexposed`, `:infectious`, etc.):
- Compact representation (like interned strings)
- Fast equality comparison (pointer comparison)
- Self-documenting (human-readable)

### C++ Solution
Use `uint8_t` everywhere with companion namespaces for string conversion:

```cpp
// categories.h
namespace Status {
    enum Value : uint8_t { none = 0, unexposed, infectious, recovered, dead };
    
    const char* to_str(uint8_t v);      // Index → String (O(1) array lookup)
    uint8_t from_str(const string& s);  // String → Index (O(1) branching)
}

// Usage in PopData
vector<uint8_t> status;  // Stores Status::Value as uint8_t

// Printing
cout << Status::to_str(popdata.status[i]);  // "infectious"

// Parsing
popdata.status[i] = Status::from_str("recovered");  // 3
```

### Why uint8_t?

1. **Uniform type** - All categorical columns are `vector<uint8_t>`, enables generic code
2. **Compact** - 1 byte per person (vs 4 bytes for `int` or 8 bytes for `enum class`)
3. **Fast** - Integer comparison and indexing
4. **Sufficient range** - 0-255 covers all our categories (status has 5 values, age has 6, etc.)

### Trade-off: Debugging Difficulty

```cpp
// Hard to debug - what does "3" mean?
cout << popdata.status[42];  // Output: 3

// Need helper functions for readability
cout << Status::to_str(popdata.status[42]);  // Output: "recovered"
```

**Solution:** Develop print methods early (like `print_table()`) to make debugging easier.

## Evolution from Julia

### Julia Journey
1. **Started with `Int`** - Fast but not self-documenting
2. **Switched to enums** - Self-documenting but every enum is a different type
3. **Discovered Symbols** - Perfect! Compact, fast, readable

### C++ Journey
1. **Can't use Symbols** - No equivalent in C++
2. **Can't use enums** - Type system too rigid for generic code
3. **Use `uint8_t` + namespaces** - Compromise: fast and uniform, but need conversion functions

## Vector Types in PopData

### Simple Attributes (one value per person)
```cpp
vector<uint8_t> status;      // Current infection status
vector<uint8_t> agegrp;      // Age group (fixed)
vector<uint8_t> cond;        // Current condition severity
vector<uint8_t> duration;    // Days in current status
```

### History Attributes (multiple events per person)
```cpp
vector<array<uint8_t, 16>> variant;      // Up to 16 variant infections
vector<uint8_t> variant_count;           // How many infections so far

vector<array<uint8_t, 16>> vaxrcvd;      // Up to 16 vaccine doses
vector<uint8_t> vax_count;               // How many doses received
vector<array<uint8_t, 16>> vaxday;       // Day of each dose
```

**Pattern:** 
- `array<uint8_t, 16>` stores up to 16 events
- `uint8_t` count tracks how many slots are used
- Fixed size (16) avoids dynamic allocation during simulation

**Why 16?** 
- Reasonable upper bound (unlikely anyone gets 16+ infections or doses)
- Fixed size = no reallocation during simulation
- Small enough to fit in cache line

### Boolean Attributes
```cpp
vector<uint8_t> quar;        // Quarantine status (0 = false, 1 = true)
vector<uint8_t> tested;      // Test status (0 = false, 1 = true)
```

**Why not `vector<bool>`?**
- `vector<bool>` is specialized (bit-packed) and slow
- `vector<uint8_t>` is faster and consistent with other columns

## Constructor Pattern

```cpp
PopData(size_t n)
    : popn(n),
      status(n, 1),      // Initialize all to Status::unexposed (1)
      agegrp(n, 1),      // Initialize all to Agegrp::age0_19 (1)
      cond(n, 1),        // Initialize all to Condition::nil (1)
      duration(n, 0),    // Initialize all to 0
      variant(n),        // Default construct arrays
      variant_count(n),  // Initialize counts to 0
      // ... etc
{
    if (n <= 0) {
        throw std::invalid_argument("Bad size input. Must be a positive integer.");
    }
}
```

**Pattern:**
- Simple attributes: `vector(n, initial_value)`
- History arrays: `vector(n)` (default constructs empty arrays)
- Counts: `vector(n)` or `vector(n, 0)` (zero-initialized)

## Column Enum for Generic Operations

```cpp
enum class Column : uint8_t {
    status, agegrp, cond, duration,
    variant, variant_count,
    // ... all columns
};
```

**Purpose:** Enable generic operations on selected columns:

```cpp
// Print specific columns
popdata.print_table(rows, {Column::status, Column::agegrp, Column::cond});

// Apply function to multiple columns
popdata.apply_to_columns(cols, [](auto& vec) { /* process vec */ });
```

## Generic Column Processing

```cpp
template <typename Action>
void apply_to_columns(const vector<Column>& cols, Action&& action) {
    for (auto col : cols) {
        switch (col) {
            case Column::status:   action(status);   break;
            case Column::agegrp:   action(agegrp);   break;
            // ... all columns
        }
    }
}
```

**Use cases:**
- Printing selected columns
- Serialization (save/load specific columns)
- Statistics (compute stats on selected columns)
- Validation (check constraints on specific columns)

## Printing for Debugging

```cpp
struct CellPrinter {
    int current_row;
    
    void operator()(const vector<uint8_t>& vec) const {
        cout << "   " << (int)vec[current_row] << "\t|";
    }
    
    void operator()(const vector<array<uint8_t, 16>>& vec) {
        cout << "   " << (int)vec[current_row][0] << "...    | ";
    }
};

void print_table(const vector<int>& rows, const vector<Column>& cols);
```

**Why this matters:**
Since we're using `uint8_t` instead of self-documenting symbols, we need good printing to understand what's happening during development and debugging.

**Future enhancement:** Add string conversion to print methods:
```cpp
cout << Status::to_str(vec[current_row]);  // "infectious" instead of "2"
```

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|------------|-------|
| Access person i's attribute | O(1) | Direct array index |
| Update person i's attribute | O(1) | Direct array index |
| Iterate all people for one attribute | O(n) | Sequential, cache-friendly |
| Add/remove person | O(n) | Must resize all vectors |
| Find people with attribute X | O(n) | Linear scan (can optimize with indices) |

## Memory Layout

For population of 100,000:
```
status:   [1,1,2,1,3,...]  100KB (100k × 1 byte)
agegrp:   [1,2,2,3,1,...]  100KB
cond:     [1,1,2,1,1,...]  100KB
variant:  [[0,0,...], ...] 1.6MB (100k × 16 bytes)
...
Total: ~5-10 MB for 100k population
```

**Cache-friendly:** Related data is contiguous, fits in CPU cache during iteration.

## Future Enhancements

1. **String conversion in print methods** - Use `Status::to_str()` etc. for readable output
2. **Index structures** - Build indices for fast queries (e.g., all infected people)
3. **Columnar compression** - Run-length encoding for sparse columns
4. **SIMD operations** - Vectorize common operations (count infected, update statuses)
5. **Serialization** - Save/load PopData to disk for checkpointing

## References

- Struct of Arrays pattern: https://en.wikipedia.org/wiki/AoS_and_SoA
- Data-Oriented Design: https://www.dataorienteddesign.com/dodbook/
- Cache-friendly data structures: https://gameprogrammingpatterns.com/data-locality.html

