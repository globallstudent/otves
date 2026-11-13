#pragma once

#include <cstdint>

#include <otves/core/point.h>

#include "step.h"

namespace otves::geom {

// A free wall turns onto an axis when it strays from it by no more than a tenth of its run,
// about 6°.
inline constexpr std::int64_t kAxisCatch = 10;

// Which axis a wall is held to.
enum class Held : std::uint8_t {
  kFree,
  kAlongX,
  kAlongY,
};

// The axis a wall from `from` to `to` is held to: the nearer one, always when `square` and
// otherwise only within kAxisCatch of it.
[[nodiscard]] constexpr Held held(PointMm from, PointMm to, bool square) {
  const StepMm run = step(from, to);
  const std::int64_t across_x = magnitude(run.y);
  const std::int64_t across_y = magnitude(run.x);
  if (across_y >= across_x) {
    return square || across_x * kAxisCatch <= across_y ? Held::kAlongX : Held::kFree;
  }
  return square || across_y * kAxisCatch <= across_x ? Held::kAlongY : Held::kFree;
}

}  // namespace otves::geom
