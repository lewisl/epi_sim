# epi_sim Compared to Covasim and EMOD

Date: 2026-04-03

## Purpose

This note compares `epi_sim` to two public IDM-related modeling codebases:

- `Covasim`: a COVID-focused agent-based simulator
- `EMOD`: a broader agent-based disease modeling platform
- `Starsim`: a newer general-purpose multi-disease framework

The point is not to copy either one. The point is to identify what they suggest about the next steps for `epi_sim`, especially around generalization.

## Why these two comparisons

`Covasim` is the better comparison for disease content and intervention semantics.

`EMOD` is the better comparison for architecture, extensibility, and long-term direction.

`Starsim` is worth attention because it appears to sit closest to the abstraction layer `epi_sim` is now moving toward.

## Quick positioning

### epi_sim

Current strengths:

- lean C++ implementation
- very fast batch reruns
- simple mental model
- explicit setup pipeline
- vectorized population storage with `AgentView`
- persisted outputs for post-run analysis

Current limitations:

- still carries implicit COVID-era assumptions
- contact selection is still mostly randomized rather than structured
- trait richness is limited
- tests are behind current APIs
- engine and disease assumptions are not yet separated as cleanly as they could be

### Covasim

From the public repository, Covasim is described as a stochastic agent-based COVID simulator used to explore infections, hospital demand, and interventions including distancing, school closures, testing, tracing, quarantine, and vaccination.

Covasim is useful as an example of:

- mature outbreak/intervention semantics
- user-facing analysis workflows
- scenario and repeated-run usability

But it is explicitly COVID-oriented. The repository also notes that it is no longer actively maintained and points users to Starsim for current IDM ABM work.

### EMOD

From the public repository, EMOD is described as a stochastic agent-based disease modeling platform in which agents can carry multiple properties and interact through decision rules. The repo structure also shows a much heavier platform shape:

- interventions
- reporters
- regression assets
- component tests
- multiple support libraries
- disease-specific packages and scripts

EMOD is useful as an example of:

- generalized ABM architecture
- richer agent-property systems
- repeated stochastic run workflows
- larger reporting and intervention frameworks

It is much closer to the direction of a general disease engine.

### Starsim

From the public docs and repository, Starsim is presented as a general-purpose, multi-disease framework built around dynamic transmission networks. It explicitly supports:

- multiple diseases in one simulation
- non-infectious conditions that affect infectious outcomes
- multiple network types
- interventions as modules
- analyzers and results
- connectors that mediate interactions between modules

Its public structure is also directly relevant to your current goals. The docs describe first-class `Sim`, `People`, `Disease`, `Network`, `Intervention`, `results`, and `analyzers` concepts, plus a `run` layer for parallel execution.

For your roadmap, Starsim is probably the most directly relevant external reference because it tackles:

- trait-rich people
- explicit network structure
- modular disease logic
- disease interaction and non-disease covariates
- repeated-run analysis workflows

## Comparison by design dimension

### 1. Disease specificity

`epi_sim` currently sits between the two.

It is less explicitly COVID-only than `Covasim` in some implementation choices, but it still has modeling assumptions that were reasonable for COVID and may not hold for measles or other sharper-transmission diseases.

Examples of where generalization pressure will land:

- age bucket design
- contact selection assumptions
- progression assumptions
- reinfection semantics
- vaccine and recovery effect structure

For this dimension, `EMOD` is the more relevant long-term reference.

### 2. Trait system

Your stated goal of adding finer-grained age buckets and one or more comorbidity columns pushes `epi_sim` toward a richer declarative trait system.

That is not mainly about adding more columns. It is about making those columns first-class drivers of:

- contact selection
- transmission risk
- progression risk
- intervention targeting
- output stratification

`EMOD` is the stronger large-platform reference here because its public description is built around agents with multiple properties and decision-rule-driven interactions.

`Starsim` is also highly relevant here because it explicitly supports non-infectious conditions that can affect infectious diseases, which is very close to your comorbidity goal.

### 3. Contact structure

This is the most important modeling gap relative to your stated goals.

Your current random-contact approach was serviceable for COVID-like spread, especially with high transmissibility and substantial latency. It becomes less adequate for diseases where transmission is more clustered, more explosive, or more dependent on who meets whom.

Your proposed "rings" idea is therefore important. It suggests movement toward:

- household or family clusters
- school/work/community layers
- trait-aware contact pools
- more directed local transmission structure

This is one of the clearest places where `epi_sim` should evolve beyond the assumptions that were acceptable for COVID.

On this dimension, Starsim is especially worth studying because its public design is built around dynamic transmission networks rather than only random contact draws.

### 4. Interactivity versus rerun speed

`Covasim` benefits from Python ergonomics and a more interactive analysis culture.

`epi_sim` has taken a different but valid path: the implementation is fast enough that repeated runs with changed inputs are practical. In C++, this matters. Since there is no natural REPL workflow after `main()` exits, persisted artifacts and fast reruns are a better fit than trying to force notebook-style interactivity into a CLI simulator.

That means speed is not just a performance win. It is part of the usability story.

### 5. Output philosophy

`epi_sim` is right to treat `HistorySeries` as output rather than model definition.

That decision fits the current architecture and the batch-oriented workflow. It also keeps `Model` from becoming a mixed setup-plus-results object.

This is a sensible place to differ from more interactive Python modeling workflows.

### 6. Software scale

`EMOD` is much larger and more infrastructural than `epi_sim`.

That is both a strength and a warning.

What to learn from it:

- clear separation between platform concerns and disease content
- explicit reporting infrastructure
- regression and repeated-run discipline
- richer intervention abstractions

What not to copy blindly:

- platform weight
- complexity overhead
- over-engineering before the model abstractions are stable

`Starsim` sits in between `Covasim` and `EMOD` here. It is more framework-oriented than a single disease model, but much lighter and more legible than a large mature platform like `EMOD`.

## What this implies for epi_sim

The right direction is not "become Covasim in C++".

The better direction is:

1. Keep the current fast batch-oriented workflow.
2. Continue using persisted outputs rather than trying to build REPL-like interactivity.
3. Generalize the model schema so disease content is less implicit.
4. Improve contact structure, especially with ring-based or layered selection.
5. Expand trait handling so age buckets and comorbidities can drive transmission and progression declaratively.
6. Strengthen test and regression support as the abstractions become more general.

If one external project is most worth reading next, it is probably Starsim, because it is closest to your stated direction while still being easier to digest than EMOD.

## Practical roadmap interpretation

If choosing what to study from each project:

Study `Covasim` for:

- intervention vocabulary
- scenario handling
- user-facing analysis workflow

Study `EMOD` for:

- agent property design
- generalized model architecture
- separation of engine from disease-specific logic
- reporters and repeated-run support

Study `Starsim` for:

- dynamic network design
- module and connector boundaries
- how non-infectious traits can affect infectious processes
- analyzers/results as first-class concepts
- how to generalize without immediately becoming as heavy as EMOD

## Bottom line

`Covasim` shows what a mature disease-specific simulation environment looks like.

`EMOD` shows what a broader disease-modeling platform looks like.

`Starsim` shows a more modern framework-level attempt to bridge those two worlds.

Your stated goals place `epi_sim` on a path much closer to `EMOD` and Starsim in direction than to Covasim, while still benefiting from the simplicity and speed of a smaller C++ codebase.

The most important next modeling step is not interactivity.

It is replacing COVID-shaped assumptions with more general representations of:

- trait structure
- contact structure
- disease progression
- intervention targeting

## Sources

- Covasim repository: https://github.com/InstituteforDiseaseModeling/covasim
- EMOD repository: https://github.com/EMOD-Hub/EMOD
- Starsim repository: https://github.com/starsimhub/starsim
- Starsim docs: https://docs.starsim.org/
- Starsim site: https://starsim.org/
