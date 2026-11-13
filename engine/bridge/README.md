# engine/bridge

The engine's door to the Kotlin app: a `Session` that runs FlatBuffers requests from `schema/engine.fbs` against one document, and the JNI functions of `libdesign.so` that carry them ([ADR 0006](../../docs/adr/0006-engine-bridge-protocol.md)).

```mermaid
sequenceDiagram
  participant K as Kotlin (engine thread)
  participant J as libdesign.so
  participant S as Session
  participant D as core::Document
  K->>J: handle(session, Request in direct ByteBuffer)
  J->>S: handle(bytes)
  S->>S: Verifier, version check
  loop each command
    S->>D: execute / begin_gesture / undo
  end
  S->>D: take_dirty()
  S-->>J: Response bytes, owned until the next call
  J-->>K: direct ByteBuffer
```

## Entry points

| API | Use |
|---|---|
| `Session(device)` | One open job; the device id stamps its operations |
| `Session::handle(request)` | Runs a `Request`, returns a `Response` valid until the next call |
| `NativeEngine.create(high, low)` | JNI: a session handle for a device id |
| `NativeEngine.handle(session, buffer, size)` | JNI: one request in, one direct `ByteBuffer` out, or null for a bad handle or buffer |
| `NativeEngine.destroy(session)` | JNI: frees the session |

## Response

| Field | Content |
|---|---|
| `results` | One per command, in order: `ErrorCode`, an English message for logs, the created element's id, the element a `Pick` found, where an `Aim` or `AimMeasured` says a wall would end |
| `changes` | Each changed level, wall or opening with its full value, or with no value when deleted; sorted by id |
| `can_undo`, `can_redo` | For the toolbar |
| `outlines` | Every wall's joined plan outline in metres, level by level; only when the model or the content changed |
| `takeoff`, `takeoff_error` | Material lines from the loaded rules, or why they could not run; absent until `LoadContent` |

`Pick`, `Aim` and `AimMeasured` are questions: they read the model and change nothing. `SetWallLength` asks `geom::resize_wall` what moves and runs it as `MoveWallEnd` commands in one undo step ([ADR 0011](../../docs/adr/0011-plan-editing-in-the-engine.md)).

`LoadContent` carries the Luau norms and rules from the app's assets. A bundle that does not compile fails that command and keeps the content that ran before.

A request that fails the `Verifier` or names another version gets a single `CorruptData` or `UnsupportedVersion` result and changes nothing.

## Android build

`android/engine-bridge` builds the `otves_design` target with the engine's own `CMakeLists.txt`: vcpkg as the toolchain, the arm64-android triplet, and NDK r30 chained in from the `ANDROID_NDK` that Gradle passes. The AAR carries `jni/arm64-v8a/libdesign.so`.

On the Kotlin side, `generateSchemaJava` downloads flatc 25.2.10 (SHA-256 pinned, matching `flatbuffers-java`) and generates `uz.otves.schema`. `EngineSession.send(commands)` encodes `EngineCommand`s, runs them and decodes an `EngineResponse` of typed results and element changes. `EngineSessionTest` runs that path against the real `libdesign.so` on an arm64 device or emulator.

## Invariants

- A session is used from one thread. JNI functions and `handle` are `noexcept`; an exception becomes an `Internal` result.
- Commands run in order. One that fails changes nothing, and the rest still run.
- Enums map explicitly both ways. An unknown value from a newer app is `CorruptData`, never a wrong value.
- `flatc` generates the C++ code at build time from `schema/engine.fbs`; nothing generated is committed.
