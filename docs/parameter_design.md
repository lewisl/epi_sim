# C++ Parameter Design Patterns

## Overview

This document captures the design patterns and principles used for parameter management in the epi_sim project. These patterns emerged from converting Julia code (which uses named tuples and dicts) to performant C++ code.

## Core Principles

### 1. One-Pass Loading
Each `load_X_params()` function loads raw data (JSON/CSV) and transforms it to typed structs in a single pass. No intermediate storage of raw JSON.

```cpp
// ✅ Good: One pass
VariantParams load_variant_params(string path) {
    json data = load_json_params(path);
    VariantParams vp;
    // Transform JSON → struct directly
    return vp;
}

// ❌ Avoid: Two-pass (Julia pattern doesn't translate well to C++)
json raw = load_json_params(path);
VariantParams vp = transform_json(raw);
```

### 2. Aggregate Initialization
Use C++20 designated initializers to construct structs without assignment. This is critical when structs have `const` members.

```cpp
// Aggregate initialization with designated initializers
model_params params{
    .geodata = load_geodata_csv(geo_path),
    .socialdata = load_social_params(social_path),
    .variantdata = load_variant_params(variants_path)
};

// Why this matters:
// - Constructs each member in place (no assignment)
// - Works with const members
// - Clear and readable
// - Enables move semantics
```

**Key distinction:**
```cpp
model_params params;              // Calls default constructor
params.x = load_x();              // Assignment (fails if x has const members)

model_params params{.x = load_x()};  // Aggregate init (constructs in place, works!)
```

### 3. Avoid `map` in Hot Paths
Use vector indices for simulation loop access (O(1)). Reserve `map` only if absolutely necessary.

```cpp
// ✅ Simulation loop (millions of iterations)
float trans = variants[variant_idx].transmissibility;  // O(1) array access

// ❌ Avoid in hot paths
float trans = variant_map["Delta"].transmissibility;   // O(log n) or O(1) but slower
```

### 4. Names for Output Only
String lookups are only needed for printing and plotting (not performance-critical). Use simple `vector<string>` for labels.

```cpp
struct VariantParams {
    // Fast numeric data for simulation
    vector<VariantDef> variants;
    
    // Simple labels for output (O(n) lookup is fine)
    vector<string> variant_names;
    
    // Optional helper for occasional name lookup
    size_t find_index(const string& name) const {
        auto it = std::find(variant_names.begin(), variant_names.end(), name);
        return std::distance(variant_names.begin(), it);
    }
};
```

**Performance profile:**
- **Setup** (once): Name lookups OK
- **Simulation** (millions of iterations): Index-based access only, no strings
- **Output** (occasionally): Name lookups for printing/plotting

### 5. Const Metadata When Fixed
Use `const` members for labels that never change during simulation. Initialize in constructor.

```cpp
struct SocialParams {
    // Mutable simulation data
    float gammashape {};
    array<array<float, 5>, 4> contactfactors {};
    
    // Immutable metadata (structure definition)
    const vector<string> touch_rows;
    const vector<string> contact_rows;
    
    // Constructor initializes const members
    SocialParams() 
        : touch_rows{"unexposed", "recovered", "nil", "mild", "sick", "severe"},
          contact_rows{"nil", "mild", "sick", "severe"}
    {}
};
```

**Trade-off:** Const members prevent copy assignment but enable aggregate initialization.

### 6. Move Semantics for Large Structs
Use `std::move()` when building larger structs to avoid expensive copies.

```cpp
Model build_model(string param_dir) {
    model_params params = load_model_params(param_dir);
    
    return Model{
        .params = std::move(params),  // Move, not copy!
        .population = initialize_population(params),
        .state = initialize_state(params)
    };
}
```

## Standard Pattern for Parameter Structs

```cpp
struct XParams {
    // Mutable simulation data (accessed by index in hot loop)
    vector<DataType> data;
    array<array<float, N>, M> matrix;
    float scalar_param {};
    
    // Labels for output (simple vectors)
    vector<string> row_names;
    vector<string> col_names;
    
    // Constructor if labels are fixed/known
    XParams() : row_names{...}, col_names{...} {}
};

XParams load_x_params(string path) {
    json data = load_json_params(path);
    
    XParams xp;
    xp.scalar_param = data["scalar"];
    
    // Load vector data
    for (auto& item : data["items"]) {
        xp.data.push_back({...});
        xp.row_names.push_back(item["name"]);
    }
    
    // Load matrix data
    for (size_t i = 0; i < xp.row_names.size(); ++i) {
        for (size_t j = 0; j < xp.col_names.size(); ++j) {
            xp.matrix[i][j] = data["matrix"][xp.col_names[j]][xp.row_names[i]];
        }
    }
    
    return xp;
}
```

## Completed Examples

### GeoData (CSV → Struct)
- Simple tabular data
- Each column → typed vector
- Clean 1-to-1 mapping

### SocialParams (JSON → Struct)
- Scalars: `gammashape`, `indoor_uplift`
- Matrices: `contactfactors` (4×5), `touchfactors` (6×5)
- Const labels: `touch_rows`, `contact_rows`, `age_columns`
- Demonstrates const metadata pattern

## Next: Complex 1-to-Many Mappings

Future parameter types will involve one JSON file populating multiple structs or complex nested structures:
- **Variants**: Multiple variant definitions, cross-immunity matrices, time-series prevalence
- **Vaccines**: Multiple vaccine types, efficacy by variant/dose, waning curves

Apply the same patterns but with more complex struct hierarchies.

## C++ Initialization Gotchas

### The Many Ways to Initialize (Historical Mess)
C++ has too many initialization syntaxes due to backward compatibility:
- C-style: `int x = 5;`
- Constructor: `MyClass obj(a, b);`
- Uniform init: `MyClass obj{a, b};`
- Aggregate init: `MyStruct s = {1, 2, 3};`
- **Designated init (C++20)**: `MyStruct s{.x = 1, .y = 2};` ← **Use this!**

### Aggregate Initialization vs Constructor Call
```cpp
MyStruct s;      // Calls default constructor (even if compiler-generated)
MyStruct s{};    // Aggregate initialization (no constructor call!)
```

These look similar but do completely different things! Aggregate init constructs members directly from the initializer list.

### Destruction is Always the Same
Regardless of how you construct (constructor vs aggregate init), destruction is identical:
- Compiler-generated destructor calls member destructors in reverse order of declaration
- Works even with const members (destruction doesn't assign, just releases resources)

## Migration from Julia

### Julia Pattern (2-pass)
```julia
raw_data = (geodata = load_csv(...), variants = load_json(...))
params = (geodata = build_struct(raw_data.geodata), ...)
model = (params = params, population = init_pop(params), ...)
```

### C++ Pattern (1-pass, cleaner)
```cpp
model_params params{
    .geodata = load_geodata_csv(path),      // Load + transform in one
    .variantdata = load_variant_params(path)
};

Model model{
    .params = std::move(params),
    .population = initialize_population(params)
};
```

**Advantages in C++:**
- One pass (load and transform together)
- No intermediate JSON storage
- Type safety enforced by compiler
- Move semantics avoid copies
- Aggregate initialization handles const members

## References

- C++20 Designated Initializers: https://en.cppreference.com/w/cpp/language/aggregate_initialization
- Move Semantics: https://en.cppreference.com/w/cpp/utility/move
- RAII Pattern: https://en.cppreference.com/w/cpp/language/raii
