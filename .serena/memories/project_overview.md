# epi_sim project overview

- C++23 epidemiological simulation using xmake, LLVM/Clang, and vcpkg packages: `p-ranav-csv2`, `nlohmann-json`, `fmt`, `abseil`.
- Main directories: `src/` for implementation and headers, `test/` for grouped tests, `sample_parameters/` for JSON/YAML/CSV inputs, `docs/` and `design/` for architecture notes.
- Core runtime path: `src/epi_sim.cpp` parses config/seed inputs, `src/setup.cpp` builds `ModelParams`, calendar days, indoor sequence, and `PopData`, then `src/sim.cpp::runsim` runs a day loop.
- `PopData` in `src/population.h` is a 1-based struct-of-arrays population. All person vectors have size `popn + 1`; valid person indices are `1..popn`; index 0 is unused. Use `pop.popn`, not vector `.size()`, for person loops.
- `PopData::AgentView` is a cheap proxy containing a `PopData&` plus index. Passing it by value is normal and still mutates underlying `PopData` through accessors.
- Main modules: `traits.h` typed wrappers and history buffers; `parameters.*` loaders and parameter containers; `cases.*` seed/social-distancing scenario logic; `disease_modeling.*`, `spread.*`, `progression.*`, `vaccination.*` model kernels; `series.*` time series; `pop_serialize.*` population output; `plot.*` Plotly HTML output.
- `AllSeries` day arrays are also effectively 1-based for simulation days, with day 0 generally unused and day 1 seeded from initial population state.
- Current worktree observed during onboarding had existing user changes in tests plus `.serena/`; do not revert unrelated edits.
- The earlier convention risk in `src/sim.cpp::print_summary` was fixed: the remaining-infected `std::count_if` now starts at `pop.status.begin() + 1`, so index 0 is skipped.