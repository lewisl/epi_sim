```
variant (e.g., "delta", "omicron")
  └─ progression_tree
      └─ agegrp (e.g., "0-19", "20-39", "40-59", "60-79", "80+")
          └─ breakday (e.g., 5, 9, 14, 19, 25)
              └─ current condition (nil, mild, sick, severe)
                  └─ outcome probabilities [recover, nil, mild, sick, severe, dead]
                                            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                                            6 floats summing to 1.0
```

At load time this JSON hierarchy is converted to `ProgressionTree`:

- `entry_index[age][duration]` directly identifies a packed breakday entry, or
  contains `NO_PROGRESSION_ENTRY` when the duration is not a breakday.
- `entries[entry][current_condition][outcome]` stores fixed-size arrays inline.
- Breakday entries are contiguous; the progression hot path performs no hash
  lookup or allocation.
- Age and current-condition indices are zero-based projections of the real
  one-based `Agegrp` and `Condition` values. Outcome indices are directly
  zero-based `Progressionmap` values (`recover=0` through `dead=5`).
