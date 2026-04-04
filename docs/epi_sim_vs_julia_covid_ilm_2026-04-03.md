# epi_sim Compared to the Earlier Julia Covid-ILM Model

Date: 2026-04-03

## Purpose

This note compares the current C++ `epi_sim` codebase to the earlier Julia model in:

- `/Users/lewislevin/code/Covid Modeling/Covid-ILM`

The point is to identify continuity, important differences, and what the rewrite changed beyond raw speed.

## Summary

The C++ project is not just a faster port of the Julia model.

It preserves the core simulation ideas from the Julia codebase:

- seeded introductions
- spreaders, contacts, and consequential contact
- progression through disease states
- age-structured outcomes
- vaccination and prior-infection effects
- policy/intervention cases

But it also makes several architectural changes:

- simpler and more explicit core state
- clearer mutation boundaries
- less dependence on interactive notebook workflows
- stronger fit for fast repeated batch runs

The rewrite also exposed some performance issues and a small number of correctness bugs that changed output slightly. That matters when comparing past and current runs: not every difference should be read as a modeling change.

## What carried forward from Julia

The basic simulation logic is continuous across both codebases.

The Julia repo already had the main ingredients that still define the model:

- `PopData`-like person-level state
- seeded run cases
- social-distancing cases
- spread logic based on contacts, touches, and infection risk
- progression trees and checkpoints
- vaccination support
- history series
- plotting and report generation

This is clear from:

- [CovidSim_ilm.jl](/Users/lewislevin/code/Covid%20Modeling/Covid-ILM/src/CovidSim_ilm.jl)
- [setup.jl](/Users/lewislevin/code/Covid%20Modeling/Covid-ILM/src/setup.jl)
- [sim.jl](/Users/lewislevin/code/Covid%20Modeling/Covid-ILM/src/sim.jl)
- [spread.jl](/Users/lewislevin/code/Covid%20Modeling/Covid-ILM/src/spread.jl)
- [progression.jl](/Users/lewislevin/code/Covid%20Modeling/Covid-ILM/src/progression.jl)

So the C++ model should be understood as a continuation and tightening of that design, not as a replacement with a different modeling worldview.

## Key differences

### 1. Batch execution versus notebook interactivity

The Julia model was built in a notebook-friendly world.

That shows up in several ways:

- `runsim()` returned live structures for immediate inspection
- plots were created inline
- reports were tightly connected to notebook/script runs
- model state, outputs, and experimentation lived in one session

The C++ version is built around a different workflow:

- run the simulator
- persist outputs
- inspect plots and serialized results afterward
- rerun quickly with changed inputs

This is not just a language constraint. It is now a design choice supported by performance.

### 2. Model object shape

The Julia `model` built in [setup.jl](/Users/lewislevin/code/Covid%20Modeling/Covid-ILM/src/setup.jl) included both:

- mutable population data
- history/output series

That fit the Julia workflow.

The current C++ direction is cleaner. `Model` defines what is needed to execute a run, while `HistorySeries` is treated as run output rather than part of model definition.

That separation is a real improvement.

### 3. State representation

The Julia model relied on:

- `LazyTable`
- symbols for trait values
- named tuples and dictionaries
- callback-style cases

The C++ model relies on:

- explicit structs
- enum-like trait wrappers
- vectorized state containers
- `AgentView` as a lightweight proxy

The C++ version is less flexible at the language level, but it is easier to reason about as an execution engine and easier to optimize carefully.

### 4. Instrumentation

The Julia model had richer in-process instrumentation:

- spread debug traces
- runtime traces
- R0 simulation hooks
- midstream reporting and interactive plots

The C++ version currently has less of that, and some earlier tracing ideas have now been intentionally removed or deferred for redesign.

That is a loss in exploratory convenience, but not necessarily a loss in design quality. Some of the old instrumentation had become stale or too tied to the earlier implementation.

### 5. Scope of interventions

The Julia repo still contains a broader intervention surface area:

- `test_and_trace.jl`
- `travel.jl`
- `quarantine.jl`
- richer social-distancing case machinery

The C++ codebase is currently more focused on getting the core engine right and making the main simulation path fast and clear.

That means the Julia model still shows more intervention breadth, while the C++ version shows more architectural discipline in the core.

## Important continuity in future goals

One of the most useful findings from the Julia repo is that several of the current C++ roadmap goals were already visible there.

The TODOs in [CovidSim_ilm.jl](/Users/lewislevin/code/Covid%20Modeling/Covid-ILM/src/CovidSim_ilm.jl) explicitly mention:

- rings
- richer filter terms
- quarantine redesign
- travel updates
- optional condition-dependent send risk
- cleaning out older seeding approaches

That means the current direction is not a new invention. It is a continuation of concerns already identified in the Julia phase.

## Performance and output differences

The C++ implementation is much faster than the Julia model, but speed is not the only difference.

During the rewrite and later cleanup, some performance issues and a couple of bugs were found that slightly changed outputs. This is important context:

- some differences between Julia-era and C++-era results reflect corrected behavior
- some differences reflect improved implementation rather than changed disease assumptions
- exact output parity should not be treated as the only measure of success

That said, this is a strong reason to keep improving tests and comparison cases so future changes are easier to interpret.

## What the Julia repo still contributes

The older Julia codebase remains useful in three ways.

### 1. It preserves modeling intent

The Julia repo makes the original conceptual structure and the policy use cases very visible.

### 2. It preserves explanatory tooling ideas

The older reports and spread/touch instrumentation were good at explaining what the model was doing, even if that implementation should not be copied directly.

### 3. It shows unfinished generalization work

The Julia code and TODOs make it clear that the long-term direction was already shifting away from a purely COVID-specific model and toward a more general simulation framework.

## Overall judgment

The C++ rewrite is an improvement in architectural clarity and runtime performance.

The Julia model was better suited to exploratory notebook-based analysis and had broader experimental intervention machinery.

The C++ model is better suited to:

- repeated fast runs
- clearer execution logic
- explicit state ownership
- eventual generalization into a more disciplined engine

The next challenge is not recreating Julia interactivity.

It is carrying forward the best ideas from the Julia model while continuing to remove:

- implicit COVID assumptions
- overly ad hoc tracing/debugging mechanisms
- residual implementation quirks that obscure the model itself

## Bottom line

The Julia codebase is best understood as the conceptual and experimental predecessor.

The C++ codebase is the tighter, faster, more execution-oriented descendant.

The most important work ahead is to generalize the model structure, not to recreate the Julia development environment in C++.
