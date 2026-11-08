#include <algorithm>
#include <array>
#include <optional>
#include <span>

#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
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

// How far `at` lies from the part of a wall between `from` and `to` along its location line,
// face to face, and where along the line it falls.
struct Reach {
  double distance = 0.0;
  double along = 0.0;
};

Reach reach_of(const Wall& wall, double from, double to, Vec2 at) {
  const Axis line = axis(wall);
  const Faces offsets = faces(wall);
  const double along = dot(at - line.start, line.dir);
  const double across = dot(at - line.start, line.left);
  const double outside_along = std::max({from - along, 0.0, along - to});
  const double outside_across = std::max({-offsets.right - across, 0.0, across - offsets.left});
  return {.distance = length({.x = outside_along, .y = outside_across}), .along = along};
}

}  // namespace

std::optional<WallHit> hit_wall(std::span<const WallInput> walls, Vec2 at, double reach) {
  std::optional<WallHit> best;
  double best_distance = 0.0;
  for (const WallInput& input : walls) {
    const double wall_length = axis(input.wall).length;
    const auto [distance, along] = reach_of(input.wall, 0.0, wall_length, at);
    if (distance > reach) {
      continue;
    }
    const bool nearer =
        !best || distance < best_distance || (distance == best_distance && input.id < best->wall);
    if (nearer) {
      best = WallHit{.wall = input.id, .along = std::clamp(along, 0.0, wall_length)};
      best_distance = distance;
    }
  }
  return best;
}

std::optional<ElementId> hit_opening(std::span<const PlacedOpening> openings, Vec2 at,
                                     double reach) {
  std::optional<ElementId> best;
  double best_distance = 0.0;
  for (const PlacedOpening& placed : openings) {
    const double from = to_metres(placed.opening.offset);
    const double to = from + to_metres(placed.opening.width);
    const double distance = reach_of(placed.wall, from, to, at).distance;
    if (distance > reach) {
      continue;
    }
    if (!best || distance < best_distance || (distance == best_distance && placed.id < *best)) {
      best = placed.id;
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
