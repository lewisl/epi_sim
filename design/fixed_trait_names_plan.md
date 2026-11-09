# Fixed trait names using directly initialized immutable vectors

Status: implementation authorized and completed on 2026-11-09 after checkpoint
014dbde. Application build passes. The user's new case-1 CSVs match the earlier
run byte-for-byte; embedded plot data/layout also match. User reports successful
visual runs with multiple variants/rings and mixed totals, plus a separate
vaccination case. Vaccination combined with multiple rings remains untested;
user accepts that limit and is reviewing the diff. See SESSION.md for evidence
and the unrelated test-target build blocker. The sections below record the
agreed plan.

## Objective and required representation

Replace the nested enums and magic_enum-based mappings in those four wrapper
types with one authoritative name definition per type.

All name tables remain real `std::vector<std::string>` objects. Their elements
own their strings. Do not substitute arrays, string_views, or view-based name
storage. Initialize the tables once and expose them as const.

Retain distinct wrapper types and one-byte per-person storage. A position in
the name vector is the raw numeric ID. Constructors and parsers enforce the
chosen validity rules; generic history selector keys continue to use numeric
IDs and their own total sentinels.

This follows the names-vector representation already used by the runtime
traits; it does not introduce a new application model. The larger audit is
needed because these four types have more callers and more varied uses. Focus
verification on preserving those contracts, rather than adding infrastructure
or treating the representation change itself as high risk.

## 1. Direct initialization; const provides immutability

Define the actual member directly, for example inside Agegrp:

```cpp
inline static const std::vector<std::string> names{
    "unknown", "age0_19", "age20_39",
    "age40_59", "age60_79", "age80_up"
};
```

This is the one authoritative name list. Its const qualification prevents
mutation after initialization. There is no need for compile-time name storage,
a factory, a second temporary catalog, or compile-time string lookup. Do not
add any of those mechanisms. Do not introduce arrays or string_view tables.

Before implementation, record current user edits and diagnostics. Preserve
the user's new constrained Agegrp constructor and strict name parsing. Do not
reset unrelated changes or fold in outstanding selector repairs.

## 2. Define each catalog once and preserve its IDs

Move each current enum's spellings into its names member in exactly the
current numeric order. Do not renumber values, remove zero entries, or change
spellings during this migration.

| Type | Entries including zero | Zero means | Existing count convention |
| --- | ---: | --- | --- |
| Agegrp | 6 | unknown | Excludes zero: 5 |
| Status | 5 | none | Excludes zero: 4 |
| Condition | 6 | error | Excludes zero: 5 |
| Vaxstatus | 5 | none, a valid status | Includes zero: 5 |

Use `names.size()` for the total number of named values. Do not add a separate
name_count member just to retain a compile-time result. Preserve the existing
uint16_t count interface and the distinct count meanings. Vaxstatus's maximum
numeric ID is `names.size() - 1`, not `count`.

There are real compile-time count consumers in the current application:

- `ProgressionTree::entry_index` in parameters.h uses Agegrp::count as an array
  extent; `seen_ages` in load_progression_set does too.
- `OutcomesByCurrentCondition` uses PROGRESSION_CONDITION_COUNT as an array
  extent. That dimension currently comes from Condition::count minus one.

Keep the necessary Agegrp and Condition counts as explicit constexpr layout
constants with their current values. Check that they agree with the actual
names-vector sizes during existing setup validation or focused runtime checks.
These constants describe fixed application dimensions; they do not make the
name storage constexpr. Derive counts from names.size() where no compile-time
consumer requires otherwise, notably Status and Vaxstatus. Do not replace
fixed progression arrays or invent factories to conceal this dependency.

Check the actual name definitions for empty/duplicate names and byte-ID capacity
at an existing initialization/validation boundary, not on every lookup or
population iteration. Do not create another complete name list as an expected
test result or add a macro/code-generation framework.

## 3. Share a small names-based lookup implementation

Use a small shared runtime lookup operating on
`const std::vector<std::string>&` and `const std::string&`. It returns an
optional raw ID. Keep exact and case-insensitive matching choices explicit.

Reuse the existing names-based matching code where suitable. Ordinary string
operations are sufficient; do not implement a special constexpr case-folding
routine. Use unsigned-character conversion when calling character functions.
Do not add a separate parser for every wrapper or a new lookup framework.

Search the already initialized static vector. Never reconstruct the list for
a lookup. The only persistent names storage is the actual static const vector.

The shared lookup returns failure; individual callers retain their current
failure policy. In particular:

- Preserve Agegrp's newly chosen throwing behavior for invalid constructor
  text. The literal "unknown" remains a valid name.
- Preserve Status and Condition constructor fallback policies until their
  separate reviews decide otherwise. Removing reflection must not silently
  change those decisions.
- Preserve `trait_from_string<T>` returning nullopt for invalid input.
- Preserve existing exact-match and alias policies at external parsing sites.

## 4. Migrate the wrappers and their named constants

For each wrapper:

1. Initialize its immutable names vector directly from its literal list.
2. Replace enum-based rendering with access to the owned names vector. Preserve
   the existing `std::string` return type and define invalid-raw-value handling
   explicitly; do not leave an unchecked out-of-range vector access.
3. Replace enum-based name resolution with the shared lookup.
4. Remove its Enum constructor and, once references have migrated, its nested
   Enum definition. This removal is part of the requested representation change.
5. Use checked, constrained integer construction, preserving raw input until
   after validation. Validate against `0 <= input < names.size()`, including for
   Vaxstatus, and then narrow to uint8_t. Other wrapper types must not qualify
   as numeric inputs through their conversion operators.
6. Preserve the named public constants: UNKNOWN, AGE0_19, AGE20_39, AGE40_59,
   AGE60_79, AGE80_UP; NONE, UNEXPOSED, INFECTIOUS, RECOVERED, DEAD;
   UNINFECTED, NIL, MILD, SICK, SEVERE; and the Vaxstat constants.

Named constants can be ordinary `inline const` instances initialized by checked
name lookup after their names vectors, for example AGE20_39 from "age20_39".
That preserves their meaningful public names without a separately maintained
numeric mapping. Const supplies the required immutability; do not add machinery
merely to retain constexpr on these objects.

Vaxstatus currently has neither a default nor a string constructor. Its named
constants can use a checked numeric ID resolved by the shared runtime lookup,
without inventing either constructor solely for constant initialization.

Remove constexpr where a constructor or consumer now depends on runtime names
storage. Audit static initialization order and genuine constant-expression uses,
including value-based static_asserts and HISTORY_AGE_TOTAL. Preserve type/size
assertions, which do not need runtime name data. Convert meaningful value checks
to the normal validation path rather than rebuilding compile-time lookup.
Define each names vector before named objects that use it, and inspect other
static initializers that depend on those objects. Do not put string lookup in
population loops; these constants are initialized once.

Keep default-state meanings, equality, one-byte storage, and numeric extraction
compatible with the existing application. Making v private and redesigning
AgentView's mutation interface remain separate review work; do not smuggle
those broader changes into the metadata migration.

## 5. Audited downstream changes

This inventory was checked against current src/ and test/ code with Serena
references and a text scan of magic_enum, nested Enum, and conversion APIs.
References to a dependent parser matter even where its caller never mentions
magic_enum. Recheck before implementation because the user is editing source.

### 5.1 Direct mapping and representation changes

| File and symbols | Required action |
| --- | --- |
| traits.h: Agegrp, Status, Condition, Vaxstatus | Replace reflected names/count construction, enum constructors, resolve_name where present, and show(); remove the four nested enums after their consumers migrate. Numeric-to-name rendering must handle invalid raw values safely; zero must still render its named entry. |
| traits.h: UNKNOWN/AGE* constants; NONE/UNEXPOSED/INFECTIOUS/RECOVERED/DEAD; UNINFECTED/NIL/MILD/SICK/SEVERE; Vaxstat constants | Resolve the names from their own tables and retain the public symbols, IDs, and wrapper types. Audit value-based static_asserts when changing constexpr objects to const objects. Keep sizeof/trivially-copyable checks. |
| traits.h: trait_from_string<T> | Replace the nested-Enum branch with names-based lookup for the four fixed wrappers. Retain the ordinary-enum branch and runtime registry behavior. Construct fixed results as checked T{raw_id}. |
| traits.h/traits.cpp: TraitEntry and trait_table | Keep const vector<string>& and count references. Do not copy runtime registries or capture their counts as values. The constexpr table of references does not require the referenced vectors to be constexpr. |
| traits.cpp: tr_val_to_id overloads and tr_val_to_string | Audit the existing lookup bounds and sentinel policies before using these as the common implementation; see 5.3. Keep the TraitID overload forwarding to the numeric lookup. |
| parameters.cpp: load_progression_set | Replace enum_cast<Agegrp::Enum>, Enum::unknown comparison, and the adjacent to_underlying expression together. Obtain a numeric age ID, reject zero/unknown, and preserve age_idx = age_id - 1. Preserve duplicate/missing-age detection and input-order independence. |
| parameters.cpp: load_progression_set condition-row decoding | Preserve Condition::names[cond_idx + 2]: rows are nil through severe; error and uninfected are not matrix rows. Removing reflection must not shift these numeric relationships. |
| parameters.h: ProgressionTree, OutcomesByCurrentCondition, PROGRESSION_CONDITION_COUNT | Keep the genuine compile-time dimensions described in section 2. The local seen_ages array in load_progression_set has the same age-count requirement. |
| series.h: age_history_idx_from_string | Replace enum_cast<Agegrp::Enum>, Enum::unknown comparison, and to_underlying together. Keep optional<uint8_t>, exact matching, rejection of unknown, and the special total result. Convert a wrapper-typed total constant to its raw ID explicitly at this boundary. |
| series.h: age_vec_label and HISTORY_AGE_TOTAL | Check their constexpr assumptions and typed/numeric conversion boundary if named objects now initialize at runtime. age_vec_label already reads Agegrp::names; do not route its names through a new enum. No current callers of age_vec_label were found; do not delete it as cleanup. |

Do not simply delete the nested-Enum branch of trait_from_string and let these
types fall into the current runtime branch. That branch uses T result{} and
then writes result.v. Vaxstatus has no default constructor, and direct writes
would bypass checked construction. Conversely, indiscriminately constructing
all runtime results as T{raw_id} would change Ring's zero-sentinel behavior.
Handle the fixed wrappers deliberately without a new trait-policy framework.

Named-constant initialization must resolve against its own type's initialized
names vector, rather than depend on the global trait_table's initialization.
Read downstream static initializers before changing their constexpr qualifiers.

### 5.2 Preserve each conversion contract

These paths intentionally do not all accept the same spellings or use the
same failure policy. Shared lookup must allow the callers to retain them.

| Conversion path | Matching and result to preserve |
| --- | --- |
| Agegrp string constructor / resolve_name | Case-insensitive; explicit unknown is valid; unrecognized input throws under the user's current change. |
| Status and Condition string constructors / resolve_name | Case-insensitive; preserve their current NONE and UNINFECTED fallbacks until their separate reviews change that policy. |
| trait_from_string for the four fixed wrappers | Case-insensitive optional result; invalid input returns nullopt; valid named zero entries are representable. No new Vaxstatus default constructor is needed. |
| Generic tr_val_to_id | Exact matching and existing exception contract. A generic ID result is a uint8_t, not a wrapper. |
| Progression-tree age decoding | Exact matching; unknown and invalid ages rejected before indexing a real-age array. |
| History age decoding | Exact matching; unknown, uppercase aliases, and age80up rejected; total is the explicit numeric-total sentinel. |
| parameters.cpp: agegrp_from_string | Existing case-insensitive and underscore-insensitive aliases remain; an unknown name warns and returns UNKNOWN. |

In particular, replacing every enum_cast with the throwing Agegrp constructor
would change both exact-match behavior and optional-return behavior. Replace
the mapping operation while preserving the surrounding caller's contract.

### 5.3 Existing generic helpers need a bounds review before reuse

The current helpers are not safe drop-in replacements for all four wrappers:

- tr_val_to_id iterates i <= trait.count. Vaxstatus::count includes its zero
  entry: it is 5 for a vector of length 5, so an unsuccessful search can reach
  names[5]. Use the actual table size as the search boundary and a size_t
  traversal variable; narrow only a verified result. Preserve the existing
  disabled-runtime-trait guard and exact-match/exception policies.
- tr_val_to_string rejects v < 1 and v > count - 1. That excludes named zero
  entries and excludes the final Agegrp/Status/Condition value. No current
  callers were found. Do not silently reuse it for show() without correcting
  the required domain policy. Either make the common conversion use actual
  names.size() bounds with caller-specific eligibility checks, or keep show()
  as a small safe access to its own table. Do not delete the unused helper.

These are pre-existing helper limitations. Record their treatment explicitly
when selecting the common lookup implementation; do not attribute them to
the removal of magic_enum or redesign unrelated runtime loaders.

### 5.4 Indirect consumers and constructor/assignment boundaries

| File and symbols | Audit requirement |
| --- | --- |
| cases.cpp: parse_term_val | Calls trait_from_string for all four types, then extracts raw IDs for seed/filter/change terms. Preserve rejection messages and case-insensitive parsing. Runtime Variant/Ring parsing and their zero rejection must continue to work. |
| cases.cpp: load_sd_case | Calls Agegrp::resolve_name for include_ages. Preserve the newly strict Agegrp behavior and the resulting numeric values. |
| parameters.cpp: agegrp_from_string, load_vax_sched, load_ring_traits | These already use names lookup. Keep vaccine-filter aliases and the ring loader's explicit rejection of UNKNOWN; no rewrite is required merely because the nested enum disappears. |
| cases.cpp: apply_change | Review checked construction of Status, Condition, Vaxstatus from parsed term IDs, including casts before construction. Preserve make_sick/make_well/make_dead routing; any untrusted numeric input must be checked before narrowing. |
| progression.cpp: do_progression | An arithmetic result is currently assigned to Condition through implicit construction. Make the final construction explicit if required, preserving the outcome-to-condition mapping and avoiding string conversion in the kernel. |
| disease_modeling.cpp: touch_map; test/test_runsim.cpp: tally | Named objects initialized by runtime lookup cannot be switch case labels. Use equivalent comparisons against the same public constants, preserving values and invalid-input behavior without hardcoded replacement IDs. |
| population.h: PopData constructor and age_distribution | Named constants initialize status, condition, vaccination status, and real ages. Preserve one-based person indexing, initial values, and the initialization order of the named constants they copy. |
| series.cpp: build_history_vectors, select_indices, aggregate_value | Review numeric-to-Agegrp construction, total constants, and generic raw-ID lookup at selector boundaries. Keep numeric selector coordinates consistent with Selsim/Seltot; no Ring{0} construction for totals. |
| pop_serialize.cpp and presentation callers of show() | Existing wrapper APIs should keep call sites working. Verify the same names and numeric IDs in output; no blanket rewrite of formatting calls is expected. |

Agegrp and rings remain fixed for existing people. Status, Condition, and
Vaxstatus transitions must retain their existing behavior and valid domains.
Do not add allocations/name lookup to unchanged hot-path reads or transitions.

### 5.5 Tests and current documentation

- test/test_traits.cpp: replace check_fixed_trait_metadata's enum_count and
  enum_values use for T::Enum; remove its caller-maintained full name arrays.
  Exercise the actual vectors and wrappers with the agreed behavior checks.
  Preserve the separate Progressionmap reflection checks; that remains an enum.
- test/test_series.cpp: the reverse-lookup test indirectly uses
  trait_from_string<Status> and age_history_idx_from_string. Preserve its
  age-token acceptance/rejection cases, while leaving TraitID/Phase reflection
  tests intact. Do not redesign its harness or repair unrelated old tests.
- test/test_parameters.cpp: retain the existing reordered-age progression
  coverage and parameter-loading checks; these exercise the changed age parser
  and the Condition row-name mapping without duplicating their implementation.
- Existing trait artifact rendering and population/plot serialization tests
  exercise show()/lookup transitively; keep their output formats unchanged.
- During implementation, update design/data-structures.md, whose current text
  says these wrappers contain reflected nested enums. Historical migration
  notes, exploratory transcripts, and the vendored magic_enum README are not
  current code and must not be rewritten by broad search/replace.
- The older test_traits_repair_plan.md describes an enum-driven approach and
  is superseded for these four types. Do not implement that obsolete direction.

### 5.6 Reflection uses that must remain

| Location | Remaining legitimate magic_enum use |
| --- | --- |
| traits.h / traits.cpp | TraitID count for the trait_table declaration; trait_lookup name/value conversion for TraitID; the ordinary-enum branch of trait_from_string. |
| series.h / series.cpp | TraitID and Phase names, all_traits/all_phases, ID/count assertions, and trait/phase label construction in build_history_vectors. |
| parameters.h | PROGRESSION_OUTCOME_COUNT reflects Progressionmap, which remains the zero-based outcome enum. |
| test/test_traits.cpp | Progressionmap/Progressmap name, value, and parsing checks. |
| test/test_series.cpp | TraitID and Phase parsing and reflection tests. |
| xmake.lua and applicable source includes | Keep the magic_enum dependency and headers needed by these real enum consumers. Scratch utilities are outside the product migration. |

Tracked_Traits and trait_table can remain constexpr arrays. TraitID, Phase,
Progressionmap, and unrelated enums remain enums. No vector needs constexpr.

Use Serena references plus the owning-type/caller pass before editing each
affected file. Changing a parser's result from enum to numeric ID also requires
reviewing adjacent to_underlying calls, zero comparisons, and array projections.
Do not use global text replacement to remove every magic_enum call.

## 6. Keep selector sentinels separate from population validity

Seltot/Selsim store raw uint8_t coordinates. Their zero sentinel means a total
dimension. Population traits have their own zero meanings. Keep those roles
explicit; constructing Ring{0} merely to form a selector is inappropriate with
the current Ring constructor.

Preserve the user's current sentinel declarations unless separately agreed.
Extract numeric IDs explicitly at selector boundaries and construct actual
traits explicitly when assigning metadata or population values. Do not weaken
trait constructors to restore implicit assignments in generic numeric code.

Initial population setup supplies valid age groups and rings. Later Status,
Condition, and Vaxstatus transitions must continue to preserve their domains.
This migration must not add string lookup, allocation, or repeated validation
to unchanged hot-path reads or population loops. The static vector is built
once; names lookup belongs at construction/parsing boundaries.

## 7. Validation and stopping conditions

Acceptance is a complete application simulation run with serialized output
and plots working correctly. A passing unit-test suite is not a substitute.
This user-selected acceptance criterion supersedes the default broad unit-test
sweep for this migration; do not make a suite rewrite part of this work.

- Build the application with `xmake build epi_sim` after migrating dependent
  call sites. Preserve the manually configured LLVM/vcpkg environment.
- Use an existing representative case that exercises population initialization,
  progression, vaccination, and history selection/aggregation. Run through all
  configured simulation days, serialization, and plot generation. Use a separate
  output location or an isolated case copy; preserve existing user case data.
- Inspect the serialized population/history output for correct trait names,
  numeric IDs, labels, and totals. Open the generated plots and verify the
  expected series, labels, and data render correctly. Successful process exit
  and the mere existence of output files are not sufficient acceptance.
- Compare with an available pre-migration run of the same case. Use matching
  randomness settings where supported before expecting identical trajectories;
  otherwise assess output structure and simulation behavior without demanding
  byte-identical stochastic results. Record any unavailable baseline or
  unexercised path rather than claiming it was verified.
- Repair tests that reference removed APIs as needed to keep them compatible.
  Focused unit tests may help investigate a concrete concern or failure, but
  are supporting diagnostics, not the acceptance gate. Do not add catalogs of
  duplicate expected values or expand into unrelated test repairs.
- Record the case, invocation, output location, and what was inspected so the
  acceptance result is reproducible. Planning alone does not authorize running
  a simulation now; perform this acceptance run when implementation is authorized.

Complete the migration audit alongside that application acceptance run:

- Check direct initialization of the static const vectors, their dependent
  counts/named constants, and the remaining genuine compile-time array extents.
- Verify unchanged numeric IDs, byte storage, named constants, and runtime
  vector/string ownership. Explicitly verify Vaxstatus's final valid ID and
  Condition's error/uninfected distinction.
- Confirm fixed-trait Enum references have been removed, legitimate remaining
  magic_enum users remain, and no factories, temporary compile-time catalogs,
  constexpr string-lookup helpers, name-table arrays, or string_views were added.
- Repeat the source/test scan for the four nested Enum types and the dependent
  T::Enum parser/test branches. Review remaining magic_enum and adjacent
  std::to_underlying calls against section 5.6; legitimate real-enum uses are
  expected to remain. Recheck indirect callers after the changed parsers bind
  successfully, rather than relying only on absence of old symbol names.

Planning originally changed only this document and the session handoff.
Implementation was subsequently authorized; current implementation and
acceptance evidence are recorded in SESSION.md. The existing test-only repair
plan is superseded for these four traits and must not be implemented as an
enum-based redesign.
