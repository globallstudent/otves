#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <otves/core/point.h>
#include <otves/core/uuid.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

namespace otves::geom {

// A part of the plan closed by walls on every side, as the plan shows it: bounded by the inner
// faces of its walls (docs/adr/0015). In metres and m².
struct Space {
  std::vector<Vec2> outline;  // counter-clockwise
  // For each point of the outline, the wall whose face the outline runs along from that point to
  // the next one.
  std::vector<ElementId> sides;
  // What stands free inside: a wall or a closed group of walls joined to nothing round it,
  // each as its outer faces, clockwise.
  std::vector<std::vector<Vec2>> islands;
  double area = 0.0;       // the floor: the outline less the islands
  double perimeter = 0.0;  // along the outline and round the islands
};

// The spaces the walls of one level close. Walls close a space when they meet by the rules
// join_walls joins them by: ends at one point, or an end inside another wall's body. Walls that
// cross away from their ends are not split, so the spaces on both sides of the crossing read as
// one. Openings do not open a space.
// Precondition: every wall passes validate() and ids are unique. The result does not depend on
// the order of `walls`.
[[nodiscard]] std::vector<Space> find_spaces(std::span<const WallInput> walls);

// Whether the point lies on the space's floor: inside its outline and outside its islands.
[[nodiscard]] bool holds(const Space& space, Vec2 point);

// The point that names a room (docs/adr/0015).
struct Anchor {
  ElementId room;
  PointMm at;
};

enum class AnchorIssueKind : std::uint8_t {
  // The anchor lies in no space: in a wall, or where the walls do not close.
  kNoSpace,
  // An older room already names the space the anchor lies in.
  kSharedSpace,
};

struct AnchorIssue {
  ElementId room;
  AnchorIssueKind kind = AnchorIssueKind::kNoSpace;

  constexpr auto operator<=>(const AnchorIssue&) const = default;
};

struct Names {
  // For each space, in the order of `spaces`, the room that names it. Ids grow with time, so of
  // several rooms in one space the oldest names it.
  std::vector<std::optional<ElementId>> rooms;
  std::vector<AnchorIssue> issues;  // sorted
};

// Gives each space the room whose anchor it holds. Precondition: room ids are unique. The result
// does not depend on the order of `anchors`.
[[nodiscard]] Names name_spaces(std::span<const Space> spaces, std::span<const Anchor> anchors);

}  // namespace otves::geom
