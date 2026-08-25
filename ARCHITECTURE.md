# epi_sim Architecture Diagram

## System Overview

epi_sim is a C++23 epidemiological simulation framework for modeling disease spread, progression, vaccination, and intervention strategies.

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              CLI Entry Point                                 │
│                                 epi_sim.cpp                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │ --use-case   │  │ --use-dir    │  │ --init-case  │  │ --r0-sim     │    │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘    │
│         │                 │                 │                 │              │
│         └─────────────────┴─────────────────┴─────────────────┘              │
│                                   │                                          │
│                                   ▼                                          │
│                        ┌─────────────────────┐                               │
│                        │   setup_sim()       │                               │
│                        │   (setup.cpp)       │                               │
│                        └──────────┬──────────┘                               │
└───────────────────────────────────┼───────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Model Structure                                 │
│  ┌──────────────────────────────────────────────────────────────────────┐   │
│  │                           Model                                       │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │   │
│  │  │ ModelParams │  │  PopData    │  │  SeedCases  │  │  SD_Cases   │  │   │
│  │  │             │  │             │  │             │  │             │  │   │
│  │  │ - variants  │  │ - status[]  │  │ - filter    │  │ - quar      │  │   │
│  │  │ - infect    │  │ - agegrp[]  │  │ - change    │  │ - touch     │  │   │
│  │  │ - progress  │  │ - cond[]    │  │ - trigger   │  │ - trigger   │  │   │
│  │  │ - vaxset    │  │ - variant[] │  │             │  │             │  │   │
│  │  │ - social    │  │ - vax[]     │  │             │  │             │  │   │
│  │  │ - rings     │  │ - ring[]    │  │             │  │             │  │   │
│  │  └─────────────┘  │ - (20+ vecs)│  └─────────────┘  └─────────────┘  │   │
│  │                   └─────────────┘                                      │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                    │   │
│  │  │   Config    │  │  AllSeries  │  │  Ring Data  │                    │   │
│  │  │ - ndays     │  │ - col map   │  │ - members   │                    │   │
│  │  │ - locale    │  │ - 6 blocks  │  │ - lengths   │                    │   │
│  │  │ - dovax     │  │ - cols×days │  │             │                    │   │
│  │  │ - debug     │  │ - age/ring  │  │             │                    │   │
│  │  └─────────────┘  │   views     │  └─────────────┘                    │   │
│  │                   │             │                                     │   │
│  │                   └─────────────┘                                     │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Main Simulation Loop                                 │
│                              runsim()                                       │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                         Day Loop (1..ndays)                            │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │  │
│  │  │ Seed Cases   │  │   Social     │  │ Vaccination  │               │  │
│  │  │ (introduce   │  │  Distancing │  │ (if dovax)   │               │  │
│  │  │  infections) │  │  (quarantine)│  │              │               │  │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘               │  │
│  │         │                 │                 │                       │  │
│  │         └─────────────────┴─────────────────┘                       │  │
│  │                           │                                         │  │
│  │                           ▼                                         │  │
│  │  ┌──────────────────────────────────────────────────────────────┐  │  │
│  │  │              Person Loop (1..popn)                            │  │  │
│  │  │  ┌────────────────────────────────────────────────────────┐   │  │  │
│  │  │  │  For each INFECTIOUS person with sickday >= today:     │   │  │  │
│  │  │  │                                                        │   │  │  │
│  │  │  │  ┌──────────────────┐  ┌──────────────────┐           │   │  │  │
│  │  │  │  │   spread()      │  │  progression()   │           │   │  │  │
│  │  │  │  │  - find contacts│  │  - update cond   │           │   │  │  │
│  │  │  │  │  - calc risk    │  │  - update status │           │   │  │  │
│  │  │  │  │  - infect       │  │  - track days    │           │   │  │  │
│  │  │  │  │  - update series│  │  - update series │           │   │  │  │
│  │  │  │  └──────────────────┘  └──────────────────┘           │   │  │  │
│  │  │  └────────────────────────────────────────────────────────┘   │  │  │
│  │  └──────────────────────────────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                   │                                         │
│                                   ▼                                         │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                    finalize_series()                                 │  │
│  │                    (currently no-op)                                 │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                            Output Generation                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │ CSV Export   │  │ HTML Plots   │  │  Summary     │  │  Debug       │    │
│  │ (series.csv) │  │ (Plotly)     │  │  Statistics  │  │  Output      │    │
│  │ (pop.csv)    │  │              │  │              │  │              │    │
│  └──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Core Data Structures

### PopData (Population Data)
- **Purpose**: Stores all person-level attributes using 1-based indexing
- **Size**: `popz = popn + 1` (index 0 unused)
- **Key Vectors** (all size `popz`):
  - `status[]` - UNEXPOSED, INFECTIOUS, RECOVERED, DEAD
  - `agegrp[]` - Age groups (0-19, 20-39, 40-59, 60-79, 80+)
  - `cond[]` - Disease condition (nil, mild, sick, severe)
  - `duration[]` - Length of infection
  - `variant[]` - Current infecting variant
  - `variant_hist[]` - History of variants (reinfection tracking)
  - `sickday[]` - Day person became sick
  - `recovday[]` - Day person recovered
  - `deadday[]` - Day person died
  - `ring[]` - Ring membership (for targeted interventions)
  - `vaxstatus[]` - Vaccination status
  - `vax[]` - Vaccine type
  - `vaxday[]` - Day of vaccination
  - `quar[]` - Quarantine status

**AgentView**: Proxy object for accessing a single person's data without materializing a row
```cpp
auto person = pop.agent(i);  // Create view for person i
if (person.status() == INFECTIOUS) { ... }
```

### ModelParams (Simulation Parameters)
- **GeoData**: Geographic information (fips, county, population, density)
- **Variants**: Disease variant definitions (delta, omicron, etc.)
- **InfectParams**: Infection parameters per variant (sendrisk, recvrisk, immunity)
- **ProgressionSet**: Disease progression trees (variant → age → day → condition → outcomes)
- **SocialParams**: Contact matrices and social behavior parameters
- **VaxSet**: Vaccine parameters (efficacy, half-life, dosing)
- **VaxSchedSet**: Vaccination schedules and campaigns
- **RingTraits**: Ring definitions for targeted interventions

### AllSeries (Time-Series Statistics)
- **Purpose**: Accumulates daily statistics across multiple dimensions
- **Storage**: One private `std::vector<std::vector<std::int32_t>>`, where each
  outer element is a history column and each inner vector contains days
  `0..day_cnt` (day 0 unused)
- **Logical blocks**:
  ```
  now_status, new_status
  now_vax, new_vax
  now_variant, new_variant
  ```
- **Column order**: block → subject → ring → age bucket, with age bucket fastest
- **Indexing**: Subject IDs use their raw trait values; ring 0 is `RING_ALL`;
  age bucket 0 is `total`; days are 1-based
- **Selection**: String specifications are resolved after the run to numeric
  outer-column indices; they do not control collection

## Key Components

### setup.cpp/h - Model Initialization
- `setup_sim()` - Main entry for building Model from Config
- `setup_model_params()` - Load and validate all parameter files
- `build_caldays()` - Generate calendar day sequence
- `assign_rings()` - Assign people to intervention rings

### sim.cpp/h - Main Simulation Loop
- `runsim()` - Core simulation driver
  - Day loop with seed cases, social distancing, vaccination
  - Person loop for spread and progression
  - Series finalization and output generation

### spread.cpp/h - Disease Transmission
- `spread()` - Find contacts and transmit infection
- Uses contact matrices and touch factors
- Accounts for vaccination, recovery immunity, social distancing
- Updates series statistics for new infections

### progression.cpp/h - Disease Progression
- `progression()` - Update disease state for infected persons
- Uses progression trees (variant × age × day × condition)
- Transitions: nil → mild → sick → severe → recovered/dead
- Updates series statistics for state changes

### vaccination.cpp/h - Vaccination Logic
- `vaccinate()` - Apply vaccination schedules
- Handles multi-dose regimens
- Tracks vaccine effectiveness over time
- Updates series statistics

### disease_modeling.cpp/h - Disease Modeling Calculations
- `infectrisk()` - Calculate infection probability
- `recoveffect()` - Recovery immunity decay
- `vaxeffect()` - Vaccine effectiveness decay
- Decay functions: linear, exponential, sigmoidal

### series.cpp/h - Statistics Collection
- `SeriesColumnMap` - Block bases, subject/ring strides, and stock/flow metadata
- `AllSeries` - Dense column table with typed numeric access
- `update()` - Record one event in its ring/age cell and both aggregates
- `init_history_series()` - Carry stock-block columns into the next day
- `finalize_series()` - Intentionally empty; no post-run rollup is required
- `resolve_selected_series()` - Convert output selectors to canonical columns
- Serialization and plotting support

### plot.cpp/h - Visualization
- `seriesplot()` - Generate interactive HTML plots using Plotly
- Supports multiple series, age groups, and ring-specific views

## Data Flow

```
Parameter Files (JSON/CSV)
    │
    ▼
setup_model_params() → ModelParams
    │
    ▼
PopData(popn) → Population initialization
    │
    ▼
Model = {ModelParams, PopData, Config, ...}
    │
    ▼
runsim(Model)
    │
    ├── Day Loop:
    │   ├── Seed Cases → introduce infections
    │   ├── Social Distancing → modify contact rates
    │   ├── Vaccination → update vax status
    │   └── Person Loop:
    │       ├── spread() → new infections → AllSeries
    │       └── progression() → state changes → AllSeries
    │
    ▼
finalize_series() → no-op (aggregates already maintained or resolved on read)
    │
    ▼
Output: CSV files, HTML plots, summary statistics
```

## Key Design Patterns

### 1-Based Indexing
- All PopData vectors use 1-based indexing (persons 1..popn)
- Matches epidemiological conventions
- Index 0 unused/reserved for sentinel values
- Critical for correctness when accessing person data

### AgentView Pattern
- Lightweight proxy for person data access
- No row materialization - direct vector access
- Enables clean agent-oriented function signatures
- Pass by value (cheap - contains reference + index)

### Series Aggregation
- Six parallel status/vaccine/variant blocks; these traits are not a Cartesian product
- Dense subject × ring × age-bucket columns within each block, followed by day values
- Ring-specific + aggregate (RING_ALL) tracking
- Efficient update with mirror-writes for aggregates
- Supports flexible querying and visualization

### Parameter Hierarchy
```
ProgressionSet
  └─ Progression (per variant)
      └─ tree (per age group)
          └─ breakday (day since infection)
              └─ condition (current state)
                  └─ transition_vector (probabilities to next states)
```

## Build System
- **Build Tool**: xmake
- **Language**: C++23
- **Compiler**: LLVM/Clang
- **Package Manager**: vcpkg
- **Key Dependencies**: fmt, absl, nlohmann/json

## Testing
- Test framework in `test/` directory
- Test groups: disease_modeling, parameters, series, setup, vaccination, etc.
- Run with: `xmake run test` or `xmake run test <group>`

## File Organization
```
src/
├── epi_sim.cpp          # CLI entry point
├── setup.cpp/h          # Model initialization
├── sim.cpp/h            # Main simulation loop
├── spread.cpp/h         # Disease transmission
├── progression.cpp/h    # Disease progression
├── vaccination.cpp/h    # Vaccination logic
├── disease_modeling.cpp/h # Disease calculations
├── series.cpp/h         # Statistics collection
├── plot.cpp/h           # Visualization
├── population.h         # PopData and AgentView
├── parameters.cpp/h     # Parameter structures
├── traits.h             # Enum definitions
├── random.h             # Random number generation
└── helpers.cpp/h        # Utility functions

test/
├── test_*.cpp           # Test files
└── fixtures/            # Test data

sample_parameters/       # Example parameter files
design/                  # Design documentation
docs/                    # User documentation
```
