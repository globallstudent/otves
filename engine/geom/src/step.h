#pragma once

#include <cstdint>

#include <otves/core/point.h>
#include <otves/core/units.h>

namespace otves::geom {

// A direction or a move on the plan in whole millimetres. Directions are sorted and tested for
// parallelism on these exactly, so results never depend on how a platform rounds floating point.
struct StepMm {
  std::int64_t x = 0;
  std::int64_t y = 0;

  constexpr bool operator==(const StepMm&) const = default;
};

[[nodiscard]] constexpr StepMm step(PointMm from, PointMm to) {
  return {.x = (to.x - from.x).value(), .y = (to.y - from.y).value()};
}
[[nodiscard]] constexpr StepMm reversed(StepMm step) { return {.x = -step.x, .y = -step.y}; }
[[nodiscard]] constexpr PointMm moved(PointMm point, StepMm by) {
  return {.x = point.x + Mm{by.x}, .y = point.y + Mm{by.y}};
}
// Coordinates stay within kMaxCoordinate, so these products fit in int64.
[[nodiscard]] constexpr std::int64_t cross(StepMm lhs, StepMm rhs) {
  return (lhs.x * rhs.y) - (lhs.y * rhs.x);
}
[[nodiscard]] constexpr std::int64_t dot(StepMm lhs, StepMm rhs) {
  return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}
[[nodiscard]] constexpr int sign(std::int64_t value) {
  return static_cast<int>(value > 0) - static_cast<int>(value < 0);
}

}  // namespace otves::geom
