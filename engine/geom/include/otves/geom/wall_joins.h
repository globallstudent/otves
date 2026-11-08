#pragma once

#include <compare>
#include <cstdint>
#include <span>
#include <vector>

#include <otves/core/point.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/vec2.h>

namespace otves::geom {

struct WallInput {
  ElementId id;
  Wall wall;
};

// The plan outline of one wall after joining, counter-clockwise, in metres. Outlines of joined
// walls share edges and do not overlap, so their areas add up to the masonry area.
struct WallFootprint {
  ElementId id;
  std::vector<Vec2> outline;
};

enum class JoinIssueKind : std::uint8_t {
  // Two walls run along the same line from one point, or a wall lies inside another.
  kOverlap,
  // Walls meet at so small an angle that the mitre would run past kMaxMitre wall thicknesses;
  // their ends are cut square instead.
  kSharpAngle,
};

struct JoinIssue {
  PointMm at;
  JoinIssueKind kind = JoinIssueKind::kOverlap;

  constexpr auto operator<=>(const JoinIssue&) const = default;
};

struct WallJoins {
  std::vector<WallFootprint> footprints;  // sorted by id
  std::vector<JoinIssue> issues;          // sorted, no duplicates
};

inline constexpr double kMaxMitre = 10.0;

// Joins walls on one level:
//  - ends at the same point are mitred (two walls) or cut around that point (three or more);
//  - an end lying inside another wall is cut back to that wall's face (butt joint).
// Precondition: every wall passes validate() and ids are unique. The result does not depend on
// the order of `walls`.
[[nodiscard]] WallJoins join_walls(std::span<const WallInput> walls);

}  // namespace otves::geom
