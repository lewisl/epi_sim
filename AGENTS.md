# Contributing to epi_sim

## Mandatory Session Start

Before proposing a plan or writing code, read in order:

1. `AGENTS.md` (this file)
2. `CONTRIBUTING.md`
3. `design/data-structures.md`
4. `design/zero and one based indexing.md`

If `SESSION.md` exists at the repo root, read it first — it is the source of truth for current task state, decisions, blockers, and next steps. Update it when finishing a unit of work or before context compaction.

## Build System

- **xmake only.** Never use cmake, make, or git worktrees.
- C++23, LLVM/Clang toolchain, `set_optimize("fastest")`.
- External libs via vcpkg with **manually configured paths** — a clean checkout will not build without that environment in place. Do not assume `xmake` "just works" on a fresh machine.
- Build file: `xmake.lua`. All targets are `set_default(false)`, so bare `xmake` builds nothing — always name a target. Relevant targets: `epi_sim` (main binary), `test` (test binary). Other targets (`this`, `randstuff`, `dates`, `ring_experiment`) are scratch utilities under `scratch/` — not part of the product.
- `xmake build <target>` builds that target. `xmake run <target>` rebuilds automatically if any of the target's source files are newer than the last build, then runs it.

```bash
xmake build epi_sim          # build the application
xmake run epi_sim [flags...] # build (if stale) and run with CLI flags
xmake run test               # build (if stale) and run tests
```

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

## Code Conventions

### PopData: 1-Based Indexing (critical)

All `PopData` member vectors have size `popz = popn + 1`. **Index 0 is unused and must never be accessed.** Valid person indices: `1..popn`. Use `pop.popn` for loop bounds, never `.size()`.

```cpp
for (int i = 1; i <= pop.popn; i++) { ... pop.status[i] ... }   // ✅
for (auto& s : pop.status) { ... }                              // ❌ touches index 0
std::accumulate(pop.status.begin() + 1, pop.status.end(), 0);   // ✅ skip element 0
```

`DayData` series are likewise 1-based across days: `for (int d = 1; d <= model.ndays; ++d)`. There is no day 0.

### MapEnum zero-value semantics

Zero means different things per enum — do not assume uniformity:
- `Condition`: 0 = "uninfected", not used to index parameter arrays.
- `Status`: 0 = "none", not used as an index.
- `Agegrp`: 0 = "unknown", should not be used as an index.
- `Variant`: 0 = "none" and is generally an error during simulation; **1 = "base" is required** and often derives other variant parameters.
- `Vaxstatus`: 0 = "none" is a valid status.
- `Progressionmap`: explicitly zero-based, valid nums `0..5`.

### PopData::AgentView

`AgentView` is a cheap proxy (reference to `PopData` + person index), not a row copy. Pass by value normally; do not switch to `const AgentView&` for performance. `const AgentView` makes the proxy const but does **not** make the underlying person data const — a `const AgentView` can still mutate person state through mutable accessors. Use `const AgentView` only to prevent rebinding the proxy itself.

### Formatting & output

Use the `{fmt}` library exclusively for new code: `fmt::print`, `fmt::print(stderr, ...)`, `fmt::format`, custom `fmt::formatter<T>`. Use `{}`-style format strings. Do not introduce new `printf`/`fprintf`/`snprintf` or `std::ostringstream` formatting.

### Naming

- Structs: PascalCase (`ModelParams`, `PopData`)
- Functions/variables: snake_case (`setup_sim`, `vax_path`)
- Constants: UPPER_SNAKE_CASE (`AGE_DIST`, `DURATIONLIM`)

### File organization

`src/` (`.h` + `.cpp`), `test/`, `sample_parameters/` (JSON/CSV params), `docs/`, `design/` (dev-focused explanations), `scratch/` (throwaway utilities, gitignored).

## Hot Paths

Treat `spread`, `progression`, `runsim`, vaccination, and population loops as hot paths. Do not add helper calls, allocations, extra bounds checks, or abstractions inside hot loops unless requested or justified by a measured bug/risk. Prefer existing inline/local logic.

## Workflow

- **Intent & scope:** If asked to inspect/review/explain, do not edit unless explicitly asked. Do not add features, refactor, or "improve" beyond the request. Prefer small targeted patches. Ask before broad changes.
- **Before editing:** Read every file you intend to modify. Trace callers, related symbols, custom argument types, and where data originates. Verify overloaded call-site binding by hand. State invariants before broad changes.
- **Code management:** Don't delete code unless requested or clearly part of another requested change. Preserve unrelated dirty worktree changes.
- **Communication:** Concise, lead with the result. For code changes report only: what changed, where, tests run, remaining risks.

## Code Navigation

Serena MCP (clangd/LSP) provides semantic navigation for C++23. For functions, methods, classes, structs, enums, aliases, variables: prefer Serena go-to-definition / find-references / diagnostics first (resolves overloads correctly). Use ripgrep for non-symbol text: comments, string literals, build files, JSON, Markdown, config, logs.

For C++ semantic questions ("where is this used?", overload binding, call-site analysis, references, declarations, implementations, diagnostics), perform a complete Serena pass before answering: start from the named symbol, then expand to the owning type and relevant functions/methods discovered from references. Do not answer from a single direct-reference query when C++ implicit construction, overload resolution, forwarding wrappers, or by-value parameters may affect the result.

## What Not To Do

- Do not change code before reading the relevant source.
- Do not use git worktrees — vcpkg paths and LLVM/Clang stdlib links cannot be reconstructed in a worktree without manual setup.
- Do not guess xmake syntax — read `xmake.lua` and acknowledge uncertainty.
- Do not delete code unless explicitly requested.
- Do not add features/refactors beyond what was asked.
