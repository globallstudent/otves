#pragma once

#include <cstdint>
#include <variant>

#include <otves/core/op.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

namespace otves {

// User intents. Document validates a command against the model and turns it into ops.

struct AddWall {
  Wall wall;
};

enum class WallEnd : std::uint8_t {
  kStart,
  kEnd,
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

struct RemoveWall {
  ElementId wall;
};

using Command = std::variant<AddWall, MoveWallEnd, SetWallThickness, SetWallHeight,
                             SetWallLocationLine, RemoveWall>;

[[nodiscard]] CommandKind kind_of(const Command& command);

}  // namespace otves
