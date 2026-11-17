# Contributing to epi_sim

Thank you for your interest in contributing to epi_sim!

## Getting Started

### Prerequisites
- C++23 compatible compiler (LLVM/Clang recommended)
- xmake build system
- vcpkg for package management

### Setting Up Development Environment

1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd epi_sim
   ```

2. **Install dependencies:**
Before building, install and configure LLVM/Clang and vcpkg. Make the vcpkg installation discoverable through VCPKG_ROOT or xmake config --vcpkg=/path/to/vcpkg. Dependencies are declared in xmake.lua; xmake’s vcpkg integration resolves their include and library paths. See also: design/configuring llvm latest clang.md:28.

   ```bash
   xmake config --vcpkg
   ```

3. **Run tests:**
   ```bash
   xmake run test
   ```

## Development Workflow

## Build System

- **xmake only.** Never use cmake, make, or git worktrees.
- C++23, LLVM/Clang toolchain, `set_optimize("fastest")`.
- External libs via vcpkg with **manually configured paths** — a clean checkout will not build without that environment in place. Do not assume `xmake` "just works" on a fresh machine.
- Build file: `xmake.lua`. All targets are `set_default(false)`, so bare `xmake` builds nothing — always name a target. Relevant targets: `epi_sim` (main binary), `test` (test binary). Other targets (`this`, `randstuff`, `dates`, `ring_experiment`) are scratch utilities under `scratch/` — not part of the product.
- `xmake build <target>` builds that target. `xmake run <target>` rebuilds automatically if any of the target's source files are newer than the last build, then runs it.
- Run the narrowest relevant `xmake run test <group>` first when available.
- Run full `xmake run test` after changes to shared headers, core simulation behavior, or test harness code.
- Do not add new test groups or test harness structure unless requested.
- `xmake run <target>` and `xmake build <target>` must be run from the project root — xmake resolves paths relative to `xmake.lua`'s location.
- If a build/test failure can't be explained by the current source (e.g. after pulling changes, syncing across machines, or switching branches), the xmake build cache may be stale and mismatched with the checked-out files. Force a full rebuild before trusting the failure: `xmake build -r <target>` (e.g. `xmake build -r test`). Do this before deep-diving into a "mysterious" crash or test failure.


```bash
xmake build epi_sim          # build the application
xmake run epi_sim [flags...] # build (if stale) and run with CLI flags
xmake run test               # build (if stale) and run tests
```

### Building the Project

```bash

# Build specific target
xmake build test
xmake build epi_sim

# Clean build
xmake clean
xmake build -r <target> # -r rebuilds
```

## Code Conventions

**Important:** This project uses specific coding conventions that are critical for correctness.

👉 **See [AGENTS.md](AGENTS.md) for detailed technical conventions**, including:
- PopData 1-based indexing (critical!)
- Loop patterns and common pitfalls
- Data structure invariants
- Naming conventions

All contributors (human and AI) should read `AGENTS.md` before making changes to the codebase.

## Project Structure

```
epi_sim/
├── src/              # Source files (.h and .cpp)
├── test/             # Test files
├── sample_parameters/  # Parameter files (JSON, CSV)
├── docs/             # Documentation
├── xmake.lua         # Build configuration
└── AGENTS.md         # Technical conventions (READ THIS!)
```

## Making Changes

1. **Read `AGENTS.md`** - Understand the critical conventions
2. **Make your changes** - Follow the conventions in AGENTS.md
3. **Test your changes** - Run the test suite
4. **Document if needed** - Update docs/ if adding features


### Naming Conventions

- Structs: PascalCase (`ModelParams`, `PopData`)
- Functions/variables: snake_case (`setup_sim`, `vax_path`)
- Constants: UPPER_SNAKE_CASE (`AGE_DIST`, `DURATIONLIM`)

### File organization

- Header files (`.h`) in `src/`
- Implementation files (`.cpp`) in `src/`
- Tests in `test/`
- Parameter files in `sample_parameters/`
- Documentation in `docs/`
- Design documents and development focused explanations in `design/`

### Hot Paths

- Treat `spread`, `progression`, `runsim`, vaccination, and population loops as hot paths. PERFORMANCE MATTERS HERE.
- Do not add helper calls, allocations, extra bounds checks, or abstractions inside hot loops unless requested or justified by a measured bug/risk.
- Prefer existing inline/local logic in hot paths unless profiling or correctness requires otherwise.

### C++ Formatting and output

This project uses the `{fmt}` library.

Prefer:
- `fmt::print(...)` for formatted output
- `fmt::print(stderr, ...)` for diagnostics/errors
- `fmt::format(...)` for constructing formatted strings
- custom `fmt::formatter<T>` specializations for project types when useful

Avoid:
- `std::fprintf`, `printf`, `snprintf` except for C interop or existing low-level code
- `std::ostringstream` for routine string formatting
- iostream formatting chains for generated output unless very, very simple
- Do not introduce new `printf`/`fprintf`/`snprintf` formatting unless there is a specific C API boundary reason. 

Use `{}`-style fmt format strings, not printf-style `%` format strings.

## Tests

Test binary is `test`; groups are selected by positional arg.

```bash
xmake run test                       # runs ALL groups EXCEPT runsim
xmake run test <group>               # one group
xmake run test <group> --artifacts   # write CSV/plot artifacts to test_output/
xmake run test runsim                # MUST be run explicitly (see below)
```

Groups: `pop_serialize`, `parameters`, `disease_modeling`, `vaccination`, `traits`, `series`, `setup`, `plot`, `runsim`.

**`runsim` is intentionally excluded from the no-arg sweep.** It is slow, writes CSV/plot artifacts to disk, and opens browser tabs. Always run it explicitly. (Note: `design/running_tests.md` is stale and does not list `runsim`.)

- Run the narrowest relevant group first; run the full `xmake run test` after changes to shared headers, core simulation, or test harness.
- Do not add new test groups or harness structure unless requested.
- Fixtures live in `test/fixtures/`; artifacts go to `test_output/` (gitignored).

#### Testing and Validation

When writing tests or debug output:
- Always verify you're accessing indices 1 to popn
- Check that index 0 remains unchanged (should stay at initialization value)
- Use `pop.popn` for loop bounds, not `pop.status.size()`

#### Running Tests

```bash
# Run the test suite
xmake run test

# Run the main simulation
xmake run epi_sim
```


## Questions?

- Check `AGENTS.md` for technical conventions
- Check `design/` for design documentation
- Open an issue for questions or discussions

## License

[Add license information here]
