#pragma once

#include <variant>

#include <otves/core/level.h>
#include <otves/core/op.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

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

// Removes the level with every wall on it and every opening in those walls.
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

struct RemoveOpening {
  ElementId opening;
};

using Command =
    std::variant<AddLevel, SetLevelElevation, SetLevelHeight, RemoveLevel, AddWall, MoveWallEnd,
                 SetWallThickness, SetWallHeight, SetWallLocationLine, RemoveWall, AddOpening,
                 MoveOpening, ResizeOpening, SetOpeningSill, SetOpeningKind, RemoveOpening>;

[[nodiscard]] CommandKind kind_of(const Command& command);

}  // namespace otves
