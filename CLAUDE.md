# Claude Code Instructions for epi_sim

## Mandatory Session Start

Before doing anything else — before proposing a plan, before reading any specific file requested, before writing a single line of code — do the following in order:

1. Read `AGENTS.md` (project conventions: indexing, AgentView, naming, build system)
2. Read `CONTRIBUTING.md`
3. Read `design/data-structures.md`
4. Read `design/zero and one based indexing.md`
5. Use `codegraph_status` to confirm the index is current, then `codegraph_context` / `codegraph_files` to establish what exists in `src/` — do not `ls`/grep `src/` for this.

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
- To find what calls a function, what uses a type, or what a signature change would break, use clangd `findReferences` / call-hierarchy — it resolves overloads correctly. Use `codegraph_context` for fast broad orientation. Fall back to grep/Read only when both return nothing. (See "Code navigation tooling" below.)
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
