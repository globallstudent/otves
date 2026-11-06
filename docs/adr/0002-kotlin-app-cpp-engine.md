# 0002. Split the app into a Kotlin shell and a C++ engine

Status: accepted    Date: 2026-11-06

## Context

The editor must redraw plans at 120 fps and 3D at 60 fps while a wall is dragged. Filament, Manifold, Clipper2, HarfBuzz and msdfgen are C++ libraries; Kotlin has no robust 3D booleans and adds GC pauses on the hot path.

## Decision

Kotlin and Jetpack Compose own screens, business data, files and devices. A C++23 engine owns the model, geometry, quantities and all drawing. They talk through batched FlatBuffers messages over JNI: commands in, events out once per frame. Viewport touches go straight to the engine thread.

## Consequences

- One model and one selection serve plan, section and 3D; geometry is testable on a desktop in seconds.
- Two toolchains to maintain, and a strict bridge contract that needs versioning and verification.
