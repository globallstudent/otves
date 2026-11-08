#pragma once

#include <span>

#include <otves/geom/vec2.h>

namespace otves::geom {

// Shoelace area in m²: positive for a counter-clockwise outline, negative for clockwise.
[[nodiscard]] double signed_area(std::span<const Vec2> outline);

}  // namespace otves::geom
