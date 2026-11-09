# Repair plan for test/test_traits.cpp

Status: proposed; no C++ implementation performed.

## Scope and invariants

- Make implementation changes only in `test/test_traits.cpp`.
- Preserve all production source, including the current enum values, sentinel
  meanings, constructors, count types, parsing rules, and runtime registries.
- Preserve the existing `traits` group and test runner. Do not add groups,
  change shared test support, or undertake a broader test-suite migration.
- Exercise the real types from `src/traits.h`. Do not declare substitute traits
  or maintain a second complete catalog of their fixed names and values.
- Preserve unrelated working-tree edits. An existing dirty production file
  must not be reset as part of validation.

## Diagnosis of line 111

`check_fixed_trait_metadata` currently takes an
`std::array<std::string_view, N>`. Its caller supplies the array extent, so
`N` is deduced at instantiation. In the `Condition` call at line 134, the
argument is explicitly an array of five elements. The resulting specialization
is `check_fixed_trait_metadata<Condition, 5>`.

The actual production enum is:

```cpp
enum class Enum : uint8_t {
    error = 0, uninfected, nil, mild, sick, severe
};
```

Thus reflection counts six entries and the test supplies five. The diagnostic
`6 == 5` is correct. `5UL` means unsigned-long 5; on this platform that is how
the diagnostic spells the deduced `size_t` value. The installed magic_enum
header declares `enum_count` to return `std::size_t`.

Changing literal suffixes or casting the operands does not repair this stale
expectation. The helper already instantiates the real production types; the
duplicated expected-name arrays are the part that has drifted.

## Implementation sequence

### 1. Make the existing fixed-trait helper take only the real type

Replace the `T, N` helper interface with a `T`-only helper, called as:

```cpp
check_fixed_trait_metadata<Agegrp>();
check_fixed_trait_metadata<Status>();
check_fixed_trait_metadata<Condition>();
check_fixed_trait_metadata<Vaxstatus>();
```

Obtain `(enum_value, name)` pairs from
`magic_enum::enum_entries<typename T::Enum>()`. Remove the caller-supplied
array and its `N` template parameter, rather than changing five to six.

For every reflected entry, exercise the actual production wrapper:

- Construct it from the enum value and check that `v` equals
  `std::to_underlying(enum_value)`.
- Construct it from the same raw numeric value through an existing supported
  constructor and compare it with the enum-constructed wrapper.
- Check `show()` against the reflected name.
- Check `trait_from_string<T>` with the exact and uppercase name against the
  wrapper, retaining case-insensitive lookup coverage.
- Check that the public names collection agrees in size and sequence with the
  reflected entries. Include sentinel entries when checking metadata.
- Retain the one-byte storage and trivially-copyable assertions.

Use `size_t` for collection indices. Do not equate an enumeration's ordinal in
the reflected sequence with its underlying value: compare stored values with
the actual enum value. Do not introduce an unconditional `i + 1` adjustment.

### 2. Respect each trait's actual sentinel and count semantics

The current source defines these relationships:

| Type | Zero entry | Reflected entries | Production count |
| --- | --- | --- | --- |
| Agegrp | unknown | All entries, including unknown | Excludes unknown |
| Status | none | All entries, including none | Excludes none |
| Condition | error | All entries, including error | Excludes error |
| Vaxstatus | none | All entries, including none | Includes none |
| Progressionmap | ToRecover | All outcomes | Zero-based outcome enum |

Test count relationships using the actual production count and reflected
domain, with explicit handling of Vaxstatus. Do not assume one count convention
for all traits or change the production `uint16_t` count members to `size_t`.

The Condition round trips must cover the zero `error` entry as metadata and
the actual positive values for uninfected through severe. This does not imply
that error is a valid simulation condition.

### 3. Remove the other duplicated fixed-enum catalogs

- Replace the six-string Progressionmap outcomes array with iteration over the
  actual `Progressionmap` entries. Check parsing and actual underlying values;
  preserve its distinct zero-based semantics.
- Drive exhaustive Agegrp string-constructor round trips from its actual enum
  entries instead of separately listing every age-group name. Exercise the
  real string constructor as well as the generic parser; they are different
  paths in production.
- Exercise Status and Condition string constructors with their actual enum
  names. Do not require Vaxstatus to support a string constructor it lacks.
- Retain focused behavior assertions using production constants, such as
  mixed-case input and the unknown-input fallbacks `UNKNOWN`, `NONE`, and
  `UNINFECTED`. Retain rejection checks for empty/invalid parser input.
- Retain a few explicit public spelling examples already in the file. They
  test intentional external behavior; they are not another complete enum
  catalog. Do not reduce the tests to comparing reflection with itself.

### 4. Repair stale Ring fixtures using the existing production API

`Ring(int)` currently validates the value against the runtime registry, while
default `Ring{}` supplies the zero sentinel. Update the tests accordingly:

- In `test_wrapper_comparisons`, establish isolated, registered Ring names
  before comparing two real Ring values. The test must not depend on whatever
  registry an earlier test left behind.
- In `test_ring_runtime_registration_renders_names`, use default `Ring{}` or
  inspect the parsed result's raw value for the sentinel checks. Replace the
  old assumption that numeric `Ring{0}` is a supported sentinel constructor.
- Preserve the registered-name, mixed-case lookup, and numeric fallback
  rendering checks. Keep deliberate raw assignment coverage separate from
  validated numeric construction.

Do not relax or change Ring's constructor in production to satisfy the tests.

### 5. Preserve runtime-trait and history coverage, with isolated state

The Variant, Vax, SDCase, and Ring fixture names are inputs to real runtime
registries, not substitute trait definitions. Keep those tests and their
controlled names. Likewise, retain scalar-wrapper comparisons, empty-history
checks, and overflow ordering/rendering checks; their concrete examples test
behavior rather than duplicate a fixed enum definition.

The existing names guards restore names only. Variant, Vax, and Ring
registration also changes static count members. Within this test file,
preserve and restore any counts changed by these fixtures, including on
exceptional exit, without extending the shared harness. Reset the relevant
fixture count explicitly when establishing an empty registration fixture.
Avoid adding assertions that require a production runtime count to follow a
new convention.

Leave the optional artifact format and unrelated formatting alone.

### 6. Validate the test-only patch

1. Request Serena diagnostics for `test/test_traits.cpp`; confirm the old
   `Condition, 5UL` static assertion and any new diagnostics are resolved.
2. Run `xmake run test traits`. This rebuilds the test target using the existing
   configured LLVM/vcpkg environment.
3. If another test translation unit blocks the build, record its exact
   diagnostic as a separate blocker. Do not edit another test or production
   file merely to make this command run.
4. Once the narrow group passes, run `xmake run test` to check that temporary
   runtime registry changes do not disrupt subsequent groups. Do not run the
   separate slow/artifact-producing `runsim` group for this repair.
5. Check the task's diff: implementation changes belong only to
   `test/test_traits.cpp`. Compare with the initial dirty state when confirming
   production files were not altered. Run a whitespace check on that file.

## Completion criteria

- Fixed-trait traversal follows the actual source definitions, including new
  sentinel entries, without an independently maintained full name list.
- Assertions test wrapper construction, rendering, parsing, invalid-input
  behavior, storage, and each trait's current count semantics.
- Ring tests use registered values and default construction appropriately.
- Runtime fixture names and any counts changed by this file are restored.
- The traits group passes, or unrelated build blockers are reported precisely.
- No production source or shared test harness changes.

## Verification performed while planning

Read the full test file, the relevant real trait definitions and constructors,
the generic parser, registry guards, representative history implementations,
the installed magic_enum signatures, and the xmake test target. Serena confirms
the failing instantiation is Condition with N=5 and a reflected count of six.
No builds or tests were run and no C++ files were modified.
