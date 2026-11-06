# 0004. Store measured lengths as integer millimetres and money as integer minor units

Status: accepted    Date: 2026-11-06

## Context

Builders measure to the millimetre and expect 6 400 to stay 6 400 after any number of edits. Estimates sum thousands of priced lines; floating point drifts and rounds differently on each device.

## Decision

Lengths typed or measured on site are `Mm` (64-bit integer millimetres) in both Kotlin and C++. Geometry converts to `double` metres internally and rounds back with `round_to_mm`. Money is `Money(minor: Long, currency)`; it never enters the engine and never uses floating point.

## Consequences

- Stored values are exact and identical across devices; golden tests compare integers.
- Conversions are explicit, and fractional quantities need a defined rounding rule when they are priced.
