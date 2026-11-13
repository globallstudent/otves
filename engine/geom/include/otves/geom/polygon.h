#pragma once

#include <span>

#include <otves/geom/vec2.h>

namespace otves::geom {

// Shoelace area in m²: positive for a counter-clockwise outline, negative for clockwise.
[[nodiscard]] double signed_area(std::span<const Vec2> outline);

// Whether `point` lies inside the outline, by counting the edges a ray to the right crosses. A
// point on an edge may fall either way.
[[nodiscard]] bool contains(std::span<const Vec2> outline, Vec2 point);

}  // namespace otves::geom
