# `AllSeries::cols_`: Flat Column Order

Read this document from top to bottom. Every ordinary table row is one physical
column in the outer `cols_` vector. An italic ellipsis row stands for the
omitted physical columns described by that row.

The section headings are only visual breaks in the one flat vector. They do not
represent another container level.

For 4 status values + none, 3 vaccines + none, 6 variants + none, 2 rings plus total, 5 ages + total and now and new, there are 576 columns.  Of these 108 columns are unused. A small modification of the column building code could eliminate these columns.

`none`, `total`, and `RING_ALL` are shown because they occupy real columns.
After those stored entries, the list shows the first two real subjects. Ring 1
is the second physical ring slot, after `RING_ALL`. If no real ring is
configured, the ring 1 rows do not exist.

## `now_status` starts at the first column

| Kind | Subject | Age | Ring |
|---|---|---|---|
| `now_status` | `none` (stored sentinel) | `total` | `RING_ALL` |
| `now_status` | `none` (stored sentinel) | `age0_19` | `RING_ALL` |
| *…* | `none` | *remaining ages* | `RING_ALL` |
| `now_status` | `none` (stored sentinel) | `total` | ring 1 |
| `now_status` | `none` (stored sentinel) | `age0_19` | ring 1 |
| *…* | `none` | *remaining ages* | ring 1 |
| *…* | `none` | *all ages* | *remaining rings* |
| `now_status` | `unexposed` | `total` | `RING_ALL` |
| `now_status` | `unexposed` | `age0_19` | `RING_ALL` |
| *…* | `unexposed` | *remaining ages* | `RING_ALL` |
| `now_status` | `unexposed` | `total` | ring 1 |
| `now_status` | `unexposed` | `age0_19` | ring 1 |
| *…* | `unexposed` | *remaining ages* | ring 1 |
| *…* | `unexposed` | *all ages* | *remaining rings* |
| `now_status` | `infectious` | `total` | `RING_ALL` |
| `now_status` | `infectious` | `age0_19` | `RING_ALL` |
| *…* | `infectious` | *remaining ages* | `RING_ALL` |
| `now_status` | `infectious` | `total` | ring 1 |
| `now_status` | `infectious` | `age0_19` | ring 1 |
| *…* | `infectious` | *remaining ages* | ring 1 |
| *…* | `infectious` | *all ages* | *remaining rings* |
| *…* | *remaining statuses: `recovered`, then `dead`* | *same age order* | *same ring order* |

## `new_status` follows immediately

| Kind | Subject | Age | Ring |
|---|---|---|---|
| `new_status` | `none` (stored sentinel) | `total` | `RING_ALL` |
| `new_status` | `none` (stored sentinel) | `age0_19` | `RING_ALL` |
| *…* | `none` | *remaining ages* | `RING_ALL` |
| `new_status` | `none` (stored sentinel) | `total` | ring 1 |
| `new_status` | `none` (stored sentinel) | `age0_19` | ring 1 |
| *…* | `none` | *remaining ages* | ring 1 |
| *…* | `none` | *all ages* | *remaining rings* |
| `new_status` | `unexposed` | `total` | `RING_ALL` |
| `new_status` | `unexposed` | `age0_19` | `RING_ALL` |
| *…* | `unexposed` | *remaining ages* | `RING_ALL` |
| `new_status` | `unexposed` | `total` | ring 1 |
| `new_status` | `unexposed` | `age0_19` | ring 1 |
| *…* | `unexposed` | *remaining ages* | ring 1 |
| *…* | `unexposed` | *all ages* | *remaining rings* |
| `new_status` | `infectious` | `total` | `RING_ALL` |
| `new_status` | `infectious` | `age0_19` | `RING_ALL` |
| *…* | `infectious` | *remaining ages* | `RING_ALL` |
| `new_status` | `infectious` | `total` | ring 1 |
| `new_status` | `infectious` | `age0_19` | ring 1 |
| *…* | `infectious` | *remaining ages* | ring 1 |
| *…* | `infectious` | *all ages* | *remaining rings* |
| *…* | *remaining statuses: `recovered`, then `dead`* | *same age order* | *same ring order* |

## `now_vax` follows immediately

Vaccine subjects use registration order. Vaccine ID 0 is `none`; the next
two rows of subjects below mean vaccine IDs 1 and 2, when present.

| Kind | Subject | Age | Ring |
|---|---|---|---|
| `now_vax` | `none` (stored sentinel) | `total` | `RING_ALL` |
| `now_vax` | `none` (stored sentinel) | `age0_19` | `RING_ALL` |
| *…* | `none` | *remaining ages* | `RING_ALL` |
| `now_vax` | `none` (stored sentinel) | `total` | ring 1 |
| `now_vax` | `none` (stored sentinel) | `age0_19` | ring 1 |
| *…* | `none` | *remaining ages* | ring 1 |
| *…* | `none` | *all ages* | *remaining rings* |
| `now_vax` | first registered vaccine | `total` | `RING_ALL` |
| `now_vax` | first registered vaccine | `age0_19` | `RING_ALL` |
| *…* | first registered vaccine | *remaining ages* | `RING_ALL` |
| `now_vax` | first registered vaccine | `total` | ring 1 |
| `now_vax` | first registered vaccine | `age0_19` | ring 1 |
| *…* | first registered vaccine | *remaining ages* | ring 1 |
| *…* | first registered vaccine | *all ages* | *remaining rings* |
| `now_vax` | second registered vaccine | `total` | `RING_ALL` |
| `now_vax` | second registered vaccine | `age0_19` | `RING_ALL` |
| *…* | second registered vaccine | *remaining ages* | `RING_ALL` |
| `now_vax` | second registered vaccine | `total` | ring 1 |
| `now_vax` | second registered vaccine | `age0_19` | ring 1 |
| *…* | second registered vaccine | *remaining ages* | ring 1 |
| *…* | second registered vaccine | *all ages* | *remaining rings* |
| *…* | *remaining registered vaccines* | *same age order* | *same ring order* |

## `new_vax` follows immediately

| Kind | Subject | Age | Ring |
|---|---|---|---|
| `new_vax` | `none` (stored sentinel) | `total` | `RING_ALL` |
| `new_vax` | `none` (stored sentinel) | `age0_19` | `RING_ALL` |
| *…* | `none` | *remaining ages* | `RING_ALL` |
| `new_vax` | `none` (stored sentinel) | `total` | ring 1 |
| `new_vax` | `none` (stored sentinel) | `age0_19` | ring 1 |
| *…* | `none` | *remaining ages* | ring 1 |
| *…* | `none` | *all ages* | *remaining rings* |
| `new_vax` | first registered vaccine | `total` | `RING_ALL` |
| `new_vax` | first registered vaccine | `age0_19` | `RING_ALL` |
| *…* | first registered vaccine | *remaining ages* | `RING_ALL` |
| `new_vax` | first registered vaccine | `total` | ring 1 |
| `new_vax` | first registered vaccine | `age0_19` | ring 1 |
| *…* | first registered vaccine | *remaining ages* | ring 1 |
| *…* | first registered vaccine | *all ages* | *remaining rings* |
| `new_vax` | second registered vaccine | `total` | `RING_ALL` |
| `new_vax` | second registered vaccine | `age0_19` | `RING_ALL` |
| *…* | second registered vaccine | *remaining ages* | `RING_ALL` |
| `new_vax` | second registered vaccine | `total` | ring 1 |
| `new_vax` | second registered vaccine | `age0_19` | ring 1 |
| *…* | second registered vaccine | *remaining ages* | ring 1 |
| *…* | second registered vaccine | *all ages* | *remaining rings* |
| *…* | *remaining registered vaccines* | *same age order* | *same ring order* |

## `now_variant` follows immediately

Variant ID 0 is `none`, variant ID 1 is `base`, and any later variants use
registration order.

| Kind | Subject | Age | Ring |
|---|---|---|---|
| `now_variant` | `none` (stored sentinel) | `total` | `RING_ALL` |
| `now_variant` | `none` (stored sentinel) | `age0_19` | `RING_ALL` |
| *…* | `none` | *remaining ages* | `RING_ALL` |
| `now_variant` | `none` (stored sentinel) | `total` | ring 1 |
| `now_variant` | `none` (stored sentinel) | `age0_19` | ring 1 |
| *…* | `none` | *remaining ages* | ring 1 |
| *…* | `none` | *all ages* | *remaining rings* |
| `now_variant` | `base` | `total` | `RING_ALL` |
| `now_variant` | `base` | `age0_19` | `RING_ALL` |
| *…* | `base` | *remaining ages* | `RING_ALL` |
| `now_variant` | `base` | `total` | ring 1 |
| `now_variant` | `base` | `age0_19` | ring 1 |
| *…* | `base` | *remaining ages* | ring 1 |
| *…* | `base` | *all ages* | *remaining rings* |
| `now_variant` | second real variant | `total` | `RING_ALL` |
| `now_variant` | second real variant | `age0_19` | `RING_ALL` |
| *…* | second real variant | *remaining ages* | `RING_ALL` |
| `now_variant` | second real variant | `total` | ring 1 |
| `now_variant` | second real variant | `age0_19` | ring 1 |
| *…* | second real variant | *remaining ages* | ring 1 |
| *…* | second real variant | *all ages* | *remaining rings* |
| *…* | *remaining registered variants* | *same age order* | *same ring order* |

## `new_variant` is the final run

| Kind | Subject | Age | Ring |
|---|---|---|---|
| `new_variant` | `none` (stored sentinel) | `total` | `RING_ALL` |
| `new_variant` | `none` (stored sentinel) | `age0_19` | `RING_ALL` |
| *…* | `none` | *remaining ages* | `RING_ALL` |
| `new_variant` | `none` (stored sentinel) | `total` | ring 1 |
| `new_variant` | `none` (stored sentinel) | `age0_19` | ring 1 |
| *…* | `none` | *remaining ages* | ring 1 |
| *…* | `none` | *all ages* | *remaining rings* |
| `new_variant` | `base` | `total` | `RING_ALL` |
| `new_variant` | `base` | `age0_19` | `RING_ALL` |
| *…* | `base` | *remaining ages* | `RING_ALL` |
| `new_variant` | `base` | `total` | ring 1 |
| `new_variant` | `base` | `age0_19` | ring 1 |
| *…* | `base` | *remaining ages* | ring 1 |
| *…* | `base` | *all ages* | *remaining rings* |
| `new_variant` | second real variant | `total` | `RING_ALL` |
| `new_variant` | second real variant | `age0_19` | `RING_ALL` |
| *…* | second real variant | *remaining ages* | `RING_ALL` |
| `new_variant` | second real variant | `total` | ring 1 |
| `new_variant` | second real variant | `age0_19` | ring 1 |
| *…* | second real variant | *remaining ages* | ring 1 |
| *…* | second real variant | *all ages* | *remaining rings* |
| *…* | *remaining registered variants* | *same age order* | *same ring order* |

The final `new_variant` ellipsis is followed by the end of `cols_`. Each
listed column contains its own inner day vector:
`[unused day 0, day 1, ..., day_cnt]`.
