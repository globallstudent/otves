#include <algorithm>
#include <cstddef>
#include <limits>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/assert.h>
#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/polygon.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {
namespace {

// Inputs sit on a 1 mm grid; an opening that ends within 1 µm of its wall's end fits.
constexpr double kEpsilon = 1e-6;

// An opening on its wall's face, in metres: u along the location line from the start, v up from
// the base.
struct Rect {
  ElementId id;
  double u0 = 0.0;
  double u1 = 0.0;
  double v0 = 0.0;
  double v1 = 0.0;
};

constexpr bool overlap(const Rect& a, const Rect& b) {
  return a.u0 < b.u1 && b.u0 < a.u1 && a.v0 < b.v1 && b.v0 < a.v1;
}

// Area of the union: for each strip between neighbouring u edges, merge the v spans of the
// rectangles covering it. A wall holds a handful of openings, so the quadratic sweep is cheap.
double union_area(std::span<const Rect> rects) {
  std::vector<double> edges;
  edges.reserve(rects.size() * 2);
  for (const Rect& rect : rects) {
    edges.push_back(rect.u0);
    edges.push_back(rect.u1);
  }
  std::ranges::sort(edges);
  const auto [first, last] = std::ranges::unique(edges);
  edges.erase(first, last);

  double area = 0.0;
  std::vector<std::pair<double, double>> spans;
  for (std::size_t i = 0; i + 1 < edges.size(); ++i) {
    spans.clear();
    for (const Rect& rect : rects) {
      if (rect.u0 <= edges[i] && rect.u1 >= edges[i + 1]) {
        spans.emplace_back(rect.v0, rect.v1);
      }
    }
    std::ranges::sort(spans);
    double covered = 0.0;
    double top = -std::numeric_limits<double>::infinity();
    for (const auto [low, high] : spans) {
      if (high > top) {
        covered += high - std::max(low, top);
        top = high;
      }
    }
    area += covered * (edges[i + 1] - edges[i]);
  }
  return area;
}

// The openings of one wall, clipped to its face, with issues for those that do not fit.
std::vector<Rect> place(const Wall& wall, std::span<const OpeningInput* const> openings,
                        std::vector<OpeningIssue>& issues) {
  const double length = geom::length(geom::to_vec2(wall.end) - geom::to_vec2(wall.start));
  const double height = to_metres(wall.height);
  std::vector<Rect> rects;
  rects.reserve(openings.size());
  for (const OpeningInput* input : openings) {
    const Opening& opening = input->opening;
    const double u0 = to_metres(opening.offset);
    const double u1 = u0 + to_metres(opening.width);
    const double v0 = to_metres(opening.sill);
    const double v1 = v0 + to_metres(opening.height);
    if (u1 > length + kEpsilon || v1 > height + kEpsilon) {
      issues.push_back({.opening = input->id, .kind = OpeningIssueKind::kOutsideWall});
    }
    const Rect clipped{.id = input->id,
                       .u0 = u0,
                       .u1 = std::min(u1, length),
                       .v0 = v0,
                       .v1 = std::min(v1, height)};
    if (clipped.u1 > clipped.u0 && clipped.v1 > clipped.v0) {
      rects.push_back(clipped);
    }
  }
  for (std::size_t i = 0; i < rects.size(); ++i) {
    for (std::size_t j = i + 1; j < rects.size(); ++j) {
      if (overlap(rects[i], rects[j])) {
        issues.push_back({.opening = rects[i].id, .kind = OpeningIssueKind::kOverlap});
        issues.push_back({.opening = rects[j].id, .kind = OpeningIssueKind::kOverlap});
      }
    }
  }
  return rects;
}

}  // namespace

Measurement measure_walls(std::span<const geom::WallInput> walls,
                          std::span<const OpeningInput> openings, const geom::WallJoins& joins) {
  const auto wall_id = [](const geom::WallInput* input) { return input->id; };
  std::vector<const geom::WallInput*> by_id;
  by_id.reserve(walls.size());
  for (const geom::WallInput& input : walls) {
    by_id.push_back(&input);
  }
  std::ranges::sort(by_id, {}, wall_id);

  const auto host = [](const OpeningInput* input) { return input->opening.wall; };
  std::vector<const OpeningInput*> by_host;
  by_host.reserve(openings.size());
  for (const OpeningInput& input : openings) {
    OTVES_ASSERT(validate(input.opening).has_value());
    by_host.push_back(&input);
  }
  std::ranges::sort(by_host, [](const OpeningInput* a, const OpeningInput* b) {
    return std::pair{a->opening.wall, a->id} < std::pair{b->opening.wall, b->id};
  });

  Measurement result;
  result.walls.reserve(joins.footprints.size());
  for (const geom::WallFootprint& footprint : joins.footprints) {
    const auto found = std::ranges::lower_bound(by_id, footprint.id, {}, wall_id);
    const bool known = found != by_id.end() && (*found)->id == footprint.id;
    OTVES_ASSERT(known);
    if (!known) {
      continue;
    }
    const Wall& wall = (*found)->wall;
    const auto hosted = std::ranges::equal_range(by_host, footprint.id, {}, host);
    const std::vector<Rect> rects = place(wall, hosted, result.issues);
    const double openings_area = union_area(rects);

    const double area = geom::signed_area(footprint.outline);
    const double height = to_metres(wall.height);
    const double thickness = to_metres(wall.thickness);
    const double length = area / thickness;
    result.walls.push_back({.id = footprint.id,
                            .thickness = wall.thickness,
                            .footprint_area = area,
                            .length = length,
                            .face_area = std::max(0.0, (length * height) - openings_area),
                            .volume = std::max(0.0, (area * height) - (openings_area * thickness)),
                            .openings_area = openings_area});
  }

  for (const OpeningInput* input : by_host) {
    const bool measured =
        std::ranges::binary_search(result.walls, input->opening.wall, {}, &WallQuantities::id);
    if (!measured) {
      result.issues.push_back({.opening = input->id, .kind = OpeningIssueKind::kNoWall});
    }
  }
  std::ranges::sort(result.issues);
  const auto [first, last] = std::ranges::unique(result.issues);
  result.issues.erase(first, last);
  return result;
}

}  // namespace otves::takeoff
