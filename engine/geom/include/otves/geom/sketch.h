#pragma once

#include <compare>
#include <span>
#include <vector>

#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/wall.h>
#include <otves/geom/wall_joins.h>

// A plan drawn roughly with a finger and read as walls (docs/adr/0012).
namespace otves::geom {

// One stroke: the finger goes down, along the walls, and up.
struct Sketch {
  std::span<const PointMm> points;  // in the order drawn
  Mm reach;                         // how far from the finger still counts as touched
  Mm grid;                          // a free coordinate falls on a multiple of this, if positive
  bool square = false;              // hold every wall to the plan's axes
};

// A wall read from a stroke, in the direction it was drawn.
struct Leg {
  PointMm start;
  PointMm end;

  constexpr auto operator<=>(const Leg&) const = default;
};

// The walls a stroke means, in the order drawn.
//  - The stroke turns a corner where it leaves the line between the corners around it by more
//    than `reach`.
//  - A leg held to an axis, as aim() holds a wall, lies where the stroke ran along it: at the
//    middle value of its points.
//  - A stroke of three legs or more that ends within two reaches of where it began closes on it:
//    both are a finger's points.
//  - A corner within reach of a wall end, of this level or of `below`, is that end, and the legs
//    into it keep their axes.
//  - Each leg then ends where aim() puts it, with the legs before it read as walls built like
//    `like`: it is caught by wall ends, lined up with them, laid on the grid and met with walls
//    along their length as a wall drawn point by point.
//  - Legs that run on in one line are one wall.
// Fails with kOffPlan when a point lies off the plan, kZeroLength when the stroke gives no wall,
// and what validate() says of a wall built like `like`. Precondition: as for aim(). The result
// does not depend on the order of `walls` or of `below`.
[[nodiscard]] Result<std::vector<Leg>> straighten(std::span<const WallInput> walls,
                                                  std::span<const PointMm> below,
                                                  const Sketch& sketch, const Wall& like);

}  // namespace otves::geom
