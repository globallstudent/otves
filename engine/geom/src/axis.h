#pragma once

#include <otves/core/wall.h>
#include <otves/geom/vec2.h>

#include "faces.h"

namespace otves::geom {

// Inputs sit on a 1 mm grid, so distances below 1 µm are rounding noise.
inline constexpr double kEpsilon = 1e-6;

// A wall's location line as an origin and unit directions along and to the left of it.
struct Axis {
  Vec2 start;
  Vec2 dir;
  Vec2 left;
  double length = 0.0;
};

// Precondition: the wall passes validate(), so it has a length.
[[nodiscard]] inline Axis axis(const Wall& wall) {
  const Vec2 start = to_vec2(wall.start);
  const Vec2 run = to_vec2(wall.end) - start;
  const double run_length = length(run);
  const Vec2 dir = run * (1.0 / run_length);
  return {.start = start, .dir = dir, .left = perp(dir), .length = run_length};
}

// Whether a wall's body holds `point` away from both of its ends: between its faces, and
// strictly between its start and its end. A wall end that lies here stands against this wall.
// Joining and editing share this one definition (docs/adr/0011).
[[nodiscard]] constexpr bool holds(const Axis& line, const Faces& offsets, Vec2 point) {
  const double along = dot(point - line.start, line.dir);
  const double across = dot(point - line.start, line.left);
  return along > kEpsilon && along < line.length - kEpsilon && across <= offsets.left + kEpsilon &&
         across >= -offsets.right - kEpsilon;
}

}  // namespace otves::geom
