#pragma once

#include <otves/core/units.h>
#include <otves/core/wall.h>

namespace otves::geom {

// How far each face of a wall lies from its location line, in metres, looking from start to end.
struct Faces {
  double left = 0.0;
  double right = 0.0;
};

[[nodiscard]] constexpr Faces faces(const Wall& wall) {
  const double thickness = to_metres(wall.thickness);
  switch (wall.location_line) {
    case LocationLine::kCenter:
      break;
    case LocationLine::kLeftFace:
      return {.left = 0.0, .right = thickness};
    case LocationLine::kRightFace:
      return {.left = thickness, .right = 0.0};
  }
  return {.left = thickness / 2, .right = thickness / 2};
}

}  // namespace otves::geom
