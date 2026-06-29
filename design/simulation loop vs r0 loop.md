The important difference: `runsim` is the real simulation loop; `r0_simulation` is a constrained reproduction-number mini-loop with its own copied spread kernel.

**Daily Loop**
`runsim` in [src/sim.cpp](/Users/lewislevin/code/epi_sim/src/sim.cpp:85) runs days `1..model.ndays`. Each day it:

- increments global sim day
- initializes day series
- applies start-of-day seed cases
- applies social-distancing cases
- runs vaccination
- scans every person `1..pop.popn`
- for infectious eligible people, calls real `spread(...)`
- then calls `progression(...)` for that same infectious person

`r0_simulation` in [src/r0_simulation.cpp](/Users/lewislevin/code/epi_sim/src/r0_simulation.cpp:195) runs days `1..DURATIONLIM`, not `model.ndays`. It resets/restores global sim day around the mini-run, seeds a synthetic generation-1 spreader set first, then each day:

- loops only over `gen1_spreaders`
- counts infections caused by those spreaders
- loops over `gen1_spreaders` again for progression

**Per-Spreader / Spread Loop**
`runsim` calls the production spread kernel at [src/sim.cpp](/Users/lewislevin/code/epi_sim/src/sim.cpp:134). That resolves to [src/spread.cpp](/Users/lewislevin/code/epi_sim/src/spread.cpp:7). This kernel:

- uses spreader SD case to choose `contactfactors`
- uses contacted-person SD case to choose `touchfactors`
- uses ring-aware contact selection when rings are enabled
- uses `model.indoor_seq`
- calls `contact.make_sick(...)` on successful infection, mutating population state and series

`r0_simulation` does not call `spread(...)`. Its mini-loop calls local `spread_and_count(...)` at [src/r0_simulation.cpp](/Users/lewislevin/code/epi_sim/src/r0_simulation.cpp:209), defined at [src/r0_simulation.cpp](/Users/lewislevin/code/epi_sim/src/r0_simulation.cpp:116). That copied kernel:

- always uses base `social.contactfactors`
- always uses base `social.touchfactors`
- has no social-distancing override path
- has no ring-aware contact selection
- uses synthetic `indoor_seq(DURATIONLIM, 1.0f)`
- counts successful infections but deliberately does not call `make_sick(...)`

So the R0 mini-loop is intentionally “generation-1 spreaders only, count-only, abstract context.” The normal `runsim` loop is “all currently infectious eligible people, mutate real population, with SD/rings/vax/seasonality context.”