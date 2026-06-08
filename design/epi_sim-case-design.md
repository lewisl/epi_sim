# epi_sim — Case & Project Design Guidance

## Core principle

**A case == its `config.json`.** The config defines a run: it names every input via an explicit key and declares the output location. Identity lives in the config and in the directory that holds it — not in any naming convention the engine assumes. Directory layout is an ergonomic default, never a requirement.

Everything reduces to: **resolve a case dir (by label under project-dir, or by direct path), load its `config.json`, run it.**

---

## Two modes

The two modes are the *same operation* with two ways of naming the target case dir. Both: scaffold a case dir → user edits inputs → run that case dir. The only difference is how you point at the dir.

### Managed mode — label under a persisted project-dir

You give a **case-label**; the tool prepends `project-dir` (from the canonical toml) to get the path. This is the mode that enables automated cross-case summary/comparison, because all cases live under one known parent.

```
epi_sim --set-project-dir /home/lewis/epi/projects   # once, persistent
epi_sim --init-case baseline                          # scaffold <project-dir>/baseline/
# ... user edits inputs/ and config.json ...
epi_sim --use-case baseline                           # run <project-dir>/baseline/config.json
```

### Explicit mode — direct path, no project-dir

You give a **path** directly. No canonical toml, no parent concept — the "parent dir" is just wherever the path points. Less ceremony; cross-case comparison still possible but you must gather paths yourself.

```
epi_sim --setup-dir ./experiments/baseline    # scaffold at that path
# ... user edits ...
epi_sim --use-dir ./experiments/baseline      # run <path>/config.json
```

Managed is literally explicit with a path-prefix step in front: `--use-case bar` ≡ `--use-dir <project-dir>/bar`. One resolver underneath.

**The flag vocabulary encodes the mode** (deliberately asymmetric so they never blur):
- managed = `--init-case` / `--use-case` (label-valued)
- explicit = `--setup-dir` / `--use-dir` (path-valued)

---

## project-dir

A single directory holding case subdirectories as siblings. Not a config, not a logic-bearing marker — just "the folder where my cases live." **Exactly one at a time.**

```
<project-dir>/
  baseline/
    config.json
    inputs/
    outputs/
  high-vax/
    config.json
    inputs/
    outputs/
```

### Set / show (persistent, explicit, never run the sim)

```
epi_sim --set-project-dir /home/lewis/epi/projects
epi_sim --show-project-dir
```

`--set-project-dir` writes the canonical file at `~/.config/epi_sim/project-dir.toml`:

```toml
project-dir = "/home/lewis/epi/projects"
```

(XDG: `$XDG_CONFIG_HOME/epi_sim/` else `~/.config/epi_sim/`. Dir `0700`, file `0600`.)
`--show-project-dir` reports path + contents, or that nothing is set.

---

## Switch summary

| Flag | Mode | Value | Runs? | Effect |
|---|---|---|---|---|
| `--set-project-dir <path>` | managed | path | no | write canonical toml |
| `--show-project-dir` | managed | — | no | report canonical toml |
| `--init-case <label>` | managed | label | no | scaffold `<project-dir>/<label>/` |
| `--use-case <label>` | managed | label | yes | run `<project-dir>/<label>/config.json` |
| `--setup-dir <path>` | explicit | path | no | scaffold at path |
| `--use-dir <path>` | explicit | path | yes | run `<path>/config.json` |

- `--init-case` / `--use-case` require a canonical project-dir; error with a useful message if absent (see Failure section).
- `--setup-dir` / `--use-dir`: relative path resolves against **cwd**, absolute used as-is. The same path given to setup and to use must resolve identically.
- The old run flags `--config`, `--seed`, `--sd-seed` are **gone** as concepts. Those inputs are now config keys (below). No one-run override flags retained.

---

## Path resolution (inside a case)

Every path value in `config.json` resolves against **the config file's directory** (i.e. effectively `inputs/...`). This transparently supports flat files *and* explicit subdirs (e.g. a value like `ring_experiment/rings_oneleaky.json`). The engine only knows "resolve relative to config dir"; the `inputs/` convention is the scaffold's default, not an engine assumption.

---

## config.json schema (new style)

```json
{
    "days": 180,
    "locale": 38015,
    "calendar_start": "2020-01-01",
    "dovax": false,
    "debug": false,

    "case_desc": "young: no distancing; old: rigorous; Moderna 50pct",

    "geodata":           "input/geodata.csv",
    "variants":          "input/variants.json",
    "social_params":     "input/socialparams.json",
    "seed":              "input/seed_basic.json",

    "vaccines":          "input/vaccines.json",
    "vax_sched_dir":     "input/vaccine_100k",
    "rings":             "input/rings.json",
    "social_distancing": "input/soc_dist.json",

    "output": "output"
}
```

### Uniform key rule (generalized from the original rings pattern)
**Key present → resolve path → activate the input. Key absent → required keys error; optional keys no-op.** No input has an assumed default filename. The sole defaulted name is `config.json` itself.

### Required vs optional

| Key | Class | If absent |
|---|---|---|
| `geodata` | required | error |
| `variants` | required | error |
| `social_params` | required | error |
| `seed` | required | error |
| `vaccines` | optional | feature off (consistent with `dovax:false`) |
| `vax_sched_dir` | optional | no vaccination schedule |
| `rings` | optional | no ring structure |
| `social_distancing` | optional | no distancing intervention |

Notes:
- `seed` is **required** — a seedless run has no initial infectious individuals and does nothing.
- `vax_sched_dir` is **dir-valued**, not file-valued: it points at a directory of vaccine-schedule files. Kept as multiple files deliberately — a single schedule runs ~60 lines of declarative policy (who/when/distribution/inventory); flattening into one array destroys readability. This is the one legitimately dir-valued key; the design tolerates others if named explicitly.
- `social_params` vs `social_distancing` are **distinct inputs** — don't collide them. (`social_params` = the existing `social` key, renamed for clarity while breaking format anyway. `social_distancing` = the new array-of-blocks distancing schedule.)

---

## case-label and case_desc

**case-label = `basename` of the directory containing `config.json`.** There is **no `name` key.** Single source of truth — the label can never disagree with anything because there is nothing to disagree with. Works in both modes (managed: the `<label>` dir; explicit: whatever dir the path points to).

Because the label becomes a filename substring, **validate the dir name** at scaffold time: no path separators, no leading dots, no whitespace-only; restrict to `[A-Za-z0-9._-]`. Keeps `<label>_*` glob-safe across Forklift/macOS/Linux.

**`case_desc`** (optional, ≤200 chars) is the escape valve for prose — the long story a dir name should never carry (`young-no-distancing-old-rigorous-moderna-50pct`). It belongs in **provenance, not filenames**: stamp it into series/pop CSV header metadata and the plot HTML `<title>`/caption, so a run's artifacts self-describe when opened, while filenames stay short and sortable. Reject (don't silently truncate) if over the length cap.

---

## Outputs

Hardcoded test directories are removed; output location comes from the config.

**`output` key** — optional, single directory. Absent → default `outputs/` relative to config dir. Present → use it (create on demand). Same path-resolution rule as inputs. Use case: run variants of a case by editing inputs *and* re-pointing `output` so runs don't intermingle.

**Flat, not subdirs.** All three artifact kinds go flat in the output dir. With `<case-label>` leading the filename, a flat listing groups by case → kind → time — exactly the browse hierarchy you want. Per-type subdirs would scatter a single run's three artifacts across three folders, fighting that grouping.

**Filename convention** (label first, type second, timestamp last):

```
<case-label>_series_<YYYYMMDDTHHMMSS>.csv
<case-label>_pop_<YYYYMMDDTHHMMSS>.csv
<case-label>_<plotname>_<YYYYMMDDTHHMMSS>.html
```

- Leading `<case-label>` makes cases sort together; type groups a case's kinds; timestamp sorts each kind chronologically.
- **Timestamp:** `YYYYMMDDTHHMMSS` (ISO basic, no separators) — lexically sortable, filename-safe (no colons), unambiguous. Identical format and field order across all three writers, so `glob('<label>_*')` reliably collects a case's full history.
- **No overwrite protection** — collision requires two runs of the same-named case to the same dir within one second; pruning is the user's job. (If ever needed, append a counter `..._01` on collision; not worth it initially.)

---

## Failure: managed op with no project-dir

When `--init-case` / `--use-case` is invoked but no canonical toml exists:

```
error: no project directory configured.
  set one with:    epi_sim --set-project-dir /path/to/projects
  or work directly: epi_sim --setup-dir /path/to/new-case   (then --use-dir <same path>)
```

Two concrete next actions, both supported.

---

## Scaffolding (--init-case / --setup-dir)

Creates a new case; does **not** run. Refuses to clobber an existing case dir.

Writes:
- `input/` with template files for every input key (required ones, plus commented/optional stubs).
- empty `output/`.
- skeleton `config.json` with all required keys filled to the template paths, optional keys present-but-pointing-at-stubs or omitted (your choice), and a placeholder `case_desc`.

After scaffolding the user edits files in an editor to define the case, then runs it (`--use-case` / `--use-dir`). The case with its inputs must exist on disk before a run — the engine looks for the files and runs only if all required inputs load.

---

## Implementation reuse notes

Factor these once; both modes share them:

- **`resolve_case_dir(args) -> path`** — the single chokepoint. Managed: read canonical toml → join `project-dir / label`. Explicit: canonicalize the given path (relative→cwd). Both yield a case dir; everything downstream is identical.
- **`config_dir(case_dir) -> path`** — `case_dir` itself; base for resolving all relative input/output paths.
- **`load_canonical_project_dir() -> optional<path>`** — XDG-aware read of `project-dir.toml`; shared by `--use-case`, `--init-case`, `--show-project-dir`.
- **`resolve_input(config, key, {required, dir_valued}) -> optional<path>`** — uniform present/absent/required logic; one place implements the rings pattern for every key.
- **`validate_case_label(dirname)`** — filename-safety check at scaffold time.
- **`scaffold_case(target_dir)`** — writes templates + skeleton config + `outputs/`; shared by `--init-case` and `--setup-dir`, differing only in where `target_dir` comes from.
- **`stamp(label, kind, plotname?) -> "<label>_<kind>_<YYYYMMDDTHHMMSS>"`** — output filename builder; used by every writer.
- **`write_provenance(stream, case_desc, config_path, timestamp)`** — emits `case_desc` + run metadata into CSV headers and HTML title; one place.
