#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/wall_joins.h>

// Editing a plan asks geometric questions; these answer them (docs/adr/0011). They read the walls
// of one level as a graph, by the definition join_walls uses: ends at one point form a joint, and
// an end inside another wall's body, away from that wall's ends, stands against it.
namespace otves::geom {

// The aimed point lined up with a wall end: a line from that end to the point.
struct Guide {
  PointMm from;
  PointMm to;

  constexpr auto operator<=>(const Guide&) const = default;
};

// Where a wall being drawn would end.
struct Aimed {
  PointMm point;
  Mm length;                  // from the wall's start along its line; zero for a first point
  std::vector<Guide> guides;  // sorted, no duplicates
  // The point is a wall end or lies in a wall's body, so a wall ending here joins the plan.
  bool meets = false;

  bool operator==(const Aimed&) const = default;
};

// A touch on the plan while a wall is drawn.
struct Aim {
  std::optional<PointMm> from;  // the wall's start; absent while its first point is placed
  PointMm at;                   // under the finger
  Mm reach;                     // how far from the finger still counts as touched
  Mm grid;                      // a free coordinate falls on a multiple of this, if positive
  bool square = false;          // hold the wall to the plan's axes
};

// The point a touch means. In this order: a wall end within reach, of this level or of `below`,
// the level under it, so walls stand on walls; else a wall from `from` turns onto the nearer axis,
// always when `square` and otherwise within a few degrees of it, and each coordinate left free
// lines up with a wall end within reach or falls on the grid; then a point within reach of a
// wall's body moves onto that wall's line, so the new wall meets it along its length.
// Precondition: every wall passes validate() and ids are unique. The result does not depend on
// the order of `walls` or of `below`.
[[nodiscard]] Aimed aim(std::span<const WallInput> walls, std::span<const PointMm> below,
                        const Aim& aim);

enum class Turn : std::uint8_t {
  kLeft,
  kStraight,
  kRight,
};

// A wall entered from a tape or laser reading: its length, and which way it goes from the wall
// that ended at `from`. That wall started at `before`; without one, the run starts to the right
// of the plan.
struct Measured {
  PointMm from;
  std::optional<PointMm> before;
  Turn turn = Turn::kStraight;
  Mm length;
};

// Where the measured wall ends. A length that is not positive leaves it at `from`.
// Precondition: as for the touch.
[[nodiscard]] Aimed aim(std::span<const WallInput> walls, const Measured& measured);

// Where one end of a wall goes.
struct EndMove {
  ElementId wall;
  WallEnd end = WallEnd::kStart;
  PointMm to;

  constexpr auto operator<=>(const EndMove&) const = default;
};

// Every wall end that moves when `wall` gets `length` along its line, sorted by wall and end.
// The wall's `fixed` end stays and its other end's joint moves along the wall. When none is named
// the end joined to other walls stays if the other is free, so a partition grows from the wall it
// stands against and a wall just drawn from the corner it left; else the start stays. What is
// joined goes with it so that walls keep their direction: a wall with a moving point on it
// stretches when it runs along the move, and otherwise moves whole, with the wall ends that stand
// against it; a wall that reaches the fixed end turns about it instead. Fails with kNotFound for an
// unknown wall, kZeroLength when the length is not positive, kOffPlan when a point would leave the
// plan, and kBreaksJoin when a joined wall would vanish or turn back on itself or an end would come
// off the wall it stands against. Precondition: as for the touch.
[[nodiscard]] Result<std::vector<EndMove>> resize_wall(std::span<const WallInput> walls,
                                                       ElementId wall, Mm length,
                                                       std::optional<WallEnd> fixed = std::nullopt);

}  // namespace otves::geom
