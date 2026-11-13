#pragma once

#include <expected>
#include <optional>
#include <variant>

#include <flatbuffers/flatbuffers.h>

#include <otves/core/command.h>
#include <otves/core/document.h>
#include <otves/core/error.h>
#include <otves/core/level.h>
#include <otves/core/model.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/editing.h>

#include "engine_generated.h"

// Conversions between schema types and core types. Every enum maps explicitly, so a value from
// an older or newer app never turns into the wrong one by cast.
namespace otves::bridge {

[[nodiscard]] schema::Uuid to_schema(Uuid uuid);
[[nodiscard]] schema::Uuid to_schema(ElementId id);
[[nodiscard]] Uuid to_uuid(const schema::Uuid* uuid);     // nil for an absent id
[[nodiscard]] ElementId to_id(const schema::Uuid* uuid);  // nil for an absent id
[[nodiscard]] schema::ErrorCode to_schema(ErrorCode code);
[[nodiscard]] schema::OpeningKind to_schema(OpeningKind kind);
[[nodiscard]] schema::LocationLine to_schema(LocationLine line);
[[nodiscard]] std::optional<OpeningKind> from_schema(schema::OpeningKind kind);
[[nodiscard]] std::optional<LocationLine> from_schema(schema::LocationLine line);
[[nodiscard]] schema::PointMm to_schema(PointMm point);
[[nodiscard]] PointMm to_point(const schema::PointMm& point);

// The core command in a schema entry, or kCorruptData when a field it needs is missing or an
// enum holds an unknown value. Gesture and undo/redo entries are not core commands.
[[nodiscard]] std::expected<Command, Error> decode(const schema::CommandEntry& entry);

// A tap on the plan of a level, with how far around it still counts as touched.
struct Tap {
  ElementId level;
  PointMm at;
  Mm reach;
};

// kCorruptData when the point is missing.
[[nodiscard]] std::expected<Tap, Error> decode(const schema::Pick* pick);

// A tap that places an opening (schema PlaceOpening); the opening's wall and offset are unset.
struct OpeningPlacement {
  ElementId level;
  PointMm at;
  Mm reach;
  Opening opening;
};

// kCorruptData when the point is missing or the kind is unknown.
[[nodiscard]] std::expected<OpeningPlacement, Error> decode(const schema::PlaceOpening* place);

// A touch on a level while a wall is drawn (schema Aim).
struct Sight {
  ElementId level;
  geom::Aim aim;
};

// kCorruptData when the point under the finger is missing.
[[nodiscard]] std::expected<Sight, Error> decode(const schema::Aim* aim);

// A wall of a typed length on a level (schema AimMeasured).
struct MeasuredSight {
  ElementId level;
  geom::Measured measured;
};

// kCorruptData when the wall's start is missing or the turn is unknown.
[[nodiscard]] std::expected<MeasuredSight, Error> decode(const schema::AimMeasured* aim);

// A wall and the length it gets (schema SetWallLength).
struct Resize {
  ElementId wall;
  Mm length;
  std::optional<WallEnd> fixed;  // absent: the plan says which end stays
};

// kCorruptData when the fixed end is unknown.
[[nodiscard]] std::expected<Resize, Error> decode(const schema::SetWallLength* set);

// A schema element as a union member: its type and its table.
struct EncodedElement {
  schema::Element type = schema::Element::NONE;
  flatbuffers::Offset<void> value;
};

[[nodiscard]] EncodedElement encode(flatbuffers::FlatBufferBuilder& builder, const Level& level);
[[nodiscard]] EncodedElement encode(flatbuffers::FlatBufferBuilder& builder, const Wall& wall);
[[nodiscard]] EncodedElement encode(flatbuffers::FlatBufferBuilder& builder,
                                    const Opening& opening);
[[nodiscard]] EncodedElement encode(flatbuffers::FlatBufferBuilder& builder,
                                    const Conditions& conditions);

[[nodiscard]] schema::ConditionsField to_schema(ConditionsField field);
[[nodiscard]] std::optional<ConditionsField> from_schema(schema::ConditionsField field);

using Element = std::variant<Level, Wall, Opening, Conditions>;

// The element in a schema union, or kCorruptData when it is absent or holds an unknown value.
[[nodiscard]] std::expected<Element, Error> decode(schema::Element type, const void* value);

// The saved elements of an OpenModel command; Document::open checks them.
[[nodiscard]] std::expected<Snapshot, Error> decode(const schema::OpenModel* open);

// The element's current value, or its deletion when the model no longer holds it.
[[nodiscard]] flatbuffers::Offset<schema::ElementChange> encode_change(
    flatbuffers::FlatBufferBuilder& builder, const Model& model, ElementId id);

}  // namespace otves::bridge
