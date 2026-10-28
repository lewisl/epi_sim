# Plan: reset vaccine state when loading successive cases

Prepared: 2026-10-28. Status: deferred for next week; no implementation authorized
or performed. Until then, quit and restart the application between cases.

## Problem and evidence

The TUI already discards its previous model and histories for `/run-case` and
`/run-dir`. Both commands rebuild the model through `build_model`, `setup_sim`,
and `setup_model_params`. Setup does run again.

However, `Vax::names` and `Vax::count` are static and outlive the model.
`setup_model_params` currently calls the vaccine and schedule loaders only when
`dovax` is true. When it is false, their local result objects are fresh, but
the vaccine registry is not completely reinitialized. Also, `load_vax_data`
currently clears names without resetting `Vax::count`, so repeated enabled
loads can accumulate the count.

`install_runtime_trait_names` assigns vaccine names from the new model but
does not update the count. History construction uses that count. The result
can be unwanted vaccine columns and out-of-bounds vaccine-name accesses.

The supplied one-ring listing contained 30 vaccine atomic columns and six
vaccine full totals despite case-1 having vaccination disabled. All 182 alias
keys mapped correctly; the excess columns were not materialized split totals.
The cross-run cause was traced in source, not independently reproduced.

## Agreed direction

Always invoke the vaccine initialization paths during setup. Pass `dovax`
into the loaders and let it control file loading, after initialization.
This follows the existing ring-loader pattern: initialize on every call,
then handle enabled and disabled behavior inside the loader.

Keep static trait registries. They must remain valid for the lifetime of the
active model, including subsequent `/list_histories` commands. Reinitialize
them while constructing a replacement model; do not clear them after a run
or before every TUI command. Preserve the selected project directory.

## Implementation checklist

- [ ] Add a `dovax` argument to `load_vax_data` in `src/parameters.h` and
  `src/parameters.cpp`; update all callers, including parameter tests.
- [ ] At the start of `load_vax_data`, create a fresh `VaxSet` and initialize
  `Vax::names` to `{"none"}` and `Vax::count` to zero, before any file access.
  Preserve the unused name slot without counting it as a vaccine brand.
- [ ] If vaccination is disabled, return the empty `VaxSet` immediately.
  Its names should be `{"none"}`, its usable vaccine count zero, and its
  parameter storage empty. Do not read or parse the vaccine file.
- [ ] If enabled, continue the existing loading logic. Preserve the existing
  dummy parameter at index zero for enabled vaccine data and keep real brand
  IDs at `1..Vax::count`. Every load must start counting brands from zero.
- [ ] Add the same flag to `load_vax_sched_set`. Construct a fresh schedule
  set and return it immediately when disabled, before checking the directory
  or enumerating files. When enabled, load schedules after vaccine data.
- [ ] In `setup_model_params` (`src/setup.cpp`), replace the outer `if (dovax)`
  with unconditional calls to both loaders, passing the flag explicitly.
  Disabled configurations already supply empty vaccine paths; retain that
  behavior and ensure the early returns make those paths harmless.
- [ ] Check `install_runtime_trait_names`: copying names from the new model
  must preserve agreement between names, brand count, and available parameters.
  Do not use a later assignment there as a substitute for loader initialization.

## Invariants and validation

After each successful setup, `Vax::count` describes only the current case and
equals `Vax::names.size() - 1`. Disabled vaccination has zero brands and zero
schedules. Enabled vaccination has the same IDs, data, and simulation behavior
as before. Read-only inspection commands retain the active case's registry.

- [ ] Extend existing parameter/setup tests for enabled -> disabled,
  disabled -> enabled, and repeated enabled loading. Verify that counts do
  not accumulate and that disabled loading accepts nonexistent input paths
  without accessing them. Do not add a new test group or harness.
- [ ] Exercise `/run-case` and `/run-dir`, followed by `/list_histories`.
  Include case-2 -> case-1 -> case-2 in one process using isolated fixture
  copies; do not modify the user's original case inputs.
- [ ] For the current one-ring case-1 with four statuses, six variants and
  vaccination disabled, expect 100 atomic columns, 20 physical full totals,
  120 physical columns overall, and 140 totals-map keys. Split totals remain
  aliases to existing columns.
- [ ] Build with `xmake build epi_sim`. Run the narrowest affected test groups
  first, then `xmake run test` because declarations in shared headers change.
  Report existing test compilation blockers separately; do not broaden this
  work into unrelated test migrations. Run `runsim` only explicitly if needed.

## Scope and separate follow-ups

This is a targeted vaccine initialization fix. A broader global reset,
subprocess execution, model-owned registries, RNG policy changes, history
redesign, and general static-state auditing are outside this implementation.

`/r0_sim` builds another model while retaining the current TUI model. Its
registry changes need a separate save/restore or reinstall design, including
exception paths. This plan does not establish complete isolation for that
command or for failed model loads. It addresses successful successive case
and directory runs without changing the retained-model TUI workflow.
