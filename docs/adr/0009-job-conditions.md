# 0009. Estimating conditions are an element of the job

Status: accepted    Date: 2026-11-10

## Context

Rules read job settings: seismic intensity, cement and mortar grades, waste allowances. Until now the engine always passed none, so every job ran on `norms/practice` defaults and the builder could not use his own mix or allowance. Settings change quantities, so they must be saved, undone and synced like the rest of the model (rule 2 of the architecture).

## Decision

- A job holds at most one `Conditions` element: `seismic_intensity` (points), `cement_grade` (400 for ПЦ400), `mortar_grade` (100 for М100), `unit_waste` and `mortar_waste` (per mille). `-1` leaves a value to the norm default.
- `SetCondition { field, value }` names no id: it creates the element when the job has none, else sets the field. Create and set-field ops, undo, the log and `OpenModel` treat it as any element.
- The bridge turns it into rule settings (`cement = "PC400"`, `unit_waste = 0.03`). The seismic advice of ADR 0008 uses its intensity.
- The values offered come from `content/catalog/conditions.json`, with the defaults of `norms/practice` shown as chosen while a value is unset.

## Consequences

- Bridge schema: `Conditions` joins the `Element` union; one command, one command kind and one op table are appended.
- Job file version 3: a `conditions` table, empty in older jobs.
- A new setting is a new field through every layer. That is deliberate: each is typed, validated and migrated; free-form keys would let the app and the rules drift apart.
