#include <algorithm>
#include <cstddef>
#include <expected>
#include <limits>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/assert.h>
#include <otves/core/error.h>
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

// A rectangle on a wall's face, in metres: u along the location line from the start, v up from
// the base. `id` is the opening it belongs to.
struct Rect {
  ElementId id;
  double u0 = 0.0;
  double u1 = 0.0;
  double v0 = 0.0;
  double v1 = 0.0;

  [[nodiscard]] constexpr bool empty() const { return u1 <= u0 || v1 <= v0; }
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

// What one wall loses to its openings and their lintels, with the lintels to order.
struct Cut {
  std::vector<Rect> holes;  // openings and lintels, clipped to the wall's face
  std::vector<Rect> openings;
  std::vector<Lintel> lintels;
};

class Cutter {
 public:
  Cutter(ElementId wall_id, const Wall& wall, const LintelSpec& spec,
         std::vector<OpeningIssue>& issues)
      : wall_id_(wall_id),
        thickness_(wall.thickness),
        length_(geom::length(geom::to_vec2(wall.end) - geom::to_vec2(wall.start))),
        height_(to_metres(wall.height)),
        spec_(spec),
        issues_(&issues) {}

  [[nodiscard]] Cut cut(std::span<const OpeningInput* const> openings) && {
    std::vector<Rect> lintels;
    for (const OpeningInput* input : openings) {
      place(*input, lintels);
    }
    check_piers(lintels);
    cut_.holes = cut_.openings;
    cut_.holes.insert(cut_.holes.end(), lintels.begin(), lintels.end());
    return std::move(cut_);
  }

 private:
  [[nodiscard]] Rect clip(const Rect& rect) const {
    return {.id = rect.id,
            .u0 = std::max(rect.u0, 0.0),
            .u1 = std::min(rect.u1, length_),
            .v0 = std::max(rect.v0, 0.0),
            .v1 = std::min(rect.v1, height_)};
  }

  [[nodiscard]] bool past_ends(const Rect& rect) const {
    return rect.u0 < -kEpsilon || rect.u1 > length_ + kEpsilon;
  }

  void report(ElementId opening, OpeningIssueKind kind) {
    issues_->push_back({.opening = opening, .kind = kind});
  }

  void place(const OpeningInput& input, std::vector<Rect>& lintels) {
    const Opening& opening = input.opening;
    const double u0 = to_metres(opening.offset);
    const double v0 = to_metres(opening.sill);
    const Rect hole{.id = input.id,
                    .u0 = u0,
                    .u1 = u0 + to_metres(opening.width),
                    .v0 = v0,
                    .v1 = v0 + to_metres(opening.height)};
    if (past_ends(hole) || hole.v1 > height_ + kEpsilon) {
      report(input.id, OpeningIssueKind::kOutsideWall);
    }
    const Rect inside = clip(hole);
    if (inside.empty()) {
      return;
    }
    cut_.openings.push_back(inside);

    const Mm bearing = opening.width <= spec_.short_span ? spec_.short_bearing : spec_.bearing;
    const double side = to_metres(bearing);
    const Rect lintel{.id = input.id,
                      .u0 = hole.u0 - side,
                      .u1 = hole.u1 + side,
                      .v0 = hole.v1,
                      .v1 = hole.v1 + to_metres(spec_.height)};
    if (past_ends(lintel)) {
      report(input.id, OpeningIssueKind::kShortBearing);
    }
    if (lintel.v1 > height_ + kEpsilon) {
      report(input.id, OpeningIssueKind::kNoRoomForLintel);
    }
    const Rect lintel_inside = clip(lintel);
    if (lintel_inside.empty()) {
      return;  // no room at all above the opening, such as a door up to the wall's top
    }
    lintels.push_back(lintel_inside);
    cut_.lintels.push_back({.opening = input.id,
                            .wall = wall_id_,
                            .length = lintel.u1 - lintel.u0,
                            .height = to_metres(spec_.height),
                            .thickness = thickness_});
  }

  // Openings may not overlap each other; a lintel may not reach another opening or lintel.
  void check_piers(std::span<const Rect> lintels) {
    const std::vector<Rect>& openings = cut_.openings;
    for (std::size_t i = 0; i < openings.size(); ++i) {
      for (std::size_t j = i + 1; j < openings.size(); ++j) {
        if (overlap(openings[i], openings[j])) {
          report(openings[i].id, OpeningIssueKind::kOverlap);
          report(openings[j].id, OpeningIssueKind::kOverlap);
        }
      }
    }
    for (std::size_t i = 0; i < lintels.size(); ++i) {
      for (const Rect& opening : openings) {
        if (opening.id != lintels[i].id && overlap(lintels[i], opening)) {
          report(lintels[i].id, OpeningIssueKind::kShortBearing);
        }
      }
      for (std::size_t j = i + 1; j < lintels.size(); ++j) {
        if (overlap(lintels[i], lintels[j])) {
          report(lintels[i].id, OpeningIssueKind::kShortBearing);
          report(lintels[j].id, OpeningIssueKind::kShortBearing);
        }
      }
    }
  }

  ElementId wall_id_;
  Mm thickness_;
  double length_;
  double height_;
  LintelSpec spec_;
  std::vector<OpeningIssue>* issues_;
  Cut cut_;
};

}  // namespace

std::expected<void, Error> validate(const LintelSpec& spec) {
  if (spec.height <= Mm{0}) {
    return make_error(ErrorCode::kHeightNotPositive, "lintel height must be positive");
  }
  if (spec.bearing < Mm{0} || spec.short_bearing < Mm{0}) {
    return make_error(ErrorCode::kBearingNegative, "lintel bearing must not be negative");
  }
  return {};
}

Result<Measurement> measure_walls(std::span<const geom::WallInput> walls,
                                  std::span<const OpeningInput> openings, const LintelSpec& lintels,
                                  const geom::WallJoins& joins) {
  if (auto valid = validate(lintels); !valid) {
    return std::unexpected{std::move(valid).error()};
  }
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
    Cut cut = Cutter{footprint.id, wall, lintels, result.issues}.cut(hosted);
    const double openings_area = union_area(cut.openings);
    const double holes_area = union_area(cut.holes);
    result.lintels.insert(result.lintels.end(), cut.lintels.begin(), cut.lintels.end());

    const double area = geom::signed_area(footprint.outline);
    const double height = to_metres(wall.height);
    const double thickness = to_metres(wall.thickness);
    const double length = area / thickness;
    result.walls.push_back({.id = footprint.id,
                            .thickness = wall.thickness,
                            .footprint_area = area,
                            .length = length,
                            .face_area = std::max(0.0, (length * height) - holes_area),
                            .volume = std::max(0.0, (area * height) - (holes_area * thickness)),
                            .openings_area = openings_area});
  }

  for (const geom::Junction& junction : joins.junctions) {
    JunctionQuantities quantities;
    double height = std::numeric_limits<double>::infinity();
    for (const ElementId id : junction.walls) {
      const auto found = std::ranges::lower_bound(by_id, id, {}, wall_id);
      if (found != by_id.end() && (*found)->id == id) {
        quantities.walls.push_back(id);
        height = std::min(height, to_metres((*found)->wall.height));
      }
    }
    if (quantities.walls.size() >= 2) {
      quantities.height = height;
      result.junctions.push_back(std::move(quantities));
    }
  }

  for (const OpeningInput* input : by_host) {
    const bool measured =
        std::ranges::binary_search(result.walls, input->opening.wall, {}, &WallQuantities::id);
    if (!measured) {
      result.issues.push_back({.opening = input->id, .kind = OpeningIssueKind::kNoWall});
    }
  }
  std::ranges::sort(result.lintels, {}, &Lintel::opening);
  std::ranges::sort(result.issues);
  const auto [first, last] = std::ranges::unique(result.issues);
  result.issues.erase(first, last);
  return result;
}

}  // namespace otves::takeoff
