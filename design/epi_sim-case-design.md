# epi_sim — Case & Project Design Guidance

## Core principle

**A case == its `config.json`.** The config defines a run: it points at every input file and declares the output location. Identity lives in the config, not in directory layout. Directory layout is an ergonomic default, never a requirement.

Everything reduces to one sentence: **resolve a config (directly, or by name via project-dir), optionally patch a couple of fields, run it.**

---

## Two modes

### Unmanaged (explicit) mode

User hands you a config path directly. No project-dir, no canonical state, no name lookup. Always works.

```
epi_sim --config /any/path/config.json
```

- Config's relative paths resolve against **the config file's own directory** (not cwd).
- Self-sufficient: can create *and* run without ever touching the canonical toml.
- `name` key is advisory here (no enclosing dir name to check against).

### Managed mode

User works inside a single persisted project-dir and refers to cases by short name. Project-dir is *only* a name→path prefix — nothing more.

```
epi_sim --use-case baseline      # runs <project-dir>/baseline/config.json
```

- `--use-case bar` is exactly equivalent to `--config <project-dir>/bar/config.json`. Managed mode is sugar over explicit mode; one code path underneath.
- `name` key is authoritative-with-check: assert `config.name == basename(case_dir)`, refuse on mismatch (catches cloned-dir / stale-name errors).

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

(Use XDG: `$XDG_CONFIG_HOME/epi_sim/` else `~/.config/epi_sim/`. Create dir `0700`, file `0600`.)

`--show-project-dir` reports the path + contents, or reports that nothing is set.

---

## Switch summary

| Command | Needs project-dir | Runs sim | Effect |
|---|---|---|---|
| `--set-project-dir <path>` | — | no | writes canonical toml |
| `--show-project-dir` | — | no | reports canonical toml |
| `--init-case <name>` | yes (managed) / no (explicit) | no | scaffolds a case dir |
| `--use-case <name>` | yes | yes | runs `<project-dir>/<name>/config.json` |
| `--config <path>` | no | yes | runs that config (explicit) |
| `--seed <path>` | — | — | optional override of config's `seed` for one run |
| `--output <path>` | — | — | optional override of config's `output` for one run |

Notes:
- `--config` and `--use-case` are mutually exclusive.
- `--seed` / `--output` are *overrides only* — they patch one field for one run and write nothing. Not part of any required combination.
- There is **no one-shot `--project-dir`** flag. Arbitrary locations are handled by explicit mode.

---

## init-case (scaffolding)

Creates a new case; does **not** run.

- **Managed:** `epi_sim --init-case bar` → scaffolds `<project-dir>/bar/`.
- **Explicit:** `epi_sim --init-case --config /some/path/` → scaffolds at that path, canonical toml untouched.

Writes:
- `inputs/` with **canonical-name template files** (`variants.json`, `vaccines.json`, `socialparams.json`, `geodata.csv`, `rings.json`, `seed.json`).
- empty `outputs/`.
- skeleton `config.json` pointing at the templates.

**Refuses to clobber** an existing case dir.

---

## config.json schema

```json
{
  "name": "baseline",
  "inputs": {
    "variants":      "inputs/variants.json",
    "vaccines":      "inputs/vaccines.json",
    "social_params": "inputs/socialparams.json",
    "geodata":       "inputs/geodata.csv",
    "rings":         "inputs/rings.json"
  },
  "seed":   "inputs/seed.json",
  "output": "outputs/"
}
```

- All paths relative to **the config file's location**.
- `--seed` / `--output` override the corresponding key for a single run.
- Output dir created on demand.
- A single config is portable, self-describing, reproducible: hand someone the case dir and they can rerun it anywhere.

---

## Hard invariants (the opinionated parts)

1. **Exactly one config per case, always named `config.json`.** The filename is hardcoded, not user-chosen. The case's identity is its directory; the config inside is always `config.json`. Want variants → make a new case dir. (Same invariant as git/Cargo; keeps `outputs/` unambiguous.)

2. **Only template input files are named programmatically.** After scaffolding, the user copies/renames/edits inputs freely; the config's paths are the single source of truth for which inputs are active. Referents can be named anything.

3. **Variants are sibling cases, not multiple configs.** To vary a parameter, clone the case dir (or just edit the one config to point elsewhere). Cost is duplicated inputs — accepted, because it buys total output→config provenance.
   - *Later optimization if duplication hurts:* a variant's config may reference unchanged files via relative paths into a sibling (`../baseline/inputs/geodata.csv`), keeping only the differing file local. Never break invariant #1 to dedupe. Ship copy-everything first.

4. **No output overwrite protection.** Output filenames carry a date-time substring; pruning is the user's job. Use one consistent, sortable timestamp format across all output types (`YYYYMMDDTHHMMSS`).

---

## Output filename convention

`name` (from config) + timestamp uniquely identifies every run; outputs from different cases never collide even in a shared output dir.

```
outputs/baseline-20260602T143000-series.csv
outputs/baseline-20260602T143000-pop.csv
outputs/baseline-20260602T143000-plot.html
```

---

## Failure: managed op with no project-dir

When `--use-case` / `--init-case` (managed) is invoked but no canonical toml exists:

```
error: no project directory configured.
  set one with:   epi_sim --set-project-dir /path/to/projects
  or work directly: epi_sim --config /path/to/config.json
```

Two concrete next actions, both supported.

---

## Implementation reuse notes

The shared helpers worth factoring out once and reusing everywhere:

- **`resolve_config(args) -> path`** — the single chokepoint. Explicit: return `--config`. Managed: read canonical toml → join `project-dir / name / "config.json"`. Both feed the same runner.
- **`config_dir(config_path)`** — base for resolving all relative input/output paths. Used by both modes identically.
- **`load_canonical_project_dir() -> optional<path>`** — XDG-aware read of `project-dir.toml`; shared by `--use-case`, managed `--init-case`, and `--show-project-dir`.
- **`apply_overrides(config, --seed, --output)`** — patch fields post-load, pre-run. One place.
- **`scaffold_case(target_dir)`** — writes templates + skeleton config + `outputs/`; shared by managed and explicit `--init-case`, differing only in where `target_dir` comes from.
- **`stamp(name) -> "<name>-YYYYMMDDTHHMMSS"`** — output filename prefix; used by every output writer.
