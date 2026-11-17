
### Why 1-Based Indexing?

1. **Domain alignment**: Epidemiological models traditionally number persons 1 to N
2. **Mathematical clarity**: Matches equations and pseudocode from papers
3. **Debugging**: Person IDs match array indices directly
4. **Interoperability**: Easier to work with R, Julia, MATLAB code that uses 1-based indexing

### one-based indexed arrays and index values
- MapEnum Condition
  - 0 is "uninfected" and not used to access parameter arrays
- MapEnum Status
  - 0 is "none"; not used as an index
- MapEnum Agegrp
  - 0 is "unknown"; should not be used as an index
- MapEnum Variant
  - 0 is "none"; not used as an index and generally is an error if it occurs during simulation
  - 1 should always be "base"; it is required and corresponds to a real variant; it is often used to derive other variant parameters
- MapEnum Vaxstatus
  - 0 is "none", which is a valid status
  - indexes the rows of `Vaxparam::effectiveness[vaxstatus][variant]`; row 0 ("none") is never read because `vaxeffect` returns 1.0 early for "none"; row "exhausted" is all zeros (effectiveness 0 by definition, never supplied by users)
- Compile-time scoped enum Progressionmap (uint8_t underlying value)
  - specifically meant to be used as a zero-based index in the progression functions
  - valid values are in 0..5; use std::to_underlying at numeric index boundaries

#### PopData
- all data vectors have size population count + 1
- popn is the actual count of the population
- popz is the size (length) of the vectors
- when looping through people use indices 1..popn. popz is the physical size of the vectors = popn + 1

#### DayData series
- `DayData` series are likewise 1-based across days: `for (int d = 1; d <= model.ndays; ++d)`. There is no day 0.
- all vectors in DayData accumulate statistics during the simulation per day. these vectors are +1 larger than the number of days to be held. indexing across days should be in 1..model.ndays. for an example here is the start of the day loop in function runsim:
        ```cpp
        // day loop
        for (int d_i = 1; d_i <= model.ndays; ++d_i) { <loop body follows...>
        ```
- 0 is not used; there is no day zero.  day 1 is the first day of the simulation.  of course,c++ vectors and arrays are zero-indexed so care must be taken when accessing day indexed elements and looping across days.



### PopData: 1-Based Indexing (critical)

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

// ✅ CORRECT - Use popn, not size()
for (int i = 1; i <= pop.popn; i++) {
    pop.status[i] = new_value;
}
```

```cpp
for (int i = 1; i <= pop.popn; i++) { ... pop.status[i] ... }   // ✅
for (auto& s : pop.status) { ... }                              // ❌ touches index 0
std::accumulate(pop.status.begin() + 1, pop.status.end(), 0);   // ✅ skip element 0
```
**Standard library algorithms**
```cpp
// ✅ CORRECT - Start at begin() + 1
int total = std::accumulate(pop.status.begin() + 1, pop.status.end(), 0);
auto it = std::find(pop.agegrp.begin() + 1, pop.agegrp.end(), target_age);

// ❌ WRONG - Don't use begin() directly
int total = std::accumulate(pop.status.begin(), pop.status.end(), 0);
```

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