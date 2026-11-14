#pragma once

#include <optional>
#include <vector>

#include <otves/core/units.h>
#include <otves/core/wall.h>
#include <otves/geom/faces.h>
#include <otves/geom/vec2.h>

#include "axis.h"
#include "step.h"

namespace otves::geom {

// Directions in [0°, 180°).
[[nodiscard]] constexpr bool upper_half(StepMm step) {
  return step.y > 0 || (step.y == 0 && step.x > 0);
}

// Counter-clockwise order from +x. Equal directions are neither before nor after each other.
[[nodiscard]] constexpr bool turns_before(StepMm lhs, StepMm rhs) {
  if (upper_half(lhs) != upper_half(rhs)) {
    return upper_half(lhs);
  }
  return cross(lhs, rhs) > 0;
}

struct Line {
  Vec2 point;
  Vec2 dir;
};

[[nodiscard]] inline std::optional<Vec2> intersect(const Line& lhs, const Line& rhs) {
  const double denominator = cross(lhs.dir, rhs.dir);
  if (denominator == 0.0) {
    return std::nullopt;
  }
  return lhs.point + (lhs.dir * (cross(rhs.point - lhs.point, rhs.dir) / denominator));
}

// One wall in metres, with the distance from its location line to each face. Joining and
// finding rooms read walls through it, so both cut by the same lines.
struct Frame {
  Vec2 start;
  Vec2 end;
  Vec2 dir;
  Vec2 left;
  StepMm step;
  double length = 0.0;
  double thickness = 0.0;
  double left_offset = 0.0;
  double right_offset = 0.0;
};

// Precondition: the wall passes validate(), so it has a length.
[[nodiscard]] inline Frame make_frame(const Wall& wall) {
  Frame frame;
  frame.start = to_vec2(wall.start);
  frame.end = to_vec2(wall.end);
  frame.step = step(wall.start, wall.end);
  frame.length = length(frame.end - frame.start);
  frame.dir = (frame.end - frame.start) * (1.0 / frame.length);
  frame.left = perp(frame.dir);
  frame.thickness = to_metres(wall.thickness);
  const Faces offsets = faces(wall);
  frame.left_offset = offsets.left;
  frame.right_offset = offsets.right;
  return frame;
}

// Whether the wall's body holds the point away from both of its ends (see holds in axis.h).
[[nodiscard]] constexpr bool holds(const Frame& frame, Vec2 point) {
  const Axis line{
      .start = frame.start, .dir = frame.dir, .left = frame.left, .length = frame.length};
  return holds(line, {.left = frame.left_offset, .right = frame.right_offset}, point);
}

// The outline without points that repeat the one before them, the last against the first too.
[[nodiscard]] inline std::vector<Vec2> drop_repeats(const std::vector<Vec2>& points) {
  std::vector<Vec2> kept;
  kept.reserve(points.size());
  for (const Vec2 point : points) {
    if (kept.empty() || length(point - kept.back()) > kEpsilon) {
      kept.push_back(point);
    }
  }
  while (kept.size() > 1 && length(kept.front() - kept.back()) <= kEpsilon) {
    kept.pop_back();
  }
  return kept;
}

}  // namespace otves::geom
