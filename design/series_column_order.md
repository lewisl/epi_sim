# `Histories::history_vectors_`: Atomic Column Order

Every physical outer-vector entry is one atomic column. No entry represents a
sentinel trait value, total age, or total ring.

## General order

The six contiguous groups are:

| Group | Base | Width |
|---|---:|---:|
| `status/now` | `0` | `S` |
| `status/new_` | `S` | `S` |
| `vax/now` | `2S` | `V` |
| `vax/new_` | `2S + V` | `V` |
| `variant/now` | `2(S + V)` | `W` |
| `variant/new_` | `2(S + V) + W` | `W` |

where:

```text
S = 4 statuses      * ring_lane_count * 5 ages
V = active vaccines * ring_lane_count * 5 ages
W = real variants   * ring_lane_count * 5 ages
```

Within every non-empty group, columns advance in this order:

```text
first real trait value
  first ring lane
    age0_19
    age20_39
    age40_59
    age60_79
    age80_up
  second ring lane
    the same five ages
  ...
second real trait value
  the same ring and age order
...
```

When rings are disabled, there is exactly one implicit whole-population lane;
its raw ring value is 0. When rings are enabled, only raw ring values `1..N`
are stored. In neither case is there an additional all-rings lane.

## Worked layout: two rings, two vaccines, two variants

Here `R = 2`, so each trait value occupies ten columns:

```text
S = 4 * 2 * 5 = 40
V = 2 * 2 * 5 = 20
W = 2 * 2 * 5 = 20
total columns = 2 * (S + V + W) = 160
```

The resulting ranges are:

| Columns | Trait | Phase | Raw trait values |
|---:|---|---|---|
| `0..39` | status | now | `1..4` |
| `40..79` | status | new | `1..4` |
| `80..99` | vax | now | `1..2` |
| `100..119` | vax | new | `1..2` |
| `120..139` | variant | now | `1..2` |
| `140..159` | variant | new | `1..2` |

Some boundary columns make the ordering concrete:

| Column | Coordinates |
|---:|---|
| `0` | status / now / unexposed / ring 1 / age0_19 |
| `4` | status / now / unexposed / ring 1 / age80_up |
| `5` | status / now / unexposed / ring 2 / age0_19 |
| `9` | status / now / unexposed / ring 2 / age80_up |
| `10` | status / now / infectious / ring 1 / age0_19 |
| `39` | status / now / dead / ring 2 / age80_up |
| `40` | status / new / unexposed / ring 1 / age0_19 |
| `79` | status / new / dead / ring 2 / age80_up |
| `80` | vax / now / first brand / ring 1 / age0_19 |
| `120` | variant / now / first variant / ring 1 / age0_19 |
| `159` | variant / new / second variant / ring 2 / age80_up |

Columns `40..49` in this example are the deliberate `new_unexposed`
placeholders. They remain zero and are not selectable. No other physical
columns are intentionally unused.

## case-1 layout

The referenced case-1 configuration has six real variants, vaccination
disabled, and rings disabled. Therefore:

```text
S = 4 * 1 * 5 = 20
V = 0
W = 6 * 1 * 5 = 30
total columns = 2 * (20 + 0 + 30) = 100
```

Its ranges are:

| Columns | Group |
|---:|---|
| `0..19` | status / now |
| `20..39` | status / new |
| none | vax / now and vax / new |
| `40..69` | variant / now |
| `70..99` | variant / new |

The five columns `20..24` are the only always-zero outer columns: the five
concrete-age `new_unexposed` placeholders in the implicit population ring
lane. Each listed outer column contains its own inner day vector:

```text
[unused day 0, day 1, ..., day_cnt]
```
