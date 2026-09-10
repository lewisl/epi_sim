Build a **catalog of selectable histories** for `/list_histories`, then use those selections in plotting and CSV export commands. I’m reading your correction as: plot or serialize selected data; don’t add a command that prints daily values to the console.

There’s already a useful starting point: `HistorySelectionSpec("all").selections` enumerates meaningful names across ages, including totals and `vaccinated`. However, it uses global name tables and only selects the all-rings aggregate. It doesn’t enumerate each ring or validate availability against a particular `Histories`.

Here’s the implementation plan for you.

1. **Define what the catalog contains.**

   Each entry should carry a `HistorySelection`: `name`, `age`, and `ring`. These are output selections, so they don’t necessarily have a single physical column index.

   Include:

   - All valid status selections, excluding `new_unexposed`.
   - Both phases of each active vaccine brand and variant.
   - `now_vaccinated` and `new_vaccinated`.
   - Each concrete age plus `"total"`.
   - The empty ring selector, meaning the whole population, plus each active named ring.

   Preserve the existing behavior that `vaccinated` is valid with zero vaccine brands and produces zeros. Exclude sentinel trait values such as `"none"`.

2. **Separate selection inspection from materialization.**

   Start in [series.cpp](/Users/lewislevin/code/epi_sim/src/series.cpp:571). `resolve_history_selection` currently validates each selection, identifies its atomic sources, creates its label, and materializes its day vector.

   Extract the first three operations into a small reusable inspection function. Its result should contain the canonical label and source indices, but no daily data. Reuse the existing `resolve_history_source` and label logic.

   Have `resolve_history_selection` use that inspection result before materializing. This gives enumeration and future TUI validation the same rules as plotting and serialization, without allocating day vectors merely to list choices.

3. **Add an enumeration function in the series layer.**

   A suitable starting signature is:

   ```cpp
   std::vector<HistorySelection>
   enumerate_history_selections(const Histories& histories);
   ```

   For a small first implementation:

   - Take candidates from `HistorySelectionSpec("all").selections`.
   - Include each candidate with its empty ring selector.
   - Expand it for each real ring in this `Histories`.
   - Keep candidates accepted by the inspection function.

   This reuses the existing name-generation rules while filtering against the actual history layout. Keep the order deterministic. Preserve separately named selections even when they happen to produce identical data—for example, one vaccine brand and `vaccinated`.

4. **Update `/list_histories` to display the catalog.**

   Show the selector fields explicitly:

   ```text
   name                 age       ring
   now_infectious       total     (all rings)
   now_infectious       age0_19   Jail
   now_vaccinated       total     (all rings)
   ```

   Explain that `(all rings)` represents an empty ring field. Report the count as “selectable histories.”

   Include a short explanation of the existing selection shortcuts:

   | Selection | Meaning |
   |---|---|
   | `"all"` | All generated names × all ages, including age totals; aggregate rings |
   | `"all", "total"` | All generated names, total age; aggregate rings |
   | `"all", selected ages` | All generated names for those ages; aggregate rings |
   | Explicit field selections | Specific name, age, and optional ring combinations |

   Keep `"all"` as a shortcut, rather than representing it as a catalog column. Don’t silently change its existing ring semantics.

5. **Then add selection input for plot/export commands.**

   Have one shared TUI selection flow produce a `HistorySelectionSpec`: either explicit field selections or one of the existing `"all"` forms.

   Validate using the inspection function, then pass the spec to `historyplot` or `serialize_selected_histories`. Materialize totals only when those consumers run. Review the existing `/plot` command deliberately: it currently reports output locations.

6. **Verify each increment.**

   Add cases to the existing `series` group covering catalog validity, omitted placeholders, age/ring totals, and zero/one/multiple vaccine brands and rings. Every enumerated selection should resolve successfully.

   Run `xmake run test series` first, then the full `xmake run test` after the shared resolver/header changes. Build `epi_sim` and manually check the TUI before and after a simulation.

I’d implement steps 1–4 as the first learning exercise, then tackle selection input and export. No files changed.