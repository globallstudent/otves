# 0010. A strip footing is a section the builder gives a storey

Status: accepted    Date: 2026-11-10

## Context

A house estimate without its foundation is half an estimate. Private builders in Uzbekistan and Russia put a strip footing (ленточный фундамент) under the load-bearing walls and decide its section from experience and the soil; the norms size it from bearing pressure, which the app does not know.

## Decision

- A level carries `footing_width` and `footing_depth` in millimetres, both zero for none, as it carries its ring beam (ADR 0008). The footing runs under the level's load-bearing walls, along their length.
- The app never sizes it. The rule counts what the section implies: concrete with placement losses, longitudinal bars at the minimum reinforcement of СП 63 (never fewer than four), stirrups at the pitch СП 63 allows, formwork on both sides.
- Excavation, the sand cushion, blinding and waterproofing are separate choices to add later.

## Consequences

- Bridge schema: `Level` gains two fields with zero defaults; one command and one command kind are appended. Job file version 4: two columns on `level`, zero by migration.
- Where walls meet, a footing wider than its wall overlaps the next one; the rule measures along the wall, so a corner is counted a little short. Good enough for ordering; a footing drawn as its own element will be exact.
- The take-off is a guide to quantities, not a structural design. The sheet must say so before it is handed to a client.
