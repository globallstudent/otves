#pragma once

#include <array>
#include <optional>
#include <span>

#include <otves/core/opening.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

namespace otves::geom {

// A point on a wall: which wall, and how far along its location line from the start, in metres,
// within [0, length].
struct WallHit {
  ElementId wall;
  double along = 0.0;
};

// The wall whose body lies nearest to `at`, if one lies within `reach` metres. A wall's body is
// its rectangle from start to end between its faces, before joining. Ties go to the smaller id,
// so the result does not depend on the order of `walls`.
// Precondition: every wall passes validate().
[[nodiscard]] std::optional<WallHit> hit_wall(std::span<const WallInput> walls, Vec2 at,
                                              double reach);

// The rectangle an opening cuts through its wall on the plan, counter-clockwise, in metres: from
// `offset` to `offset + width` along the location line, face to face.
// Precondition: the wall passes validate().
[[nodiscard]] std::array<Vec2, 4> opening_outline(const Wall& wall, const Opening& opening);

}  // namespace otves::geom
