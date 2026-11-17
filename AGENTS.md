# Contributing to epi_sim

## Mandatory Session Start

Before doing anything else — before proposing a plan, before reading any specific file requested, before writing a single line of code — do the following in order:

1. On startup or after context compaction, always read `SESSION.md` first if it exists at the repo root. It is the source of truth for current task state, decisions, blockers, and next steps. Update it when finishing a unit of work or before context compaction. 
2. Read `CONTRIBUTING.md`
3. Read `design/data-structures.md`
4. Read `design/zero and one based indexing.md`


Only after completing these steps should you engage with the user's task.




### Communication

- Concise: Lead with the answer or result.
- Avoid long explanations unless asked for detail.
- For code changes, report only: what changed, where, tests run, and remaining risks.
- Make sure the response is very crisp. Always support markdown, table. Limit the output tokens as much as possible, unless ask to elaborate more.

### Intent And Scope

- If the user asks to inspect, evaluate, review, or explain, do not edit files unless explicitly asked.
- Do not add features, refactor, or improve beyond the request.
- Preserve unrelated dirty worktree changes.
- Prefer small, targeted patches over structural rewrites.
- Ask before broad changes.
- When in doubt, ask before changing files. 
- Don't delete code unless requested or clearly part of another requested change. Preserve unrelated dirty worktree changes.


## Before Making Any Change

- Read every file you intend to modify. No exceptions.
- Identify whether vectors involved use 1-based indexing (most PopData vectors do — see AGENTS.md).
- State your understanding of the relevant invariants before proposing code.
- Do not change code before reading the relevant source. Trace callers, related symbols, custom argument types, and where data originates. Verify overloaded call-site binding by hand. State invariants before broad changes.
- Do not use git worktrees. This project's build environment (vcpkg paths, LLVM/Clang std library links) cannot be reconstructed in a worktree without manual intervention. Worktrees are a dead end here.
- Do not guess xmake syntax — read `xmake.lua` and acknowledge uncertainty.
- Do not delete code unless explicitly requested.
- Do not add features/refactors beyond what was asked. You may propose, but do not implement without explicit approval.

  
## Code Navigation
- For semantic C++ questions (callers, type uses, signature-change impact), prefer clangd `findReferences` / call hierarchy — it resolves overloads correctly. Use grep for text matches (comments, strings, non-C++ files, commented-out code) or when clangd hasn't finished indexing.
 - For C++ semantic questions ("where is this used?", overload binding, call-site analysis, references, declarations, implementations, diagnostics), perform a complete Serena or LSP pass before answering: start from the named symbol, then expand to the owning type and relevant functions/methods discovered from references. Do not answer from a single direct-reference query when C++ implicit construction, overload resolution, forwarding wrappers, or by-value parameters may affect the result.
- clangd-lsp plugin is installed. It provides go-to-definition, 
  find-references, and diagnostics via clangd with full semantic 
  understanding of C++23, templates, and overloads. If you have a plugin or built-in LSP navigation, prefer it to Serena.
- Serena MCP (clangd/LSP) is an alternative if there is no LSP plugin or built-in tool: provides semantic navigation for C++23. For functions, methods, classes, structs, enums, aliases, variables: prefer Serena go-to-definition / find-references / diagnostics first (resolves overloads correctly). Use ripgrep for non-symbol text: comments, string literals, build files, JSON, Markdown, config, logs.


## Session State Management

- Treat `SESSION.md` as the source of truth for:
  - Current task and progress
  - Key decisions made this session
  - Open questions or blockers
  - Files modified and why
  - Next concrete steps
- When you finish a meaningful unit of work or are about to lose context, especially before compaction, update `SESSION.md` with succinct bullet points. Write it as if briefing
  a replacement who knows the codebase but not what you did today.