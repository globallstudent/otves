#pragma once

#include <cmath>

#include <otves/core/point.h>
#include <otves/core/units.h>

namespace otves::geom {

// A point or a direction on the plan, in metres: x to the right, y up.
struct Vec2 {
  double x = 0.0;
  double y = 0.0;
};

[[nodiscard]] constexpr Vec2 operator+(Vec2 a, Vec2 b) { return {.x = a.x + b.x, .y = a.y + b.y}; }
[[nodiscard]] constexpr Vec2 operator-(Vec2 a, Vec2 b) { return {.x = a.x - b.x, .y = a.y - b.y}; }
[[nodiscard]] constexpr Vec2 operator-(Vec2 v) { return {.x = -v.x, .y = -v.y}; }
[[nodiscard]] constexpr Vec2 operator*(Vec2 v, double k) { return {.x = v.x * k, .y = v.y * k}; }
[[nodiscard]] constexpr Vec2 operator*(double k, Vec2 v) { return v * k; }

[[nodiscard]] constexpr double dot(Vec2 a, Vec2 b) { return (a.x * b.x) + (a.y * b.y); }

// Positive when b turns counter-clockwise from a.
[[nodiscard]] constexpr double cross(Vec2 a, Vec2 b) { return (a.x * b.y) - (a.y * b.x); }

// v turned 90° counter-clockwise: the left-hand normal of a direction.
[[nodiscard]] constexpr Vec2 perp(Vec2 v) { return {.x = -v.y, .y = v.x}; }

// sqrt is correctly rounded on every platform; hypot is not.
[[nodiscard]] inline double length(Vec2 v) { return std::sqrt(dot(v, v)); }

[[nodiscard]] constexpr Vec2 to_vec2(PointMm point) {
  return {.x = to_metres(point.x), .y = to_metres(point.y)};
}

}  // namespace otves::geom
