# PopData Design

## Overview

`PopData` is the simulation's core population store. It uses a **struct of arrays (SoA)** layout: each member vector is one attribute across all people, and index `i` refers to the same person in every vector.

This design is intentionally column-oriented:

- fast indexed access in hot loops
- good cache locality when touching one attribute across many people
- simple storage for serialization and debugging
- direct alignment between simulation state and physical stored columns

## One-Based Population Storage

All `PopData` vectors are **1-based**.

- `popn` is the real population size
- `popz = popn + 1` is the physical vector size
- index `0` is unused
- valid person indices are `1..popn`

This matches the simulation's person numbering and keeps agent id and vector index aligned.

## Why SoA Instead of Array-of-Structs

With an array-of-structs design, one person's entire state is packed together. That can be convenient, but it is a poor fit for this model, where most kernels touch only a few attributes at a time.

With `PopData`, all values for one attribute are contiguous:

```cpp
for (int i = 1; i <= pop.popn; ++i) {
    if (pop.status[i] == INFECTIOUS) {
        pop.duration[i] += 1;
    }
}
```

This keeps the memory layout close to the actual work being done.

## AgentView: Virtual Mutable Row

SoA storage is efficient, but person-oriented code is awkward if every access has to spell out `pop.column[i]`.

`PopData::AgentView` is the compromise:

- it stores a reference to `PopData`
- it stores one person index
- it exposes row-like accessors such as `status()`, `variant()`, and `vaxday()`
- it does not materialize a copied row

That means code can stay person-oriented without giving up columnar storage:

```cpp
auto person = pop.agent(i);

if (person.status() == UNEXPOSED) {
    person.status() = INFECTIOUS;
    person.cond() = MILD;
    person.duration() = Duration{1};
}
```

`AgentView` is intentionally mutable. Its accessors return references into the underlying vectors.

## Trait Structs Instead of Plain Enums or Raw Primitives

The older design discussion was framed as "pseudo-enums." The current code is more explicit than that.

Most semantic column types are now tiny dedicated structs in [`src/traits.h`](/Users/lewislevin/code/epi_sim/src/traits.h):

- compile-time categorical traits such as `Status`, `Agegrp`, `Condition`, and `Vaxstatus`
- small scalar wrappers such as `Duration`, `Sickday`, `Recovday`, `Testday`, `Quarday`, and `Vaxday`
- runtime-loaded traits such as `Variant` and `Vax`

These types are still compact, but they carry convenience behavior that plain enums or raw integers do not:

- `show()` for readable output
- per-type names and string lookup
- a valid column-specific type at each vector element
- small operator support where it is actually needed, such as arithmetic on `Duration`

This also fixes the old debugging problem. A column no longer has the type "some arbitrary primitive that happens to mean status." It has the correct semantic type for that column.

The underlying storage is still compact:

- many categorical traits use `uint8_t`
- ordinal day wrappers use `int16_t`
- the type stays small enough for dense population storage and fast copying

## Real Columns in PopData

The **real columns** are the physical vectors in `PopData`. That is the source of truth.

Derived values are not columns. For example, `tested` is derived from `testday != 0`, but it is not stored as its own vector and should not be printed or serialized as if it were a physical column.

## Stored Vectors

| Vector | Purpose | Element type | Notes |
|---|---|---|---|
| `status` | current infection state | `Status` | categorical trait |
| `agegrp` | age bucket | `Agegrp` | assigned at initialization |
| `cond` | current severity/condition | `Condition` | categorical trait |
| `duration` | days being infected | `Duration` | small counter |
| `variant` | current active variant | `Variant` | runtime-loaded names |
| `variant_hist` | prior variant history | `VariantHist` | up to 16 stored entries |
| `sickday` | day sickness started | `Sickday` | `0` is sentinel for none |
| `sickday_hist` | sickness-day history | `SickdayHist` | up to 16 stored entries |
| `recovday` | recovery day | `Recovday` | ordinal sim day |
| `recovday_hist` | recovery-day history | `RecovdayHist` | up to 16 stored entries |
| `deadday` | death day | `Deadday` | ordinal sim day |
| `ring` | ring assignment / ring flag | `uint8_t` | small numeric code |
| `sdcase` | social distancing case marker / code | `uint8_t` | small numeric code |
| `testday` | latest test day | `Testday` | `0` means never tested |
| `testday_hist` | testing history | `TestdayHist` | up to 16 stored entries |
| `quar` | quarantine flag | `uint8_t` | pseudo-bool stored as byte |
| `quarday` | quarantine day | `Quarday` | ordinal sim day |
| `vaxstatus` | vaccination status class | `Vaxstatus` | none/first/full/booster |
| `vax` | latest vaccine type | `Vax` | runtime-loaded names |
| `vax_hist` | vaccine history | `VaxHist` | up to 16 stored entries |
| `vaxday` | latest vaccine day | `Vaxday` | ordinal sim day |
| `vaxday_hist` | vaccine-day history | `VaxdayHist` | up to 16 stored entries |

### Notes on History Vectors

The history types use a small fixed-capacity array plus a count.

- no per-agent dynamic allocation in the simulation loop
- enough room for ordinary usage
- if more than 16 events occur, the oldest entries are dropped and the newest are retained

## Printing and Serialization

Each semantic type used for human-readable output provides a `show()` method.

That is the main API for rendering:

- debugging output
- table-style printing
- CSV serialization

This is simpler than trying to make every type participate in elaborate generic formatting machinery. The serialization code only needs a column registry plus per-column render functions.

## Design Boundaries

Some values are intentionally strict physical storage, not rich abstractions.

- `PopData` stores the simulation state
- `AgentView` provides convenient row-like access
- trait structs provide semantic element types and rendering
- derived predicates such as "tested" belong in query/filter logic, not in stored column layout

That boundary keeps printing and serialization honest: output should reflect what is physically stored.

## Performance Characteristics

| Operation | Complexity | Notes |
|---|---|---|
| access one attribute for one person | `O(1)` | direct vector index |
| update one attribute for one person | `O(1)` | direct vector index |
| sweep one column across all people | `O(n)` | contiguous memory access |
| build row-like access with `AgentView` | `O(1)` | lightweight handle only |
| serialize selected columns | `O(n * k)` | `k` selected columns |

The design optimizes for the common case: many repeated sweeps over some subset of columns.

## Future Work

Most of the previously listed "future enhancements" were either completed already or turned out not to be compelling.

The remaining realistic future work is modest:

- checkpoint/restart serialization if long simulations need resumable runs
- targeted performance tuning of specific kernels if profiling shows a real bottleneck
- better developer-side debugging helpers where they improve tracing without affecting hot paths

Ideas such as general indexing schemes, column compression, or broad SIMD treatment are possible in principle, but they are not obviously a good fit for this model as it currently mutates state. The simulation tends to branch heavily and update scattered values, which reduces the payoff of those approaches.

## References

- Struct of Arrays pattern: https://en.wikipedia.org/wiki/AoS_and_SoA
- Data-Oriented Design: https://www.dataorienteddesign.com/dodbook/
