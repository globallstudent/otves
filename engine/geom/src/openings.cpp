#include <algorithm>
#include <array>
#include <optional>
#include <span>

#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/wall.h>
#include <otves/geom/openings.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

#include "faces.h"

namespace otves::geom {
namespace {

// A wall's location line as an origin and unit directions along and to the left of it.
struct Axis {
  Vec2 start;
  Vec2 dir;
  Vec2 left;
  double length = 0.0;
};

Axis axis(const Wall& wall) {
  const Vec2 start = to_vec2(wall.start);
  const Vec2 run = to_vec2(wall.end) - start;
  const double run_length = length(run);
  const Vec2 dir = run * (1.0 / run_length);
  return {.start = start, .dir = dir, .left = perp(dir), .length = run_length};
}

}  // namespace

std::optional<WallHit> hit_wall(std::span<const WallInput> walls, Vec2 at, double reach) {
  std::optional<WallHit> best;
  double best_distance = 0.0;
  for (const WallInput& input : walls) {
    const Axis line = axis(input.wall);
    const Faces offsets = faces(input.wall);
    const double along = dot(at - line.start, line.dir);
    const double across = dot(at - line.start, line.left);
    const double outside_along = std::max({-along, 0.0, along - line.length});
    const double outside_across = std::max({-offsets.right - across, 0.0, across - offsets.left});
    const double distance = length({.x = outside_along, .y = outside_across});
    if (distance > reach) {
      continue;
    }
    const bool nearer =
        !best || distance < best_distance || (distance == best_distance && input.id < best->wall);
    if (nearer) {
      best = WallHit{.wall = input.id, .along = std::clamp(along, 0.0, line.length)};
      best_distance = distance;
    }
  }
  return best;
}

std::array<Vec2, 4> opening_outline(const Wall& wall, const Opening& opening) {
  const Axis line = axis(wall);
  const Faces offsets = faces(wall);
  const Vec2 near = line.start + (line.dir * to_metres(opening.offset));
  const Vec2 far = near + (line.dir * to_metres(opening.width));
  const Vec2 right = line.left * -offsets.right;
  const Vec2 left = line.left * offsets.left;
  return {near + right, far + right, far + left, near + left};
}

}  // namespace otves::geom
