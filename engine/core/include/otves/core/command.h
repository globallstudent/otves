#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include <otves/core/conditions.h>
#include <otves/core/level.h>
#include <otves/core/op.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/room.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/core/wall_type.h>

namespace otves {

// User intents. Document validates a command against the model and turns it into ops.

struct AddLevel {
  Level level;
};

struct SetLevelElevation {
  ElementId level;
  Mm elevation;
};

struct SetLevelHeight {
  ElementId level;
  Mm height;
};

// The height of the ring beam on the level's load-bearing walls; zero removes it.
struct SetLevelRingBeam {
  ElementId level;
  Mm height;
};

struct SetLevelJunctionMeshes {
  ElementId level;
  bool meshes = false;
};

// The section of the strip footing under the level's load-bearing walls; both zero removes it.
struct SetLevelFooting {
  ElementId level;
  Mm width;
  Mm depth;
};

// Removes the level with the names of its rooms, every wall on it and every opening in those
// walls.
struct RemoveLevel {
  ElementId level;
};

// The wall's level must exist.
struct AddWall {
  Wall wall;
};

struct MoveWallEnd {
  ElementId wall;
  WallEnd end = WallEnd::kEnd;
  PointMm to;
};

struct SetWallThickness {
  ElementId wall;
  Mm thickness;
};

struct SetWallHeight {
  ElementId wall;
  Mm height;
};

struct SetWallLocationLine {
  ElementId wall;
  LocationLine location_line = LocationLine::kCenter;
};

// The masonry unit as laid; all zero clears the choice.
struct SetWallUnit {
  ElementId wall;
  Mm length;
  Mm width;
  Mm height;
};

// Whether the wall's length was typed or measured (docs/adr/0012).
struct SetWallExact {
  ElementId wall;
  bool exact = true;
};

// A new kind of wall with its layers from the outer face in (docs/adr/0014). The layers' type
// and position are not read: the type is the new one and the positions follow the order given.
struct AddWallType {
  WallRole role = WallRole::kExterior;
  std::vector<WallLayer> layers;
};

struct SetWallTypeRole {
  ElementId type;
  WallRole role = WallRole::kExterior;
};

// A layer's thickness and, for masonry, its unit as laid; all zero clears the unit. Every wall
// of the layer's type changes with it.
struct SetWallLayer {
  ElementId layer;
  Mm thickness;
  Mm unit_length;
  Mm unit_width;
  Mm unit_height;
};

// Makes the wall one of its type's kind. The wall's own thickness and unit are dropped.
struct SetWallType {
  ElementId wall;
  ElementId type;
};

// Which face of the wall its type's layers start from.
struct SetWallMirrored {
  ElementId wall;
  bool mirrored = false;
};

// Removes the wall and every opening in it.
struct RemoveWall {
  ElementId wall;
};

// The opening's wall must exist.
struct AddOpening {
  Opening opening;
};

struct MoveOpening {
  ElementId opening;
  Mm offset;
};

struct ResizeOpening {
  ElementId opening;
  Mm width;
  Mm height;
};

struct SetOpeningSill {
  ElementId opening;
  Mm sill;
};

struct SetOpeningKind {
  ElementId opening;
  OpeningKind kind = OpeningKind::kWindow;
};

// Which edge a door's leaf hangs on (docs/adr/0017).
struct SetOpeningHinge {
  ElementId opening;
  Hinge hinge = Hinge::kNear;
};

// Which side of its wall a door's leaf opens to.
struct SetOpeningSwing {
  ElementId opening;
  Swing swing = Swing::kLeft;
};

struct RemoveOpening {
  ElementId opening;
};

// Names the space of the room's level that holds its anchor (docs/adr/0015). The level must
// exist; whether the anchor lies in a space is not checked here.
struct AddRoom {
  Room room;
};

struct MoveRoom {
  ElementId room;
  PointMm anchor;
};

struct SetRoomKind {
  ElementId room;
  RoomKind kind = RoomKind::kRoom;
};

// Takes the name away; the space stays.
struct RemoveRoom {
  ElementId room;
};

// Sets one of the job's estimating conditions, creating them when the job has none
// (docs/adr/0009). Conditions::kUnset returns the value to the norm default.
struct SetCondition {
  ConditionsField field = ConditionsField::kSeismicIntensity;
  std::int64_t value = Conditions::kUnset;
};

using Command =
    std::variant<AddLevel, SetLevelElevation, SetLevelHeight, RemoveLevel, AddWall, MoveWallEnd,
                 SetWallThickness, SetWallHeight, SetWallLocationLine, RemoveWall, AddOpening,
                 MoveOpening, ResizeOpening, SetOpeningSill, SetOpeningKind, RemoveOpening,
                 SetWallUnit, SetLevelRingBeam, SetLevelJunctionMeshes, SetCondition,
                 SetLevelFooting, SetWallExact, AddWallType, SetWallTypeRole, SetWallLayer,
                 SetWallType, SetWallMirrored, AddRoom, MoveRoom, SetRoomKind, RemoveRoom,
                 SetOpeningHinge, SetOpeningSwing>;

[[nodiscard]] CommandKind kind_of(const Command& command);

}  // namespace otves
