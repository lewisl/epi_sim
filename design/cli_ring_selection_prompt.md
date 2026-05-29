# Task: make ring selection expressible in SeriesColSpec (do this, not your option A or C)

The series accumulation layer already stores a ring dimension: each `SeriesGroup`
indexes `subjects[subject_idx][ring][AgeBucket][day]`, ring 0 is the all-rings
aggregate (`RING_ALL`), real rings are 1..N matching `Ring::names`. `resolve_series`
can already read a ring once it's told which one. What's missing is a way for the
**selection** layer (`SeriesColSpec` / `SeriesSelection`) to name a ring, so that
`serialize_selected_series`, `print_selected_series`, and `seriesplot` can emit
ring-specific columns.

We are committing to **your option B** (promote `SeriesSelection` to a struct with a
defaulted ring field). Do NOT implement option A (suffix-in-name like
`@ring:Jail`) — we are removing that convention, not extending it. Do NOT implement
option C (a separate `rings` vector cross-product on `SeriesColSpec`).

Rationale you don't need to re-derive: there is no interactive/REPL path in C++, so
selections are written as source literals fed to
`SeriesColSpec(std::initializer_list<SeriesSelection>)`. A three-field struct with a
defaulted ring keeps the common (no-ring) literal exactly as terse as today and makes
ring a self-documenting per-column field. Ring is identified by **name string** (not
a numeric id) in the literal, because a literal `{"now_infectious","total",3}` is
cryptic and order-coupled, whereas `"Jail"` is clear and survives renumbering.

## Constraints — read before editing

- Existing two-field literals MUST keep compiling and behave identically (ring
  defaults to all-rings aggregate = today's numbers). E.g. every call in `sim.cpp`
  (~lines 175, 189-222) and `build_for_buckets` constructs `{"now_x", bucket}` and
  must be untouched.
- `SeriesSelection` is currently `using SeriesSelection = std::pair<string,string>`.
  Grep confirmed there is NO `.first`/`.second` usage anywhere — all construction is
  brace-init `{"now_x", bkt}`. So promoting to an aggregate struct is safe and
  `build_for_buckets` + the three sentinel constructors need ZERO changes (aggregate
  init fills `name`, `bucket`, leaves `ring=""`).
- If a ring token is unknown (not in `Ring::names`), treat that column as an invalid
  selection (same path as an unknown bucket), don't crash.
- This must build to a committable, working state. Run the full test suite and a real
  sim with no rings (output must be byte-identical to before) and a real sim with
  rings defined.

## Changes (exhaustive list — nothing else should change)

### 1. `src/series.h` — replace the alias (currently line 11)

```cpp
struct SeriesSelection {
    std::string name;
    std::string bucket;
    std::string ring = "";   // "" → RING_ALL (all-rings aggregate)
};
```

Update the `SeriesColSpec` usage doc comment (~lines 104-110) to add one example
line showing a ring-qualified row alongside a bare row, e.g.:
`SeriesColSpec{{"now_infectious","total","Jail"}, {"now_dead","total"}}`.

### 2. `src/series.h` — `resolve_series` declaration

Add a defaulted ring param:

```cpp
std::optional<vector<int>> resolve_series(const AllSeries& series,
    std::string_view name, AgeBucket bucket, uint8_t ring = RING_ALL);
```

(`RING_ALL` already exists from the accumulation work. If for some reason it does
not, add `inline constexpr uint8_t RING_ALL = 0;` near the top of series.h.)

### 3. `src/series.cpp` — add a ring-token resolver and thread ring through `resolve_series`

Add above `resolve_series`:

```cpp
// "" → RING_ALL. A decimal token is taken as a literal ring id; otherwise looked
// up by name in Ring::names. nullopt if unknown.
static std::optional<uint8_t> ring_id_from_token(const std::string& tok) {
    if (tok.empty()) return RING_ALL;
    if (std::all_of(tok.begin(), tok.end(), ::isdigit)) {
        auto v = std::stoul(tok);
        if (v < Ring::names.size() || v == 0) return static_cast<uint8_t>(v);
        return std::nullopt;
    }
    auto it = std::find(Ring::names.begin(), Ring::names.end(), tok);
    if (it == Ring::names.end()) return std::nullopt;
    return static_cast<uint8_t>(std::distance(Ring::names.begin(), it));
}
```

In `resolve_series`, add the `uint8_t ring = RING_ALL` param and pass `ring` into
every `.at(idx, bucket)` call inside it (the status loop, the now_/new_vaccinated
aggregate loop, and the named variant/vax lookups) → `.at(idx, bucket, ring)`.

(If `Ring` is not visible in series.cpp, include its header — traits.h — though it's
likely already transitive via parameters.h/population.h.)

### 4. `src/series.cpp` — the two consumer loops

`print_selected_series` (~line 258) and `serialize_selected_series` (~line 317) both
currently do:

```cpp
for (const auto& [name_text, bucket_text] : selections) {
    auto bucket = age_bucket_from_string(bucket_text);
    if (!bucket) { invalid_selections.push_back(fmt::format("{}:{}", name_text, bucket_text)); continue; }
    auto data = resolve_series(series, name_text, *bucket);
    if (!data)   { invalid_selections.push_back(fmt::format("{}:{}", name_text, bucket_text)); continue; }
    cols.push_back({fmt::format("{}:{}", name_text, bucket_text), std::move(*data)});
}
```

Replace each with:

```cpp
for (const auto& sel : selections) {
    auto bucket = age_bucket_from_string(sel.bucket);
    auto ring   = ring_id_from_token(sel.ring);
    auto label  = sel.ring.empty()
                      ? fmt::format("{}:{}", sel.name, sel.bucket)
                      : fmt::format("{}:{}:{}", sel.name, sel.bucket, sel.ring);
    if (!bucket || !ring) { invalid_selections.push_back(label); continue; }
    auto data = resolve_series(series, sel.name, *bucket, *ring);
    if (!data)            { invalid_selections.push_back(label); continue; }
    cols.push_back({label, std::move(*data)});
}
```

The label appends the ring only when present, so existing column headers/labels are
unchanged for bare selections and ring columns read like `now_infectious:total:Jail`.

### 5. `src/plot.cpp` — `seriesplot` (~line 117)

Same destructuring change as step 4: replace the `[name_text, bucket_text]` loop with
the `sel.name/sel.bucket/sel.ring` version, compute `ring` via `ring_id_from_token`,
build the same conditional label, pass `*ring` into `resolve_series`.

### 6. Tests — `test/test_series.cpp`

Add two cases (existing tests must still pass unchanged):
- ring-qualified selection resolves to the correct ring: seed distinct values in two
  real rings, build a spec with `{"now_infectious","total","<ring1name>"}`, assert the
  resolved column matches ring 1's seeded data and differs from ring 2's.
- bare selection resolves to the all-rings aggregate: `{"now_infectious","total"}`
  returns the RING_ALL slot (== sum over rings).

## Do NOT
- Do not add `@ring:` suffix parsing (that's the rejected option A).
- Do not add a `rings` vector to `SeriesColSpec` (rejected option C).
- Do not change `build_for_buckets`, the sentinel constructors, the
  `initializer_list`/`vector` `SeriesColSpec` constructors, or any `sim.cpp` call site.
- Do not touch the accumulation layer (`SeriesGroup::update`, hot-loop call sites).

## Definition of done
Builds clean. Full test suite green. A no-rings sim produces byte-identical output to
the pre-change branch. A rings-defined sim can serialize/plot a literal like
`{{"now_infectious","total","Jail"}, {"now_dead","total"}}` and gets a Jail-specific
infectious column plus an all-rings dead column.
