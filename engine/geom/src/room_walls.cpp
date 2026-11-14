#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/editing.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

#include "axis.h"
#include "frame.h"
#include "step.h"

namespace otves::geom {
namespace {

// A face of a wall as built: an edge of its joined outline, which runs counter-clockwise, so
// the wall's body lies to the left of it.
struct Face {
  Vec2 from;
  Vec2 to;
  ElementId wall;
};

std::vector<Face> faces_of(std::span<const WallFootprint> footprints) {
  std::vector<Face> faces;
  for (const WallFootprint& footprint : footprints) {
    for (std::size_t i = 0; i < footprint.outline.size(); ++i) {
      faces.push_back({.from = footprint.outline[i],
                       .to = footprint.outline[(i + 1) % footprint.outline.size()],
                       .wall = footprint.id});
    }
  }
  return faces;
}

// The nearer of two points to `to`; of two as near, the one that sorts first.
bool nearer(Vec2 lhs, Vec2 rhs, Vec2 to) {
  const double left = dot(lhs - to, lhs - to);
  const double right = dot(rhs - to, rhs - to);
  if (left != right) {
    return left < right;
  }
  return lhs.x != rhs.x ? lhs.x < rhs.x : lhs.y < rhs.y;
}

// The point of the segment nearest to `to`.
Vec2 nearest_on(const Face& face, Vec2 to) {
  const Vec2 run = face.to - face.from;
  const double part = std::clamp(dot(to - face.from, run) / dot(run, run), 0.0, 1.0);
  return face.from + (run * part);
}

// The room's corner: a corner of what is built within reach of the touch, else a point of a
// face within reach, else the touch on the grid.
PointMm corner_of(std::span<const Face> faces, const RoomAsk& ask) {
  const Vec2 touch = to_vec2(ask.at);
  const double reach = to_metres(ask.reach);
  std::optional<Vec2> best;
  const auto take = [&](Vec2 point) {
    if (length(point - touch) <= reach && (!best || nearer(point, *best, touch))) {
      best = point;
    }
  };
  for (const Face& face : faces) {
    take(face.from);
  }
  if (!best) {
    for (const Face& face : faces) {
      take(nearest_on(face, touch));
    }
  }
  if (!best) {
    return {.x = on_grid(ask.at.x, ask.grid), .y = on_grid(ask.at.y, ask.grid)};
  }
  return {.x = round_to_mm(best->x), .y = round_to_mm(best->y)};
}

// A side of the room, from one corner along a unit direction: the room lies to its left.
struct Side {
  PointMm start;
  PointMm end;
  Vec2 from;
  Vec2 dir;
  double length = 0.0;
};

Side side(PointMm start, PointMm end) {
  const Vec2 from = to_vec2(start);
  const Vec2 run = to_vec2(end) - from;
  const double reach = length(run);
  return {.start = start, .end = end, .from = from, .dir = run * (1.0 / reach), .length = reach};
}

// A stretch of a side that a wall's face lies along, from the side's start.
struct Lined {
  double from = 0.0;
  double to = 0.0;
  ElementId wall;
};

// The stretches of the side that lie along faces of walls standing outside the room: faces on
// the side's line that run the other way, in order along the side.
std::vector<Lined> lined(const Side& side, std::span<const Face> faces) {
  std::vector<Lined> found;
  for (const Face& face : faces) {
    const Vec2 run = face.to - face.from;
    const bool opposite = dot(run, side.dir) < 0.0 && std::abs(cross(run, side.dir)) <= kEpsilon;
    const bool on_line = std::abs(cross(face.from - side.from, side.dir)) <= kEpsilon;
    const double from = std::max(dot(face.to - side.from, side.dir), 0.0);
    const double to = std::min(dot(face.from - side.from, side.dir), side.length);
    if (opposite && on_line && to - from > kEpsilon) {
      found.push_back({.from = from, .to = to, .wall = face.wall});
    }
  }
  std::ranges::sort(found, {}, &Lined::from);
  return found;
}

// How a side stands to the walls already there.
enum class Standing : std::uint8_t {
  kOpen,   // no wall along it: it needs one
  kLined,  // a wall, or walls end to end, along all of it
  kPartly  // a wall along part of it
};

Standing standing_of(const Side& side, std::span<const Lined> along) {
  if (along.empty()) {
    return Standing::kOpen;
  }
  double reached = 0.0;
  for (const Lined& part : along) {
    if (part.from - reached > kEpsilon) {
      return Standing::kPartly;
    }
    reached = std::max(reached, part.to);
  }
  return side.length - reached > kEpsilon ? Standing::kPartly : Standing::kLined;
}

// Whether the segment passes through the rectangle between `low` and `high`, its edge left out.
bool passes_through(const Face& face, Vec2 low, Vec2 high) {
  double enters = 0.0;
  double leaves = 1.0;
  const std::array spans{std::pair{face.from.x, face.to.x - face.from.x},
                         std::pair{face.from.y, face.to.y - face.from.y}};
  const std::array lows{low.x + kEpsilon, low.y + kEpsilon};
  const std::array highs{high.x - kEpsilon, high.y - kEpsilon};
  for (std::size_t axis = 0; axis < spans.size(); ++axis) {
    const auto [start, run] = spans.at(axis);
    if (run == 0.0) {
      if (start < lows.at(axis) || start > highs.at(axis)) {
        return false;
      }
      continue;
    }
    const double one = (lows.at(axis) - start) / run;
    const double other = (highs.at(axis) - start) / run;
    enters = std::max(enters, std::min(one, other));
    leaves = std::min(leaves, std::max(one, other));
  }
  return enters <= leaves;
}

// Where the side's line meets the line of the wall it ends against, if that is in the wall's
// body: the new wall ends there, so it joins that wall.
std::optional<PointMm> meeting(std::span<const WallInput> walls, ElementId wall, const Side& side) {
  const auto host = std::ranges::find(walls, wall, &WallInput::id);
  if (host == walls.end()) {
    return std::nullopt;
  }
  const Frame frame = make_frame(host->wall);
  const std::optional<Vec2> met =
      intersect({.point = side.from, .dir = side.dir}, {.point = frame.start, .dir = frame.dir});
  if (!met || !holds(frame, *met)) {
    return std::nullopt;
  }
  return PointMm{.x = round_to_mm(met->x), .y = round_to_mm(met->y)};
}

}  // namespace

Result<std::vector<WallLine>> room_walls(std::span<const WallInput> walls, const RoomAsk& ask) {
  if (ask.width <= Mm{0} || ask.depth <= Mm{0}) {
    return make_error(ErrorCode::kDoesNotFit, "a room has a width and a depth");
  }
  const std::vector<Face> faces = faces_of(join_walls(walls).footprints);
  const PointMm corner = corner_of(faces, ask);
  const PointMm across{.x = corner.x + (ask.right ? ask.width : -ask.width), .y = corner.y};
  const PointMm low{.x = std::min(corner.x, across.x),
                    .y = ask.up ? corner.y : corner.y - ask.depth};
  const PointMm high{.x = std::max(corner.x, across.x), .y = low.y + ask.depth};
  if (!on_plan(low) || !on_plan(high)) {
    return make_error(ErrorCode::kOffPlan, "the room would leave the plan");
  }
  if (std::ranges::any_of(faces, [&](const Face& face) {
        return passes_through(face, to_vec2(low), to_vec2(high));
      })) {
    return make_error(ErrorCode::kDoesNotFit, "a wall stands in the room");
  }
  // Counter-clockwise from the bottom, so the room lies to the left of each side.
  const std::array sides{
      side(low, {.x = high.x, .y = low.y}), side({.x = high.x, .y = low.y}, high),
      side(high, {.x = low.x, .y = high.y}), side({.x = low.x, .y = high.y}, low)};
  std::array<std::vector<Lined>, 4> along;
  std::array<Standing, 4> standing{};
  for (std::size_t i = 0; i < sides.size(); ++i) {
    along.at(i) = lined(sides.at(i), faces);
    standing.at(i) = standing_of(sides.at(i), along.at(i));
    if (standing.at(i) == Standing::kPartly) {
      return make_error(ErrorCode::kDoesNotFit, "a wall runs along part of a side of the room");
    }
  }
  std::vector<WallLine> built;
  for (std::size_t i = 0; i < sides.size(); ++i) {
    if (standing.at(i) != Standing::kOpen) {
      continue;
    }
    // A side that ends against a wall already there runs on to that wall's line.
    const std::size_t before = (i + sides.size() - 1) % sides.size();
    const std::size_t after = (i + 1) % sides.size();
    WallLine wall{
        .start = sides.at(i).start, .end = sides.at(i).end, .line = LocationLine::kLeftFace};
    if (standing.at(before) == Standing::kLined) {
      wall.start = meeting(walls, along.at(before).back().wall, sides.at(i)).value_or(wall.start);
    }
    if (standing.at(after) == Standing::kLined) {
      wall.end = meeting(walls, along.at(after).front().wall, sides.at(i)).value_or(wall.end);
    }
    built.push_back(wall);
  }
  if (built.empty()) {
    return make_error(ErrorCode::kDoesNotFit, "the room is there already");
  }
  return built;
}

}  // namespace otves::geom
