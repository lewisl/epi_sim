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
  zero-based `Progressionmap` underlying values (`ToRecover=0` through `ToDead=5`).

`Agegrp`, `Status`, `Condition`, and `Vaxstatus` define their names directly as
`inline static const std::vector<std::string>` members. A name's position is
its numeric ID. Names-based lookup supplies parsing; constrained explicit
integer constructors check the value before narrowing to one-byte `uint8_t v`
storage. The wrappers remain distinct types with no per-person name storage.
Agegrp and Condition retain compile-time counts for progression array dimensions;
setup checks those dimensions against the actual name tables. `Progressionmap`,
`TraitID`, and `Phase` remain ordinary scoped enums reflected with magic_enum.
History indexing and progression continue to use the same numeric offsets.

`Variant`, `Vax`, `SDCase`, and `Ring` use runtime name registries. Their loaders
register supplied names explicitly, rejecting empty, duplicate (including
case-insensitive duplicates), reserved-zero, and excess entries. Ordinary string
construction only resolves an existing name; it never extends a registry.
Explicit constrained integer constructors check registry bounds before narrowing.
Zero defaults remain available, while numeric `Ring{0}` remains invalid; use
`Ring{}` for its sentinel. Cardinal day values and per-person histories retain
their separate simulation invariants and do not use registry-membership guards.
