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

### PopData::AgentView Arguments

`PopData::AgentView` is a small proxy/handle, not a materialized row copy of `PopData`.

- An `AgentView` contains a reference to the underlying `PopData` plus the person's index
- Passing `PopData::AgentView` by value is normally appropriate and cheap
- Passing `PopData::AgentView` by value does **not** copy the population vectors or person state
- Do not switch `AgentView` parameters to `const PopData::AgentView&` for performance reasons; that usually adds indirection without benefit

Constness needs care here:

- `const PopData::AgentView` makes the proxy object itself const inside the callee
- It does **not** automatically make the underlying `PopData` entry const
- If `AgentView` exposes const-qualified accessors returning mutable references, a function can still update the underlying person data through a `const AgentView`
- Use `const PopData::AgentView` only when you specifically want to prevent rebinding or mutation of the proxy object inside the function; do **not** assume it means read-only access to the person's data

Practical guidance:

- Use `PopData::AgentView person` for ordinary agent-oriented helper functions
- Use `const PopData::AgentView person` only if the API is intentionally designed so the proxy itself should not be changed in the callee
- If a function must be truly read-only with respect to person state, the `AgentView` API itself must provide read-only accessors or a separate const view type


### File Organization
- Header files (`.h`) in `src/`
- Implementation files (`.cpp`) in `src/`
- Tests in `test/`
- Parameter files in `sample_parameters/`
- Documentation in `docs/`
- Design documents and development focused explanations in `design/`

### Naming Conventions
- Structs: PascalCase (e.g., `ModelParams`, `PopData`)
- Functions: snake_case (e.g., `setup_sim`, `load_vax_data`)
- Variables: snake_case (e.g., `popn`, `vax_path`)
- Constants: UPPER_SNAKE_CASE (e.g., `AGE_DIST`, `DURATIONLIM`)

### Build System
- Using xmake for build configuration
- C++23 standard
- LLVM/Clang toolchain

## Other requests

### Code management
- Don't delete code unless requested by the user or clearly part of another requested code change 

## Codex Workflow Instructions

### Intent And Scope

- If the user asks to inspect, evaluate, review, or explain, do not edit files unless explicitly asked.
- Do not add features, refactor, or improve beyond the request.
- Preserve unrelated dirty worktree changes.
- Prefer small, targeted patches over structural rewrites.
- When in doubt, ask before changing files.

### Before Editing

- Read the relevant source files before editing.
- For non-trivial changes, trace callers, custom argument types, and where key data originates.
- State important assumptions or invariants before broad changes.

### Build And Test

- Use `xmake` only. Do not use cmake, make, or git worktrees.
- Run the narrowest relevant `xmake run test <group>` first when available.
- Run full `xmake run test` after changes to shared headers, core simulation behavior, or test harness code.
- Do not add new test groups or test harness structure unless requested.

### Hot Paths

- Treat `spread`, `progression`, `runsim`, vaccination, and population loops as hot paths.
- Do not add helper calls, allocations, extra bounds checks, or abstractions inside hot loops unless requested or justified by a measured bug/risk.
- Prefer existing inline/local logic in hot paths unless profiling or correctness requires otherwise.

### Communication

- Keep responses concise and directly tied to the request.
- Lead with the answer or result.
- Avoid long explanations unless asked for detail.
- For code changes, report only: what changed, where, tests run, and remaining risks.

## Session State Management

- On startup or after context compaction, always read `SESSION.md` first.
- Treat `SESSION.md` as the source of truth for:
  - Current task and progress
  - Key decisions made this session
  - Open questions or blockers
  - Files modified and why
  - Next concrete steps
- When you finish a meaningful unit of work or are about to lose context,
  update `SESSION.md` with succinct bullet points. Write it as if briefing
  a replacement who knows the codebase but not what you did today.