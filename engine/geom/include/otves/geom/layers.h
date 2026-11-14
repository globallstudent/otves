#pragma once

#include <span>
#include <vector>

#include <otves/core/units.h>
#include <otves/core/wall.h>
#include <otves/geom/vec2.h>

// A wall's layers as geometry (docs/adr/0014): strips of its joined outline along the wall.
namespace otves::geom {

// The part of a wall between two lines along it: `from` and `to` metres to the left of its
// location line, negative to the right. `from` is less than `to`.
struct Strip {
  double from = 0.0;
  double to = 0.0;
};

// Where layers of the given thicknesses lie across the wall, from its outer face in: its right
// face, looking from start to end, or its left one when the wall is mirrored. They cover the
// wall from face to face when the thicknesses add up to its thickness.
// Precondition: the wall passes validate().
[[nodiscard]] std::vector<Strip> layer_strips(const Wall& wall, std::span<const Mm> thicknesses);

// The part of the wall's joined outline inside the strip, counter-clockwise; empty when none of
// it lies there. A corner cut by a mitre gives each layer its own share of the corner.
// Precondition: the wall passes validate() and `outline` is its footprint from join_walls.
[[nodiscard]] std::vector<Vec2> clip(const Wall& wall, std::span<const Vec2> outline, Strip strip);

}  // namespace otves::geom
