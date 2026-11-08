# 0005. Write norm rules as sandboxed Luau scripts in content

Status: accepted    Date: 2026-11-08

## Context

Norms change between app releases: СП 70 got its 8th amendment in June 2026, and decree 727 orders a КМК 2.01.03 update. Rules need logic (lookups, conditions, rounding), not only numbers. Content bundles ship on their own schedule, so their scripts must not reach files, network or clocks, and must give the same bits on arm64 and x86-64.

## Decision

- `content/norms/*.luau` return plain tables of values with their source. `content/rules/*.luau` return `{ run = function(input, norms, settings) }` and produce quantity lines `{ item, unit, amount }`.
- `engine/takeoff` runs them in Luau 0.746:
  - a fresh state per run;
  - only `base`, `table`, `string` and the exact part of `math`;
  - no `os`, `debug`, `print`, `pcall`, `getfenv` or `math.random`;
  - read-only globals, norms, input and settings;
  - a step budget and a memory cap.
- Scripts compile at optimisation level 0, so builtin fast paths cannot bypass the removed functions.
- The engine gets script text from Kotlin, which reads the content bundle. It never opens files itself.
- Output lines are validated, then sorted by item and unit.

## Consequences

- A norm update is a content bundle, not an APK. Rules are tested on the desktop against real quantities.
- Content authors write a second language. Transcendental functions are gone, and `^` calls the platform `pow`, so rules multiply instead.
- Luau releases weekly. The vcpkg baseline lags behind, so `engine/ports/luau` pins the version until the baseline catches up.
