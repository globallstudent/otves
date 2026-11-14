#include <cstddef>
#include <span>
#include <vector>

#include <otves/core/units.h>
#include <otves/core/wall.h>
#include <otves/geom/faces.h>
#include <otves/geom/layers.h>
#include <otves/geom/vec2.h>

#include "axis.h"

namespace otves::geom {
namespace {

// Keeps the part of a polygon where `inside` holds, cutting its edges where `across` reaches
// `bound` (Sutherland and Hodgman). A point within kEpsilon of the bound counts as inside, so a
// vertex on a face is never cut off by rounding.
template <typename Across, typename Inside>
std::vector<Vec2> keep(std::span<const Vec2> polygon, double bound, const Across& across,
                       const Inside& inside) {
  std::vector<Vec2> kept;
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const Vec2 from = polygon[i];
    const Vec2 to = polygon[(i + 1) % polygon.size()];
    const double at_from = across(from);
    const double at_to = across(to);
    if (inside(at_from)) {
      kept.push_back(from);
    }
    if (inside(at_from) != inside(at_to)) {
      kept.push_back(from + ((to - from) * ((bound - at_from) / (at_to - at_from))));
    }
  }
  return kept;
}

}  // namespace

std::vector<Strip> layer_strips(const Wall& wall, std::span<const Mm> thicknesses) {
  const Faces offsets = faces(wall);
  std::vector<Strip> strips;
  strips.reserve(thicknesses.size());
  // Whole millimetres are added before the conversion, so the strips meet exactly.
  Mm done;
  for (const Mm thickness : thicknesses) {
    const double near = to_metres(done);
    const double far = to_metres(done + thickness);
    strips.push_back(wall.mirrored
                         ? Strip{.from = offsets.left - far, .to = offsets.left - near}
                         : Strip{.from = near - offsets.right, .to = far - offsets.right});
    done += thickness;
  }
  return strips;
}

std::vector<Vec2> clip(const Wall& wall, std::span<const Vec2> outline, Strip strip) {
  const Axis line = axis(wall);
  const auto across = [&line](Vec2 point) { return dot(point - line.start, line.left); };
  const std::vector<Vec2> above = keep(outline, strip.from, across,
                                       [&strip](double at) { return at >= strip.from - kEpsilon; });
  return keep(above, strip.to, across, [&strip](double at) { return at <= strip.to + kEpsilon; });
}

}  // namespace otves::geom
