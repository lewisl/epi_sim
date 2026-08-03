# Spread Runtime Optimization Opportunities

## Goal

Reduce runtime in the `spread` hot path while keeping the implementation clear,
maintainable, and epidemiologically equivalent. The project already builds with
`set_optimize("fastest")` and target-scoped ThinLTO, so this is a data-layout and
repeated-work problem rather than a compiler-flag problem.

The current contact buffer retains capacity and therefore does not normally
allocate inside `spread()`. Optimizations should be measured against a fixed-seed,
representative run rather than justified solely from source inspection.

## Recommended Order

1. Measure with production timing overhead disabled or sampled.
2. Precompute daily contact-scale and touch-probability tables.
3. Cache spreader risk data once per `spread()` call.
4. Convert vaccine factors from string searches to numeric indexed tables.
5. Precompute recovery and vaccine decay curves.
6. Flatten ring membership if ring-enabled runtime matters.
7. Consider fixed buffers and unchecked agent access only after profiling.
8. Change gamma or bounded-integer RNG implementations only as a final step.

## 1. Make Fine-Grained Timing Optional or Sampled

`runsim()` currently calls `steady_clock::now()` around every `spread()` and
`progression()` invocation. With many infectious-person-days, this instrumentation
can contribute measurable total runtime.

Reasonable approaches:

- Enable per-call timers only through a performance/debug option.
- Sample one call in every N calls and scale the estimate.
- Use a sampling profiler such as Instruments for detailed attribution.

For temporary profiling, distinguish these spread stages:

- Gamma contact-count generation.
- Contact-ID generation.
- Ring ordinal mapping.
- Eligibility and touch screening.
- Infection-risk calculation.
- Successful infection state/series updates.

Avoid leaving multiple clock reads inside the permanent contact loop.

## 2. Precompute a Daily Spread Context

`indoor_factor` is constant for a simulation day, and social-distancing factors
come from a small set of matrices. Build compact lookup tables once per day:

```text
contact_scale[sd_case][current_condition][spreader_age]
touch_probability[sd_case][unexposed_or_recovered][contact_age]
```

The contact-scale table combines:

```text
density_factor * indoor_factor * contact_factor
```

The touch table combines and clamps:

```text
touch_factor * indoor_factor
```

This removes repeated work from `spread()`:

- Matrix-selection boilerplate.
- `touch_map()` calls for contacts already known to be unexposed or recovered.
- Repeated multiplication by density and indoor factors.
- Per-contact probability clamping.

A small `SpreadDayContext` value would make these precomputed inputs explicit
without adding complex abstraction inside the hot loop. Allocate any dynamically
sized social-distancing table once and overwrite it each day rather than allocating
per spreader.

## 3. Cache Spreader-Invariant Risk Data

Build a small immutable context once at the start of each `spread()` call:

```text
variant
duration
sendrisk
reference to the variant's InfectParams
age
condition
```

Currently `isinfected()` retrieves the spreader's variant and duration for every
touched contact, and `infectrisk()` retrieves the same `sendrisk` again. The
infection check should consume the cached values instead.

In particular, calculate once:

```text
const InfectParams& variant_params
const float sendrisk
```

Then each infection candidate only needs the contact's age, immunity factors,
and `variant_params.recvrisk[age]`.

This is a behavior-preserving optimization and should remain straightforward if
implemented as a small POD context or as explicit primitive parameters. ThinLTO
may remove function-call overhead, but it cannot make logically repeated loads
unnecessary unless the aliasing is obvious.

## 4. Compile Vaccine Parameters into Numeric Tables

This is likely the largest maintainable opportunity for vaccinated simulations.
`vaxeffect()` currently performs string work for infection candidates:

- Copies the target variant name.
- Resolves the vaccination-status name.
- Linearly searches `infectfactor`.
- Linearly searches the status-specific effectiveness collection.
- Linearly searches variants again within that collection.

Resolve names during JSON loading and store direct indexed data:

```text
infectfactor[variant]
effectiveness[vax_status][variant]
```

The hot path then indexes with the compact existing `Variant` and `Vaxstatus`
values. Missing names and dimensions should be validated during loading, as with
the packed progression parameters.

This should improve both clarity and performance by keeping configuration-name
resolution out of simulation kernels.

## 5. Precompute Immunity Decay Curves

`recoveffect()` and `vaxeffect()` call `sigdecay()`, which evaluates `exp()`, for
eligible infection candidates. These functions depend on a small collection of
parameter sets and an integer number of elapsed days.

Precompute their time-dependent portions when the model duration is known:

```text
recovery_curve[previous_variant][days_since_recovery]
vaccine_curve[vaccine][days_since_vaccination]
```

The remaining hot-path calculations are indexed effectiveness values and a few
multiplications. A more fully compiled table could also include target variant
and vaccination status if measurement justifies the extra storage.

Keep `effect_rise()` and `sigdecay()` as the source formulas used to build the
tables. Tests should compare table entries against direct formula evaluation at
boundary and representative days.

This optimization matters most late in an epidemic, when many eligible contacts
are recovered or vaccinated.

## 6. Flatten Ring Membership

With rings enabled, every out-of-ring contact currently maps an ordinal to a
person by scanning rings and subtracting their lengths.

Store all ring members in one contiguous array, with each ring's start and length:

```text
all_ring_members
ring_start[ring]
ring_length[ring]
```

An ordinal from the population excluding the source ring can then skip that
ring's contiguous segment directly:

```text
if ordinal >= ring_start[source_ring]:
    ordinal += ring_length[source_ring]
person = all_ring_members[ordinal]
```

This replaces a per-contact ring scan with constant-time indexing without
replicating the population for every source ring. Same-ring sampling can use the
source ring's contiguous span.

## 7. Simplify Eligibility and Touch Screening

Use an early-continue structure in the contact loop:

```text
if status is neither unexposed nor recovered:
    continue
```

After this check, the touch row is directly:

```text
unexposed -> 0
recovered -> 1
```

This makes the following current work unnecessary:

- Calling `touch_map()` for these two known statuses.
- Maintaining a zero-initialized `touchprob` through multiple branches.
- Assigning `touchprob = baseprob` when `indoor_factor == 1.0f`; that assignment
  is immediately overwritten by the subsequent multiply-and-clamp assignment.

Clang may already eliminate some dead work, so this is primarily a clarity
improvement unless profiling shows otherwise.

## 8. Consider a Fixed Contact Buffer

Contact count is currently capped at 12 by `gamma_int(..., 12)`. A reusable:

```text
array<size_t, MAX_CONTACTS> + active count
```

would remove vector `push_back` capacity checks and make the relationship to the
contact cap explicit.

The existing vector is cleared but retains capacity, so this is not an allocation
fix and is expected to produce only a modest improvement. Keep the cap in one
shared constant if this change is made.

Fusing random contact generation directly into contact processing could remove
the buffer entirely, but it would complicate ring handling and make deterministic
RNG behavior harder to preserve. A fixed buffer is the more maintainable option.

## 9. Consider Validated Unchecked Agent Access

Contact IDs generated by global and ring sampling are already intended to be in
`1..popn`, but `pop.agent(c)` validates the range for every contact.

If profiling shows this check matters, provide a deliberately named internal
unchecked operation with a debug assertion. Do not remove validation from public
or general-purpose population access.

An alternative is to read `status`, `agegrp`, and `sdcase` directly from the
SoA vectors for eligibility screening and construct `AgentView` only after a
successful touch. That may be faster, but mixing access styles should be weighed
against readability.

## 10. RNG Changes: Deferred

Potentially expensive operations include:

- `std::gamma_distribution` for every spreader.
- `std::uniform_int_distribution` for contact IDs.
- `std::binomial_distribution` with rings enabled.

Do not replace these first. Custom RNG mapping and approximated/precomputed gamma
distributions carry statistical and reproducibility risks.

If profiling proves gamma generation dominates, investigate a precomputed
categorical distribution over contact counts `0..12` for the finite set of
contact scales. Such a change requires statistical validation and may intentionally
change fixed-seed trajectories.

## Optimizations Not Recommended Initially

- Parallelizing spread: contacts mutate shared population and series state, and
  deterministic RNG ordering would change.
- SIMD contact processing: random contact indices cause scattered SoA access and
  divergent status/protection branches.
- Large per-person/per-variant lazy caches: they add invalidation and memory
  complexity before simpler parameter/curve tables are exhausted.
- Broad helper abstraction inside the contact loop: prefer small precomputed
  contexts and direct indexed access.

## Validation and Benchmarking

For behavior-preserving changes:

- Use identical input and RNG seed.
- Require identical daily status, variant, recovery, and death series.
- Compare total runtime and spread runtime over several runs, not one run.
- Record configuration: population, days, vaccination, rings, social distancing,
  and compiler revision.

Add focused tests for:

- Zero and maximum contact counts.
- Unexposed versus recovered contacts.
- Vaccinated and unvaccinated contacts across variants/statuses.
- Indoor-factor and social-distancing lookup equivalence.
- Ring probabilities of 0 and 1.
- Preservation of one-based population indexing and unused index 0.

If RNG implementation changes, fixed-seed identity is no longer an appropriate
criterion. Use repeated-run distribution checks for contact counts and aggregate
simulation outcomes instead.

## Suggested Implementation Stages

### Stage A: Low-risk universal improvements

- Make fine-grained timing optional or sampled.
- Introduce the daily lookup context.
- Cache spreader risk values.
- Simplify eligibility screening.

### Stage B: Parameter compilation

- Convert vaccine factors to indexed tables.
- Precompute recovery and vaccine curves.

### Stage C: Configuration-specific structures

- Flatten ring membership.
- Replace the dynamic contact buffer if measurement supports it.
- Add validated unchecked internal access only if it remains visible in profiles.

### Stage D: Statistical/RNG investigation

- Profile gamma, uniform, and binomial sampling.
- Change RNG algorithms only with explicit statistical and reproducibility review.
