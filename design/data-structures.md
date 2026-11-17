## PopData::Struct of vectors and AgentView

### Struct of vectors
PopData holds the state for every agent (person, cow, other livestock) within a single population group that is the focus of the simulation.  Each vector holds a type based on the Trait structs. During the simulation, an agent's state is mutated based on outcomes from Spread and Progression.

<need more explanation here--might be in existing design files>

### AgentView
`PopData::AgentView` is a small proxy/handle, not a materialized row copy of `PopData`.

- An `AgentView` contains a reference to the underlying `PopData` plus the person's index
- Passing `PopData::AgentView` by value is normally appropriate and cheap
- Passing `PopData::AgentView` by value does **not** copy the population vectors or person state
- Do not switch `AgentView` parameters to `const PopData::AgentView&` for performance reasons; that usually adds indirection without benefit

Constness needs care here:

- `const PopData::AgentView` makes the proxy object itself const inside the callee
- It does **not** automatically make the underlying `PopData` entry const
- If `AgentView` exposes const-qualified accessors returning mutable references, a function can still update the underlying person data through a `const AgentView`
- Use `const PopData::AgentView` only when you specifically want to prevent rebinding or mutation of the proxy object inside the function; do **not** assume it means read-only access to the person's data

Practical guidance:

- Use `PopData::AgentView person` for ordinary agent-oriented helper functions
- Use `const PopData::AgentView person` only if the API is intentionally designed so the proxy itself should not be changed in the callee
- If a function must be truly read-only with respect to person state, the `AgentView` API itself must provide read-only accessors or a separate const view type




## Traits structs
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

### Traits zero-value semantics

Zero means different things per enum — do not assume uniformity:
- `Condition`: 0 = "uninfected", not used to index parameter arrays.
- `Status`: 0 = "none", not used as an index.
- `Agegrp`: 0 = "unknown", should not be used as an index.
- `Variant`: 0 = "none" and is generally an error during simulation; **1 = "base" is required** and often derives other variant parameters.
- `Vaxstatus`: 0 = "none" is a valid status.
- `Progressionmap`: explicitly zero-based, valid nums `0..5`.

### Variants and Progression

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


## Histories

Histories struct is a data structure for tracking daily outcomes during the simulation. History columns are vector of vectors.  History totals are summed at the completion of the days (or other click 'ticks') of the simulation into vectors already present in the Histories struct. Two maps are used for accessing the history vectors:  