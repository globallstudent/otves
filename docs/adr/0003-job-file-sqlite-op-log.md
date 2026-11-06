# 0003. Store each job as one SQLite file with an operation log

Status: accepted    Date: 2026-11-06

## Context

A builder works offline on several jobs and shares a job by sending it. Undo, history and later multi-device sync all need the same record of changes. Geometry can always be rebuilt from parameters.

## Decision

Each job is one SQLite file (`<job>.job`) holding parts, levels, element parameters, an operation log, estimate snapshots and the ledger. Geometry is never stored. Rows use UUIDv7 ids; each operation carries a hybrid logical clock and a device id. Materials and prices live in a separate app-wide `catalog.db`.

## Consequences

- Opening, sharing and backing up a job is one file; undo and history come from the log.
- Sync can merge per property, last writer wins, without changing the file format.
- Opening a large job must rebuild geometry quickly (budget: under 1 s for 5 000 elements).
