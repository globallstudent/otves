# content

Norm data and rules that turn take-off quantities into material lines. They are Luau scripts that `engine/takeoff` runs in a sandbox ([ADR 0005](../docs/adr/0005-norm-rules-in-sandboxed-luau.md)), so a norm update ships without a new APK.

```mermaid
flowchart LR
  N["norms/*.luau<br/>tables of values"] -- "norms.&lt;file name&gt;" --> R["rules/*.luau<br/>run(input, norms, settings)"]
  Q["engine/takeoff<br/>quantities, masonry"] -- "input" --> R
  S["job settings"] -- "settings" --> R
  R -- "{ item, unit, amount }" --> L["quantity lines"]
```

## Layout

| Path | Holds |
|---|---|
| `norms/<role>.luau` | One table of values with a `source` record: standard, clause, jurisdiction, status. The file is named by its role (`mortar`, `seismic`), not by its standard, so a norm profile (UZ, RU, EU) swaps the file and rules stay the same. The name is its key in `norms` |
| `rules/<name>.luau` | `return { run = function(input, norms, settings) ... end }`, returning an array of lines |

## Rule contract

| Value | Fields |
|---|---|
| `input.walls[i]` | `id`, `thickness` m, `footprint_area` m², `length` m, `face_area` m² and `volume` m³ net of openings, `openings_area` m² |
| `input.lintels[i]` | `opening`, `wall`, `length` m to order, `height` m, `thickness` m |
| `input.masonry` | `units`, `mortar` m³, `walls[i]` with `id`, `units`, `mortar` |
| `settings` | Job settings by name, numbers or strings; absent ones are `nil` |
| line | `item` (`a-z 0-9 _ .`, ≤ 64), `unit` (`pcs`, `m`, `m2`, `m3`, `kg`), `amount` (finite, ≥ 0) |

The engine adds no waste and no rounding; the rule does both. One rule may not return the same item and unit twice.

## Writing rules

- Available: `base` without `print`, `pcall` and `getfenv`, plus `table`, `string` and the exact part of `math` (`abs ceil clamp floor fmod max min modf round sign sqrt`).
- Not available: `os`, `debug`, `math.random`, trigonometry, `exp`, `log`. Write `x * x`, not `x ^ 2`: `^` calls the platform `pow`, whose last bit differs between phones and CI.
- Norms, input, settings and library tables are read-only. Globals set by one rule are invisible to the others.
- Each script gets 1 000 000 steps; a run gets 32 MiB.

## Files

| File | Content | Source |
|---|---|---|
| `norms/mortar.luau` | Cement per m³ of masonry mortar by cement and mortar grade | СП 82-101-98, tables 4–5 |
| `norms/practice.luau` | Waste allowances (запас), default cement and mortar grade, lintel bar 2ПБ 120 × 140 | Practice; ГОСТ 948-2016 table 1; editable in job settings |
| `norms/seismic.luau` | Lintel bearing: 350 mm, 250 mm up to a 1 500 mm opening | КМК 2.01.03-19, 3.5.15 |
| `rules/masonry.luau` | Units and mortar with waste, cement in that mortar | `mortar`, `practice` |
| `rules/lintels.luau` | Bar lintels by length: `lintel.bar.l22` is a bar at least 2.2 m long, as many side by side as the wall is thick | `practice` |

The engine also reads `seismic.lintel` and `practice.lintel.height` itself to place lintels, through `lintel_spec`. GOST 948 marks are not chosen yet: their length table is not verified.
