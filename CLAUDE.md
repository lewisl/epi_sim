# Claude Code Instructions for epi_sim

## Mandatory Session Start

Before doing anything else — before proposing a plan, before reading any specific file requested, before writing a single line of code — do the following in order:

1. Read `AGENTS.md` (project conventions: indexing, AgentView, naming, build system)
2. Read `CONTRIBUTING.md`
3. Read `design/data-structures.md`
4. Read `design/zero and one based indexing.md`
5. Scan `src/` and list the files to establish what exists

Only after completing these steps should you engage with the user's task.

## Build System

- **xmake only.** Never use cmake, make, or any other build system.
- Build file is `xmake.lua` in the project root.
- C++23, LLVM/Clang toolchain.
- External libraries are installed via vcpkg with manually configured paths — do not assume a clean build will work without that environment in place.
- Run the narrowest relevant `xmake run test <group>` first when available.
- Run full `xmake run test` after changes to shared headers, core simulation behavior, or test harness code.
- Do not add new test groups or test harness structure unless requested.

## Before Making Any Change

- Read every file you intend to modify. No exceptions.
- Trace call chains for any function you are changing: find callers, find what custom types the arguments are, find where data originates.
- Identify whether vectors involved use 1-based indexing (most PopData vectors do — see AGENTS.md).
- State your understanding of the relevant invariants before proposing code.

## What Not To Do

- Do not make changes before reading the relevant code.
- Do not use git worktrees. This project's build environment (vcpkg paths, LLVM/Clang std library links) cannot be reconstructed in a worktree without manual intervention. Worktrees are a dead end here.
- Do not delete code unless explicitly requested.
- Do not add features, refactor, or "improve" beyond what was asked.
- Do not guess at xmake syntax — read `xmake.lua` and acknowledge uncertainty rather than bluffing.

### Intent And Scope

- If the user asks to inspect, evaluate, review, or explain, do not edit files unless explicitly asked.
- Do not add features, refactor, or improve beyond the request.
- Preserve unrelated dirty worktree changes.
- Prefer small, targeted patches over structural rewrites.
- When in doubt, ask before changing files.

### Hot Paths

- Treat `spread`, `progression`, `runsim`, vaccination, and population loops as hot paths.
- Do not add helper calls, allocations, extra bounds checks, or abstractions inside hot loops unless requested or justified by a measured bug/risk.
- Prefer existing inline/local logic in hot paths unless profiling or correctness requires otherwise.

### Communication

- Keep responses concise and directly tied to the request.
- Lead with the answer or result.
- Avoid long explanations unless asked for detail.
- For code changes, report only: what changed, where, tests run, and remaining risks.