# Codebase Review

Date: 2026-04-03

## Summary

`epi_sim` is a batch-oriented epidemiological simulator with a clear core split:

- model setup and parameter loading
- mutable per-person execution state
- day-by-day simulation kernels
- persisted run outputs for later analysis

## Structure

### Setup and model assembly

[setup.cpp](/Users/lewislevin/code/epi_sim/src/setup.cpp) and [parameters.cpp](/Users/lewislevin/code/epi_sim/src/parameters.cpp) are responsible for building the simulation model from config and parameter files.

This layer loads:

- geodata
- variants and infection parameters
- progression trees
- social contact/touch parameters
- vaccine definitions and schedules
- the initial population
- calendar and indoor-seasonality sequences

`Model` in [setup.h](/Users/lewislevin/code/epi_sim/src/setup.h) now reads as a setup/runtime container for a run, not as a mixed setup-plus-output object.

### Execution state

[population.h](/Users/lewislevin/code/epi_sim/src/population.h) defines `PopData` as a structure-of-arrays with strict 1-based indexing. That is the central mutable state of the simulation.

`PopData::AgentView` is a lightweight proxy over one person. This is a good fit for the codebase because it preserves the performance advantages of vectorized storage while still allowing person-oriented helper functions.

### Simulation loop

[sim.cpp](/Users/lewislevin/code/epi_sim/src/sim.cpp) owns the main day loop. It:

- initializes `HistorySeries`
- applies seed cases
- performs vaccination
- runs spread
- runs progression
- finalizes history output

That separation is sensible. `HistorySeries` is run output, not model definition, so keeping it outside `Model` is the better design.

### Domain kernels

The disease logic is split across focused modules:

- [disease_modeling.cpp](/Users/lewislevin/code/epi_sim/src/disease_modeling.cpp): shared disease and immunity calculations
- [spread.cpp](/Users/lewislevin/code/epi_sim/src/spread.cpp): contact and transmission
- [progression.cpp](/Users/lewislevin/code/epi_sim/src/progression.cpp): within-host disease progression
- [vaccination.cpp](/Users/lewislevin/code/epi_sim/src/vaccination.cpp): daily vaccine administration

This is a reasonable decomposition and matches the stated intent in [code_organization.md](/Users/lewislevin/code/epi_sim/docs/code_organization.md).

### Output and analysis

[series.cpp](/Users/lewislevin/code/epi_sim/src/series.cpp) and [plot.cpp](/Users/lewislevin/code/epi_sim/src/plot.cpp) handle persisted outputs.

This is the right direction for this codebase. In C++, unlike Julia, there is no natural REPL-driven post-stop exploration of live simulation variables. The meaningful analysis surface is what gets written to disk.

## Key Capabilities

The current codebase already supports:

- config-driven simulation startup
- locale-specific population sizing and seasonal indoor uplift
- variant-specific spread and severity
- vaccination schedules and vaccine effect decay
- prior-infection immunity effects
- declarative seeding via filter/change rules
- age-bucketed history series
- persisted CSV/HTML output
- local-browser Plotly visualization
- agent-level inspection for debugging

## Architectural Judgments

### Good decisions

- Keeping `HistorySeries` out of `Model` is the correct design choice.
- Treating `PopData` as transient execution state is correct.
- Persisting outputs to disk instead of trying to force interactivity into a CLI C++ program is pragmatic.
- Using Plotly in a local browser is a strong output strategy for this kind of simulator.

### Why output does not belong in `Model`

`Model` defines what is needed to run the simulation. `HistorySeries` captures what happened during a specific run.

Those are different responsibilities. Folding outputs back into `Model` would make the object heavier and blur setup versus results. It would also move the design in a more Julia-like direction without Julia’s interactive advantages.

## Remaining Improvement Areas

These are the main issues still visible after the recent cleanup.

### 1. Tests are behind the current API

[test.cpp](/Users/lewislevin/code/epi_sim/test/test.cpp) is not aligned with the current interfaces. This is now the clearest maintenance weakness in the repository.

### 2. Windows support for browser launch is incomplete

The plot format itself is sound. The portability issue is the platform-specific browser-launch path, not the use of Plotly or HTML output.

### 3. Some debug and under-construction residue remains

[sim.cpp](/Users/lewislevin/code/epi_sim/src/sim.cpp) and [setup.cpp](/Users/lewislevin/code/epi_sim/src/setup.cpp) still contain comments and small pieces of transitional scaffolding that should eventually be cleaned up.

### 4. The larger strategic task is model generalization

The most important long-term work is not interactivity. It is reducing implicit COVID-specific assumptions and making the model easier to generalize.

Given the current performance of the C++ implementation, repeated short runs are already feasible and are a practical substitute for much of the exploratory workflow that would otherwise require richer interactivity.

## Overall Assessment

The project has a solid core architecture:

- vectorized population state
- explicit setup pipeline
- reasonably separated simulation kernels
- persisted outputs for analysis

The codebase is now cleaner than it was at the start of this review. The recent fixes removed actual design debt, not just cosmetic issues.

The next priorities should be:

- bring tests back in sync
- continue small cleanup of transitional code
- improve cross-platform browser launching
- focus design effort on generalizing the disease model rather than adding interactive control surfaces
