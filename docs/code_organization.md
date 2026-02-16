
## Separation of Concerns:

### population.h/cpp - Data structure operations

Constructors, basic getters/setters
Printing, data access utilities
Pure data manipulation


### disease_modeling.h/cpp - Core disease logic (shared algorithms)

Risk calculations based on vaccination history
Immunity decay functions
Probability calculations for outcomes
PopData methods that encapsulate disease state changes (like make_sick())
Uses disease parameters extensively


### spread.cpp - Infection transmission accounting

Who contacts whom
Transmission probability checks
Calls disease_modeling functions to determine outcomes
Updates PopData with new infections
Will be long due to contact tracing, exposure accounting


### progression.cpp - Disease progression accounting

Daily updates for infected people
Calls disease_modeling functions to determine if condition worsens/improves
Recovery/death accounting
Updates PopData based on progression outcomes


### sim.cpp - Main simulation loop

Day-by-day orchestration
Calls spread, progression, vaccination, etc.
High-level flow control

### summary
Why This Works Well
Shared disease logic in disease_modeling.cpp:

Both spread.cpp and progression.cpp can call the same risk calculation functions
Disease parameters stay centralized
PopData methods that change disease state (make_sick(), make_recovered(), etc.) live where the disease logic lives
Avoids code duplication