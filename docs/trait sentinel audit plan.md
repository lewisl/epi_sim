# Plan Trait Literal Audit And Range Hardening

## Summary
Do a targeted safety pass on trait wrapper construction with one narrow goal: keep intentional `0`/default sentinels, but eliminate or guard against accidental construction of invalid nonzero trait values, especially values greater than the known domain for a trait.

This is not a broad refactor away from trait wrappers. It is a focused audit and hardening pass that preserves the current 1-based indexing and `none`/empty-state sentinel design.

## Implementation Changes
- Audit all production uses of `Trait{...}` style construction in `src/` and classify each use into one of three buckets:
  - Allowed sentinel/default: `Variant{}` / `Vax{}` / `Duration{0}` / similar empty-state initialization.
  - Allowed conversion from validated input: parsing from JSON strings, lookup results, loaded parameter names, or current agent state.
  - Suspicious hard-coded nonzero literal: any direct `Trait{N}` where `N > 0` is not clearly derived from validated data or a named domain constant.
- Leave the known sentinel uses unchanged:
  - population initialization defaults
  - empty-history returns like `VariantHist::latest()` and `VaxHist::latest()`
  - temporary “no selection” sentinel logic such as the vaccine-choice fallback, unless it is converted to a named helper without changing behavior
- Replace any suspicious hard-coded nonzero literals that remain with one of:
  - an established named constant such as `UNEXPOSED`, `NIL`, `UNKNOWN`, `Vaxstat::none`
  - a validated lookup result from `names`
  - a value passed through the call stack from already-parsed configuration or existing agent state
- Add explicit range validation for runtime-indexed trait types where invalid values are currently possible:
  - `Variant`
  - `Vax`
- Implement that validation at the narrowest safe boundary used by runtime conversions, not everywhere:
  - parsing from config/JSON index values
  - lookup helpers that convert `names` positions into wrapper types
  - any place a raw integer is rewrapped into `Variant` or `Vax`
- For compile-time trait-like wrappers with fixed domains (`Status`, `Condition`, `Agegrp`, `Vaxstatus`), prefer named constants in business logic and only allow raw numeric construction in parsing/deserialization code.

## Public/API Or Type Changes
- Keep existing trait types and indexing conventions unchanged.
- Add small checked helpers for runtime-indexed traits if needed, for example:
  - `checked_variant(uint8_t idx)`
  - `checked_vax(uint8_t idx)`
- Those helpers should:
  - accept `0` as the valid sentinel
  - reject values `>= names.size()`
  - throw `std::runtime_error` or `std::out_of_range` with a message that identifies the trait type and bad index
- Do not change the meaning of `0` for sentinel/empty state.
- Do not change JSON schema or parameter file formats in this pass.

## Test Plan
- Add focused unit tests for checked runtime-index conversion:
  - `0` is accepted for `Variant` and `Vax`
  - a valid loaded index is accepted
  - an out-of-range index throws
- Add regression coverage for any suspicious call sites found during the audit and converted to named constants or validated lookups.
- Keep current seed-case and vaccination behavior unchanged for valid inputs.
- If a runtime index is now rejected, add a failure-path test that confirms the exception is raised before invalid state reaches `PopData`.

## Assumptions
- `0` remains the correct sentinel for runtime-indexed traits and empty state.
- The goal is not to ban all brace construction of trait wrappers; it is to ban unsafe or semantically arbitrary nonzero literals in production logic.
- Parsing/deserialization code is allowed to construct wrappers from integers only after validation.
- Test fixtures may continue to use direct literals where they are deliberately setting up known values, unless you later want a separate readability cleanup in tests.
