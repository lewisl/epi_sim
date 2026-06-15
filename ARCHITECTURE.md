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
│  │  │ - ndays     │  │ - now_status│  │ - members   │                    │   │
│  │  │ - locale    │  │ - new_status│  │ - lengths   │                    │   │
│  │  │ - dovax     │  │ - now_vax   │  │             │                    │   │
│  │  │ - debug     │  │ - new_vax   │  │             │                    │   │
│  │  └─────────────┘  │ - now_var   │  └─────────────┘                    │   │
│  │                   │ - new_var   │                                     │   │
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
│  │                    (compute cumulative stats)                        │  │
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
- **Structure**:
  ```
  AllSeries
  ├── now_status (current state by status, age, ring, day)
  ├── new_status (new events by status, age, ring, day)
  ├── now_vax (current vaccination state)
  ├── new_vax (new vaccinations)
  ├── now_variant (current infections by variant)
  └── new_variant (new infections by variant)
  ```
- **Indexing**: 1-based for days, supports age buckets and ring aggregation

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
- `AllSeries` - Multi-dimensional time-series container
- `update()` - Record events with ring aggregation
- `finalize_series()` - Compute cumulative statistics
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
finalize_series() → cumulative statistics
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
- Multi-dimensional statistics (status × age × ring × day)
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
