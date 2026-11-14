#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/editing.h>
#include <otves/geom/spaces.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

#include "axis.h"
#include "frame.h"

namespace otves::geom {
namespace {

constexpr Vec2 towards(Hand hand) {
  switch (hand) {
    case Hand::kTop:
      return {.x = 0.0, .y = 1.0};
    case Hand::kBottom:
      return {.x = 0.0, .y = -1.0};
    case Hand::kLeft:
      return {.x = -1.0, .y = 0.0};
    case Hand::kRight:
      break;
  }
  return {.x = 1.0, .y = 0.0};
}

// An edge of a space's outline or of one of its islands, with the wall whose face it is; nil for
// an island's.
struct Edge {
  Vec2 from;
  Vec2 to;
  ElementId wall;
};

std::vector<Edge> edges_of(const Space& space) {
  std::vector<Edge> edges;
  edges.reserve(space.outline.size());
  for (std::size_t i = 0; i < space.outline.size(); ++i) {
    edges.push_back({.from = space.outline[i],
                     .to = space.outline[(i + 1) % space.outline.size()],
                     .wall = space.sides[i]});
  }
  for (const std::vector<Vec2>& island : space.islands) {
    for (std::size_t i = 0; i < island.size(); ++i) {
      edges.push_back({.from = island[i], .to = island[(i + 1) % island.size()], .wall = {}});
    }
  }
  return edges;
}

// The edge a ray from `point` reaches first, going `way`. An end of an edge counts as its start,
// so a corner is one edge's.
const Edge* reached(std::span<const Edge> edges, Vec2 point, Vec2 way) {
  const Edge* nearest = nullptr;
  double least = 0.0;
  for (const Edge& edge : edges) {
    const Vec2 run = edge.to - edge.from;
    const double turn = cross(way, run);
    if (turn == 0.0) {
      continue;
    }
    const double along = cross(edge.from - point, run) / turn;
    const double part = cross(edge.from - point, way) / turn;
    if (along > kEpsilon && part >= 0.0 && part < 1.0 && (nearest == nullptr || along < least)) {
      nearest = &edge;
      least = along;
    }
  }
  return nearest;
}

// Where a line crosses an edge, as the distance along the line from its point.
struct Cut {
  double at = 0.0;
  ElementId wall;
};

// The line's crossings of the edges, in order along it: it enters the floor at one and leaves
// it at the next. An end on the line counts as beside it, so a corner is crossed once.
std::vector<Cut> cuts(std::span<const Edge> edges, const Line& line) {
  const Vec2 left = perp(line.dir);
  std::vector<Cut> found;
  for (const Edge& edge : edges) {
    const double from = dot(edge.from - line.point, left);
    const double to = dot(edge.to - line.point, left);
    if ((from > 0.0) == (to > 0.0)) {
      continue;
    }
    const Vec2 crossing = edge.from + ((edge.to - edge.from) * (from / (from - to)));
    found.push_back({.at = dot(crossing - line.point, line.dir), .wall = edge.wall});
  }
  std::ranges::sort(found, {}, &Cut::at);
  return found;
}

// The stretch of floor on the line that lies beside `at`, or else the longest; the first of
// equals. Nothing when the line misses the floor.
std::optional<std::size_t> stretch_beside(std::span<const Cut> found, double at) {
  std::optional<std::size_t> best;
  for (std::size_t i = 0; i + 1 < found.size(); i += 2) {
    if (at >= found[i].at && at <= found[i + 1].at) {
      return i;
    }
    if (!best || found[i + 1].at - found[i].at > found[*best + 1].at - found[*best].at) {
      best = i;
    }
  }
  return best;
}

// Where the line meets the wall's own line, if that is in the wall's body: a wall that ends
// there stands against it.
std::optional<PointMm> meeting(std::span<const WallInput> walls, ElementId wall, const Line& line) {
  const auto host = std::ranges::find(walls, wall, &WallInput::id);
  if (host == walls.end()) {
    return std::nullopt;
  }
  const Frame frame = make_frame(host->wall);
  const std::optional<Vec2> met = intersect(line, {.point = frame.start, .dir = frame.dir});
  if (!met || !holds(frame, *met)) {
    return std::nullopt;
  }
  return PointMm{.x = round_to_mm(met->x), .y = round_to_mm(met->y)};
}

}  // namespace

Result<WallLine> across(std::span<const WallInput> walls, const Across& across) {
  const Vec2 in = to_vec2(across.in);
  const std::vector<Space> spaces = find_spaces(walls);
  const auto space =
      std::ranges::find_if(spaces, [in](const Space& found) { return holds(found, in); });
  if (space == spaces.end()) {
    return make_error(ErrorCode::kNotFound, "no room at the point");
  }
  const std::vector<Edge> edges = edges_of(*space);
  const Edge* side = reached(edges, in, towards(across.from));
  if (across.distance <= Mm{0} || side == nullptr || side->wall.is_nil()) {
    return make_error(ErrorCode::kDoesNotFit, "no wall to measure the distance from");
  }
  // The outline runs counter-clockwise, so the floor lies to the left of its edges.
  const Vec2 dir = (side->to - side->from) * (1.0 / length(side->to - side->from));
  const Line line{.point = side->from + (perp(dir) * to_metres(across.distance)), .dir = dir};
  const std::vector<Cut> found = cuts(edges, line);
  const std::optional<std::size_t> stretch = stretch_beside(found, dot(in - line.point, dir));
  if (!stretch) {
    return make_error(ErrorCode::kDoesNotFit, "the room ends before the distance");
  }
  const std::optional<PointMm> start = meeting(walls, found[*stretch].wall, line);
  const std::optional<PointMm> end = meeting(walls, found[*stretch + 1].wall, line);
  if (!start || !end || *start == *end) {
    return make_error(ErrorCode::kDoesNotFit, "no walls for the new wall to stand against");
  }
  if (!on_plan(*start) || !on_plan(*end)) {
    return make_error(ErrorCode::kOffPlan, "the new wall would leave the plan");
  }
  return WallLine{.start = *start, .end = *end, .line = LocationLine::kRightFace};
}

}  // namespace otves::geom
