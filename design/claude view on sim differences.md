Structural framing

runsim advances a real, mutating epidemic over model.ndays and emits series/CSV/plots. The r0 loop runs a non-mutating, count-only experiment over a gen-1 cohort and returns one scalar r0_infected / gen1_spreader_cnt. The cleanest way to see it: the r0 loop is what R0 means operationally — let a fixed seed cohort transmit for their full infectious window into a population that never changes, and average the transmission events per seed.

Day loop

┌────────────────────┬─────────────────────────────────────────┬──────────────────────────────────────────────────┐
│       Aspect       │           runsim (sim.cpp:86)           │       r0 mini-loop (r0_simulation.cpp:195)       │
├────────────────────┼─────────────────────────────────────────┼──────────────────────────────────────────────────┤
│ Bound              │ 1..model.ndays                          │ 1..DURATIONLIM                                   │
├────────────────────┼─────────────────────────────────────────┼──────────────────────────────────────────────────┤
│ Cohort             │ rescans all 1..popn every day           │ fixed gen1_spreaders, seeded once before day 1   │
├────────────────────┼─────────────────────────────────────────┼──────────────────────────────────────────────────┤
│ Per-day prelude    │ init series, seed cases, SD cases,      │ none                                             │
│                    │ vaccination                             │                                                  │
├────────────────────┼─────────────────────────────────────────┼──────────────────────────────────────────────────┤
│ Spread/progression │ interleaved per person in one pass      │ two separate passes (all spreads, then all       │
│                    │                                         │ progressions)                                    │
├────────────────────┼─────────────────────────────────────────┼──────────────────────────────────────────────────┤
│ Eligibility        │ status != INFECTIOUS || sickday() >=    │ sendrisk <= 0 || status != INFECTIOUS            │
│                    │ today                                   │                                                  │
└────────────────────┴─────────────────────────────────────────┴──────────────────────────────────────────────────┘

Two things stand out that a surface read misses:

- No sickday guard in the r0 loop. runsim blocks same-day spreading (sickday() >= today). The r0 loop omits it — it doesn't need it, because secondary cases are never instantiated, so there's no gen-2 to accidentally spread on its seed day.
- The two-pass vs interleaved split is only meaningful in runsim. Because runsim's spread mutates (calls make_sick on contacts), intra-day ordering has real effects there. In the r0 loop nothing mutates, so splitting into two passes is purely organizational — order is irrelevant.

Spread kernel: production spread() vs local spread_and_count()

Same skeleton (gamma-draw contact count, Bernoulli touch, isinfected test), but the copy strips four context features and the mutation:

┌───────────────────────┬──────────────────────────────────────────┬─────────────────────────────────────────────┐
│                       │          spread() (spread.cpp)           │ spread_and_count() (r0_simulation.cpp:116)  │
├───────────────────────┼──────────────────────────────────────────┼─────────────────────────────────────────────┤
│ Spreader              │ SD-overridden via person.sdcase() (line  │ always base social.contactfactors (128)     │
│ contactfactors        │ 21)                                      │                                             │
├───────────────────────┼──────────────────────────────────────────┼─────────────────────────────────────────────┤
│ Contact touchfactors  │ SD-overridden via contact.sdcase() (85)  │ always base social.touchfactors (143)       │
├───────────────────────┼──────────────────────────────────────────┼─────────────────────────────────────────────┤
│ Contact selection     │ ring-aware split when rings on (46–72)   │ global append_n_draws only (133)            │
├───────────────────────┼──────────────────────────────────────────┼─────────────────────────────────────────────┤
│ On infection          │ contact.make_sick(...) — mutates pop +   │ ++newly_infected — counts, no mutation      │
│                       │ series                                   │ (148–151)                                   │
└───────────────────────┴──────────────────────────────────────────┴─────────────────────────────────────────────┘

Context the r0 run flattens (beyond the kernel)

These are set in run_r0_sim/r0_sim, not in the doc's kernel comparison:

- Density: runsim looks up real geodata.density for the locale; r0 hardcodes density_factor = 1.0.
- Seasonality: runsim uses model.indoor_seq; r0 uses indoor_seq(DURATIONLIM, 1.0f) — flat.
- Vaccination: runsim calls vaccinate() daily; the r0 loop never vaccinates during the run, though it still passes dovax/vaxset into the kernel and progression, so existing vax protection is honored — no new doses are administered.
- RNG seeding: runsim calls xo::seed(99999); run_r0_sim seeds nothing — its result depends on global RNG state at call time. Worth flagging as a reproducibility caveat.
- Series: r0series is built only to satisfy the make_sick/progression signatures and is then discarded — no output.

Counting semantics (the conceptual payoff)

Because targets are never marked sick, an UNEXPOSED person can be "infected" repeatedly across days by different gen-1 spreaders. So r0_infected is the total transmission events generated by the cohort over their full infectious window (gated each day by sendrisk[duration] as progression advances them toward recovery/death). Dividing by cohort size yields mean secondary infections per spreader = R0. That's correct by construction, not a bug.

---
Two things this fresh pass turned up that the existing doc didn't:

1. SimDayGuard (r0_simulation.cpp:83) appears to be dead code — defined but never instantiated; the day snapshot/restore in run_r0_sim is done manually via saved_day (162, 224). Possibly a leftover from an earlier RAII approach.
2. The unseeded RNG in run_r0_sim — a genuine reproducibility wrinkle the doc doesn't mention.
