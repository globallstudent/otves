#pragma once

#include <expected>

#include <flatbuffers/flatbuffers.h>

#include <otves/core/command.h>
#include <otves/core/error.h>
#include <otves/core/model.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>

#include "engine_generated.h"

// Conversions between schema types and core types. Every enum maps explicitly, so a value from
// an older or newer app never turns into the wrong one by cast.
namespace otves::bridge {

[[nodiscard]] schema::Uuid to_schema(ElementId id);
[[nodiscard]] ElementId to_id(const schema::Uuid* uuid);  // nil for an absent id
[[nodiscard]] schema::ErrorCode to_schema(ErrorCode code);
[[nodiscard]] schema::OpeningKind to_schema(OpeningKind kind);

// The core command in a schema entry, or kCorruptData when a field it needs is missing or an
// enum holds an unknown value. Gesture and undo/redo entries are not core commands.
[[nodiscard]] std::expected<Command, Error> decode(const schema::CommandEntry& entry);

// A tap that places an opening (schema PlaceOpening); the opening's wall and offset are unset.
struct OpeningPlacement {
  ElementId level;
  PointMm at;
  Mm reach;
  Opening opening;
};

// kCorruptData when the point is missing or the kind is unknown.
[[nodiscard]] std::expected<OpeningPlacement, Error> decode(const schema::PlaceOpening* place);

// The element's current value, or its deletion when the model no longer holds it.
[[nodiscard]] flatbuffers::Offset<schema::ElementChange> encode_change(
    flatbuffers::FlatBufferBuilder& builder, const Model& model, ElementId id);

}  // namespace otves::bridge
