# Contributing to epi_sim

## Code Conventions

### PopData: 1-Based Indexing

**Important:** All `PopData` vectors use **1-based indexing** to match mathematical/epidemiological conventions where persons are numbered 1 to N.

#### Vector Structure
- All PopData member vectors have size `popz = popn + 1`
- Index 0 is **unused** and should never be accessed
- Valid person indices: `1` to `popn` (inclusive)
- Example: For a population of 100, vectors have size 101, with valid indices 1-100

#### Looping Conventions

**Preferred: Index-based loops**
```cpp
// ✅ CORRECT - Index-based loop
for (int i = 1; i <= pop.popn; i++) {
    pop.status[i] = new_status;
    pop.agegrp[i] = age_group;
}

// ❌ WRONG - Don't start at 0
for (int i = 0; i < pop.popn; i++) {
    pop.status[i] = new_status;  // Accesses index 0!
}
```

**Iterator-based loops (when necessary)**
```cpp
// ✅ CORRECT - Skip first element
for (auto it = pop.status.begin() + 1; it != pop.status.end(); ++it) {
    *it = new_status;
}

// ❌ WRONG - Don't use begin() directly
for (auto it = pop.status.begin(); it != pop.status.end(); ++it) {
    *it = new_status;  // Processes index 0!
}
```

**Standard library algorithms**
```cpp
// ✅ CORRECT - Start at begin() + 1
int total = std::accumulate(pop.status.begin() + 1, pop.status.end(), 0);
auto it = std::find(pop.agegrp.begin() + 1, pop.agegrp.end(), target_age);

// ❌ WRONG - Don't use begin() directly
int total = std::accumulate(pop.status.begin(), pop.status.end(), 0);
```

#### Why 1-Based Indexing?

1. **Domain alignment**: Epidemiological models traditionally number persons 1 to N
2. **Mathematical clarity**: Matches equations and pseudocode from papers
3. **Debugging**: Person IDs match array indices directly
4. **Interoperability**: Easier to work with R, Julia, MATLAB code that uses 1-based indexing

#### Common Pitfalls

```cpp
// ❌ WRONG - Range-based for loop processes ALL elements including index 0
for (auto& status : pop.status) {
    status = new_value;  // Modifies index 0!
}

// ❌ WRONG - Using .size() instead of .popn
for (int i = 0; i < pop.status.size(); i++) {
    // Processes 0 to popz-1, should be 1 to popn
}

// ✅ CORRECT - Use popn, not size()
for (int i = 1; i <= pop.popn; i++) {
    pop.status[i] = new_value;
}
```

#### Testing and Validation

When writing tests or debug output:
- Always verify you're accessing indices 1 to popn
- Check that index 0 remains unchanged (should stay at initialization value)
- Use `pop.popn` for loop bounds, not `pop.status.size()`

#### Example: Counting by Status

```cpp
// Count people by infection status
std::vector<int> status_counts(5, 0);  // 5 status types

// ✅ CORRECT
for (int i = 1; i <= pop.popn; i++) {
    status_counts[pop.status[i]]++;
}

// Alternative with iterators
for (auto it = pop.status.begin() + 1; it != pop.status.end(); ++it) {
    status_counts[*it]++;
}
```

## Other Conventions

(Add other coding conventions here as the project develops)

### File Organization
- Header files (`.h`) in `src/`
- Implementation files (`.cpp`) in `src/`
- Tests in `test/`
- Parameter files in `sample_parameters/`
- Documentation in `docs/`

### Naming Conventions
- Structs: PascalCase (e.g., `ModelParams`, `PopData`)
- Functions: snake_case (e.g., `setup_sim`, `load_vax_data`)
- Variables: snake_case (e.g., `popn`, `vax_path`)
- Constants: UPPER_SNAKE_CASE (e.g., `AGE_DIST`, `DURATIONLIM`)

### Build System
- Using xmake for build configuration
- C++23 standard
- LLVM/Clang toolchain

