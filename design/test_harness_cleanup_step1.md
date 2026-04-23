## Step 1 — Test Harness Cleanup

### Goal

Make the `test` target current, buildable, and easy to navigate without trying to solve the whole testing problem at once.

### Scope of this slice

1. Add a central test dispatcher.
2. Add shared test support helpers.
3. Rewrite the PopData serialization tests onto the live APIs:
   - `write_pop_data`
   - `pop_print`
   - `pop_to_csv`
4. Update `xmake.lua` so the `test` target builds this first split slice instead of the stale monolithic `test/test.cpp`.

### Out of scope for this slice

- series serialization tests
- plot smoke tests
- vaccination tests
- disease_modeling tests beyond what is needed for compilation
- full end-to-end smoke target

### Files to add

- `test/test_main.cpp`
- `test/test_support.h`
- `test/test_pop_serialize.cpp`

### Files to update

- `xmake.lua`

### Design choices

- Keep one `test` binary.
- Put all run-selection in `test/test_main.cpp`.
- For now, build only the new split files needed for the first slice.
- Leave `test/test.cpp` on disk but out of the target until it is decomposed safely.

### Pop serialization test coverage in this slice

#### Pretty output to stream

- `write_pop_data(..., Style::pretty, false, ...)`
- verify header and compact row rendering

#### Pretty output convenience wrapper

- `pop_print(...)`
- verify multi-line history rendering path still works

#### Serialized output to stream

- `write_pop_data(..., Style::serialized, false, ",", true)`
- verify CSV-like header and row output

#### Serialized output to file

- `pop_to_csv(...)`
- verify file exists and contains expected escaped content

#### Error handling

- invalid row `0`
- invalid row `> pop.popn`
- invalid column name

### Shared support helpers

- save/restore `Variant::names`
- save/restore `Vax::names`
- line trimming/splitting
- temp file helpers
- small 3-person PopData fixture using 1-based indexing

### Validation

This slice is complete when:

1. `xmake build -r test` succeeds.
2. `xmake run test` succeeds.
3. The active test target contains only the new split files for this slice.
4. No active test references deleted PopData serialization APIs.

### Next slices after this one

1. parameter-loading tests
2. disease-modeling tests
3. vaccination tests
4. trait tests
5. series I/O tests
6. smoke target