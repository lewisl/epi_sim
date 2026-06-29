
**Big Picture**
`runsim` in `src/sim.cpp` is the full epidemic simulation loop. The mini-loop in `src/r0_simulation.cpp` is a constrained R0 estimator: it seeds generation-1 spreaders, lets only those spreaders generate hypothetical infections, counts those infections, and never turns contacts into new infectious people.

**Key Differences**
| Area | `runsim` | R0 mini-loop |
|---|---|---|
| Population | Uses `model.pop` | Uses separate `r0pop`; `r0_sim` synthesizes `200'000` people, `rt_sim` copies a supplied population |
| Duration | `1..model.ndays` | `1..DURATIONLIM` |
| Day state | Resets global sim day and leaves it advanced | Saves day, resets to 0, restores day afterward |
| Series | Full `AllSeries` with model days, vaccine slots, ring slots; initializes/finalizes history | Minimal `AllSeries(DURATIONLIM, ..., 1, 1)`; no daily history init/finalize |
| Seeding | Runs configured `SeedCase`s on matching days | Calls `seed_gen1` once before day loop, proportional by age distribution |
| Who spreads | Every infectious person in population, except same-day infections | Only the fixed `gen1_spreaders` vector |
| Same-day guard | Requires `person.sickday() < sim::ds.day` | No sickday guard; gen1 is seeded before day 1 |
| Spread function | Calls full `spread(...)` | Calls local `spread_and_count(...)` |
| Infection mutation | Infected contacts become sick via `contact.make_sick(...)` | Contacts are counted but not mutated |
| Secondary spread | Newly infected people can later spread | No secondary spread; only generation-1 spreaders ever spread |
| Social distancing | Applies daily cases and per-person sd overrides | Does not apply sd cases; uses base social params |
| Rings | Full ring-aware contact selection | Global random mixing only |
| Seasonality/indoor | Uses `model.indoor_seq` | Uses all-ones indoor sequence |
| Density | Locale geodata density factor | Hard-coded `density_factor = 1.0` |
| Vaccination | Runs daily `vaccinate(...)` if enabled | No daily vaccination; only passes existing vax data/effects into kernels |
| Output | CSV, summary, timings, browser plots unless headless | Prints `r0 infected`, returns `r0_infected / gen1_spreader_cnt` |

**Shared Parts**
Both loops still use the same `progression(...)` kernel for spreaders, and both check `sendrisk` before attempting spread. Both use `isinfected(...)` for the infection probability test, but `runsim` applies the result to population state while R0 only increments a counter.

**Most Important Behavioral Difference**
`runsim` models transmission chains. The R0 mini-loop estimates direct infections caused by the initially seeded generation only, deliberately preventing those contacts from becoming new spreaders.

---

## User

r0_simulation meets the academic definition of how R0 should be calculated.

---

## Assistant (Code · GPT-5.5 · 28.7s)

_Thinking:_

**Considering R0 vs Rt Analysis**

I'm thinking about the difference between R0 and Rt. If we're looking at if `dovax` is true or using recovered/vaccinated populations, then it’s about Rt or effective reproduction, not R0. The new PopData likely reflects many unexposed individuals. I wonder if I need to consider age distribution statuses too. R0 assumes no behavioral interventions, and the model uses specific social and contact parameters. I might not need tools for this, but being rigorous is important.

Yes. The important academic-definition-aligned choice is that `r0_simulation.cpp` counts only direct secondary infections caused by the seeded generation-1 spreaders, and does not mutate contacted people into new infectious cases. That prevents onward transmission from being included in the numerator.

More precisely, `run_r0_sim` estimates:

`R0 = total direct infections caused by initial spreaders / number of initial spreaders`

The key caveats are:

- It is a Monte Carlo estimate, not an analytic value.
- It is only pure R0 when the population is fully susceptible and no intervention/immunity effects are active.
- If `dovax` or a copied non-naive population is used, the same loop is closer to `Rt` or an effective reproduction number.
- Because contacts are not mutated, the code counts infection events rather than enforcing unique newly infected people. With a large synthetic population this is probably negligible, but academically R0 is usually “secondary cases,” meaning unique individuals.

So the structural design is right for R0: seed infectives, count only their direct expected secondary infections, exclude secondary spread.

---
