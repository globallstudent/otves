# 0001. Keep the app, engine and content in one repository

Status: accepted    Date: 2026-11-06

## Context

The Kotlin app and the C++ engine share one contract (FlatBuffers schemas) and change together. The team is one to three people, and the engine ships only inside the APK.

## Decision

One monorepo with `android/`, `engine/`, `schema/` and `content/`. Each part keeps its own build (Gradle, CMake + vcpkg) and its own versioning where it ships separately (content bundles).

## Consequences

- A contract change and both sides of it land in one commit series; one CI.
- Split only on a real trigger: a separate team, a server, or publishing the engine on its own.
