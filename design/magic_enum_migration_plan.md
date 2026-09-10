# Plan: migrate fixed trait metadata to magic_enum

Status: implemented and validated on 2026-09-10. The approved plan below was
prepared from the working tree, Serena/clangd navigation, and the supplied
discussion.

## Completion results

- xmake resolved magic_enum v0.9.8; the user's package declarations were
  sufficient and were not changed. Refreshed the local compilation database
  with `xmake project -k compile_commands --lsp=clangd`.
- Fixed metadata now comes from the seven scoped enums described below.
  Population wrappers still have one-byte storage and their existing numeric
  interfaces. Runtime registries and atomic-history update arithmetic remain
  unchanged.
- Reconciled selection-migration leftovers: the all-selection builder, test
  literals and reverse-layout test, ring-disabled introspection, and phase
  presentation. Fixed the missing comma in the vaccinated cumulative plot's
  death selection; added a regression check for that trace.
- Application build passed. Focused groups passed, followed by all 1,197
  checks in `xmake run test` and all 35 in `xmake run test runsim --artifacts`.
  The reconciled baseline passed 1,037 and 34 checks, respectively.
- All three CSV outputs (population, selected histories, comprehensive
  histories) matched the baseline byte-for-byte. Comprehensive output has 180
  day rows and 486 columns. Every existing plot trace matched exactly; the
  corrected cumulative plot additionally contains `now_dead:total`.
- Optimized application ThinLTO disassembly has identical instruction
  sequences for `spread` (767 instructions) and `progression` (428).
  `vaxeffect` retains its existing diagnostic string construction; reflection
  changes that function from 328 to 333 instructions. Single-run kernel timings
  were similar (baseline/new progression 31.06/31.59 ms, spread 132.38/134.99 ms);
  these are end-to-end observations, not a controlled microbenchmark.
- Serena reports no errors in changed C++ files. Diff checking found only
  pre-existing trailing whitespace in user-modified files, left untouched.
- Aggregate history columns remain separate, unimplemented work.

## Objective and scope

Use ordinary C++23 scoped enums as the single definition of fixed trait values
and names. Use `magic_enum` for their name lookup, reverse lookup, enumeration,
and counts. Preserve compact population storage, numeric simulation updates,
current input behavior, and output spellings.

Implement this before appending aggregate history columns. Keep the user's
current `HistorySelection` structure and field order:
`phase, trait_value, age, trait, ring`, with the last two defaulting to empty.
Do not reintroduce `HistoryColumnCoordinates`.

Keep `Variant`, `Vax`, `SDCase`, and `Ring` and their runtime registration and
lookup behavior. The attachment's proposed generic `DynamicEnum` and reverse
hash tables are outside this migration. Also leave duration/day wrappers,
per-person history structs, `ColumnName`, and unrelated enums alone.

## Recommended representation

Retain `Agegrp`, `Status`, `Condition`, and `Vaxstatus` as small concrete
structs used by `PopData` and `AgentView`. Each gains a nested ordinary
`enum class Enum : uint8_t` defining its fixed values. Keep the existing single
`uint8_t v` member and numeric conversion behavior. The enum supplies metadata;
there is no second per-person value or runtime metadata member.

This is a deliberate compatibility choice: `PopData` stores vectors of these
wrappers, `AgentView` returns mutable references to them, callers use `.v`, and
numeric parameters, subscripts, comparisons, and switches use their implicit
byte conversion. A scoped enum alone could be a vector element, but replacing
all those interfaces is unnecessary for the requested metadata simplification.

Use enum-derived convenience constants, retaining existing public spellings
such as `INFECTIOUS`, `AGE20_39`, and `Vaxstat::full`. Preserve their wrapper
types. Add a straightforward constructor from the nested enum to support this;
retain existing byte/int constructors and their explicitness. Avoid adding
another implicit outbound conversion to the enum alongside the byte conversion,
which could complicate overload resolution.

`Progressionmap` is fixed at compile time and is not a `PopData` column type.
Replace it with a plain `enum class Progressionmap : uint8_t`. Keep
`Progressmap::ToRecover` through `ToDead` as constexpr aliases of its enumerators.
Use explicit underlying-value conversions at its numeric comparison/subscript
sites. `Trait` and `Phase` remain plain scoped enums.

### Values and names to preserve

Assign numeric values explicitly in every definition.

| Enum | Values, in numeric order | Domain restrictions |
|---|---|---|
| `Agegrp::Enum` | `unknown=0`, `age0_19=1`, `age20_39=2`, `age40_59=3`, `age60_79=4`, `age80_up=5` | Real ages are 1..5; unknown is not total age. |
| `Status::Enum` | `none=0`, `unexposed=1`, `infectious=2`, `recovered=3`, `dead=4` | Real status history values are 1..4. |
| `Condition::Enum` | `uninfected=0`, `nil=1`, `mild=2`, `sick=3`, `severe=4` | Progression current-condition rows use values 1..4 projected to indices 0..3. |
| `Vaxstatus::Enum` | `none=0`, `first=1`, `full=2`, `booster=3` | Zero is a valid person state; effectiveness rows exclude it. |
| `Progressionmap` | `ToRecover=0`, `ToNil=1`, `ToMild=2`, `ToSick=3`, `ToSevere=4`, `ToDead=5` | All six values are valid zero-based outcome indices. |
| `Trait` | `status=0`, `vax=1`, `variant=2` | These describe history traits, not every person trait. |
| `Phase` | `now=0`, `new_=1` | Preserve current phase-token and presentation conventions separately. |

Remove `COUNT` from `Trait` and `Phase`; derive counts with `enum_count`.
Keep real sentinel enumerators such as `none` and `unknown`: they are data
values, unlike `COUNT`. Keep convenience aliases outside enum definitions so
reflection does not have to choose between duplicate enumerator names.

## Implementation sequence

### 1. Establish the baseline and package availability

- Preserve existing dirty edits in `series.*`, `sim.cpp`, `show_help.h`,
  `tui_commands.cpp`, and `xmake.lua`.
- The package is already declared with `add_requires("magic_enum")` and attached
  to both `epi_sim` and `test`. Verify resolution and record the installed
  version during the first named-target build. At plan time, the inspected
  xmake package cache has no magic_enum entry; installation was not attempted.
- Use the package's core `<magic_enum/magic_enum.hpp>` header. No hash-mode,
  flags, enum-container, or global stream-operator integration is needed.
- Establish application and test baselines before enum edits. There is an
  existing clangd error at `test/test_series.cpp:91`: the reverse-layout test
  passes string members of `HistorySelection` to the numeric history indexer.
  Update that test's conversions as a separate baseline reconciliation.
- Audit remaining positional selection literals and builders against the new
  five-field structure. In particular, `HistorySelectionSpec::build_for_ages`
  still emits the former combined-name/two-field form. These are remnants of
  the selection migration, not magic_enum regressions. Preserve intended
  assertions rather than weakening them to obtain a green baseline.

### 2. Migrate the fixed population wrappers

Start with `Agegrp`, then apply the same small pattern to `Status`, `Condition`,
and `Vaxstatus` in `src/traits.h`.

- Introduce the nested enum and derive each wrapper's `names` using
  `magic_enum::enum_names<Enum>()`. This retains the familiar `T::names` API
  while replacing its hand-maintained entries with generated string views.
- Implement `show()` using `enum_name(static_cast<Enum>(v))`, returning an owning
  `std::string` as today. This keeps existing rendering and error-message
  consumers working without a separate string-lifetime migration.
- Replace handwritten `resolve_name()` branches with case-insensitive
  `enum_cast<Enum>()` and the existing zero fallback. Retain the string
  constructors that depend on that behavior.
- Keep a small `trait_from_string<T>()` adapter: fixed wrappers dispatch through
  their nested enum, ordinary enums through `enum_cast<T>()`, runtime wrappers
  through their existing name-vector search. Retain its `optional<T>` result
  and case-insensitive behavior; failed parsing remains distinguishable from
  a valid zero-valued sentinel.
- Define wrapper constants from enum values, not a second list of numeric IDs.
  Assert one-byte size, trivial copyability, and the required numeric mappings.

This removes the fixed lookup implementations while retaining the struct
interfaces that have useful callers. Do not introduce a new wrapper hierarchy
or dynamic registry framework for this step.

### 3. Adapt consumers of generated string views

`enum_names` supplies `array<string_view, N>`, whereas current tables contain
`std::string`. This requires a caller pass even when `T::names` is retained.

| Area | Required work |
|---|---|
| `parameters.cpp`, `parameters.h` | Adapt owning-string bindings, JSON keys, validation messages, vaccine shot names, and fixed counts. Replace fixed-name searches with enum lookup where appropriate. |
| `cases.cpp` | Preserve `parse_term_val` errors and zero-sentinel rules; preserve age construction in social-distancing input. |
| `series.h`, `series.cpp` | Adapt history labels, counts, selection construction, and status-name resolution to string views. |
| `setup.cpp`, `sim.cpp` | Verify age counts and age labels still use exactly the same IDs and names. |
| `pop_serialize.cpp` | Verify wrapper `show()` continues supplying the same owning strings and serialized values. |
| Existing tests | Adapt direct name-table uses, string concatenation/JSON operations, and expected representations without changing simulation expectations. |

The important mixed-type case is `TraitSelectionView::trait_value_names` in
`series.cpp`: it currently accepts `span<const string>` for both fixed status
names and runtime vaccine/variant names. Resolve the static status case with
`enum_cast<Status::Enum>()`; retain the existing runtime-name scan for vaccine
and variant selectors. Share subsequent source-index expansion. Do not create
runtime string copies of every reflected name just to preserve that span type.

Preserve parser policies individually:

- `Agegrp`, `Status`, and `Condition` string constructors: case-insensitive,
  unknown input falls back to their zero sentinel.
- `trait_from_string`: case-insensitive, unknown input returns `nullopt`.
- Progression JSON age/condition keys and history age/status selections:
  preserve current exact-name acceptance and reject domain-invalid sentinels.
- `parameters.cpp::agegrp_from_string`, used for vaccination filters and ring
  input: preserve its underscore-insensitive behavior and warning/fallback.
  Use normalization or a small matching predicate around reflected metadata;
  case-insensitive enum_cast alone does not handle omitted underscores.

### 4. Migrate Progressionmap

- Replace its name table and `name()` with enum reflection and update its test.
- Derive `PROGRESSION_OUTCOME_COUNT` from `enum_count<Progressionmap>()`.
- Update `redistribute_probability`, `do_progression`, and progression test
  fixtures where `Progressmap` constants currently convert implicitly to
  byte indices or compare with a sampled `uint8_t` outcome. Use
  `std::to_underlying` for these numeric boundaries.
- Assert that outcomes 1..4 still match `Condition` values 1..4. This numeric
  correspondence is required by the existing outcome-to-condition assignment.
- Preserve the categorical draw, packed progression arrays, and indexing
  arithmetic. Enum reflection is not needed to choose or apply an outcome.

### 5. Migrate Trait and Phase metadata

- Replace manually listed `all_traits` and `all_phases` with generated enum
  values, either used directly or retained as generated constexpr aliases.
- Derive `phase_widths_` size and enum bounds from `enum_count`. Remove `COUNT`
  cases and update invalid-value validation without introducing hot-loop checks.
- Replace `trait_names`, `phase_names`, and name-only switches with generated
  names or direct `enum_name` calls. Keep the arithmetic `trait_phase_base`
  switch: it computes storage offsets and is not redundant name metadata.
- Preserve numeric `Trait` and `Phase` values, six-group history ordering,
  phase widths, and all existing atomic indices.
- Preserve the current `HistorySelection` strings and aggregate semantics.
  Its phase token is presently `"new_"`; `phase_label()` separately presents
  `"new"`. Reflect `Phase::new_` as `"new_"` and retain the presentation mapping
  where required. Do not globally customize it to `"new"` in this migration.
- Preserve `"total"`, `"vaccinated"`, and empty-ring meaning as domain selection
  rules. They are not new values of person enums. Fixed reflection does not
  replace runtime trait lookup or composite history selection.

### 6. Validate and complete

Run narrow groups after their corresponding changes: `traits`, `parameters`,
`disease_modeling`, `vaccination`, `series`, `pop_serialize`, `setup`, and `plot`.
Because `test` is one binary, the baseline reconciliation must precede even
the narrow runs.

Add focused coverage to existing groups for:

- Exact enum numeric IDs, counts, canonical names, one-byte wrapper storage,
  and round trips for all fixed values, including genuine sentinels.
- Each existing parser's case sensitivity, invalid-input behavior, and age
  aliases; exclusion of `COUNT`, unknown age, and status none where required.
- Unchanged runtime registration/lookup, including dynamically supplied names
  absent from every compile-time enum.
- Unchanged atomic-history index mappings and progression outcome mappings.
- Exact CSV/plot labels and data, with corrected current selection literals.

Build `epi_sim`; run the full `xmake run test` once the migration is complete.
Then explicitly run `xmake run test runsim --artifacts` (writes artifacts and
opens browser tabs) to validate end-to-end output. Run `git diff --check` and
Serena diagnostics on changed files. All builds use xmake with named targets.

Compare deterministic simulation outcomes and output data against the
reconciled baseline. Review numeric update code to ensure no added lookup,
allocation, validation, or struct construction in agent loops. If hot-loop
code generation changes, inspect optimized output and measure before accepting
it. Do not enable hashed enum lookup based on the attachment's performance
claims; these fixed enums have only two to six values.

One existing exception to the "names only outside hot loops" assumption is
`vaxeffect()`, which calls `Vaxstatus::show()` for an effectiveness diagnostic
argument. Preserve its interface and do not expand that work; optimizing that
existing diagnostic path is a separate decision.

Completion means all fixed metadata comes from enum declarations, runtime
registries retain their behavior, the reconciled tests pass, and atomic history
storage and update arithmetic remain unchanged. Aggregate-column finalization
can then proceed independently. Update session notes and the stale description
of `Progressionmap` as runtime-defined when implementing.

## Evidence and library references

The supplied discussion is design input, not an instruction to implement its
runtime-registry proposal. The source of truth for current behavior is the
working tree; the earlier session notes predate the user's HistorySelection
migration.

The [official reference](https://github.com/Neargye/magic_enum/blob/master/doc/reference.md)
documents `enum_name`, `enum_cast`, generated names/values/counts, and
case-insensitive matching. Names and values are ordered by numeric value;
reflection index and raw numeric value are different concepts, so preserve
the explicit contiguous-value invariants used by this project.

The [limitations](https://github.com/Neargye/magic_enum/blob/master/doc/limitations.md)
cover compiler support, reflection ranges, and alias behavior. These small
fixed enums fit the documented default range. Confirm APIs against the version
actually resolved by xmake rather than assuming the upstream master version.
