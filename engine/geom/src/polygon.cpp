#include <cstddef>
#include <span>

#include <otves/geom/polygon.h>
#include <otves/geom/vec2.h>

namespace otves::geom {

double signed_area(std::span<const Vec2> outline) {
  double twice = 0.0;
  for (std::size_t i = 0; i < outline.size(); ++i) {
    twice += cross(outline[i], outline[(i + 1) % outline.size()]);
  }
  return twice / 2;
}

bool contains(std::span<const Vec2> outline, Vec2 point) {
  bool inside = false;
  for (std::size_t i = 0; i < outline.size(); ++i) {
    const Vec2 a = outline[i];
    const Vec2 b = outline[(i + 1) % outline.size()];
    if ((a.y > point.y) != (b.y > point.y) &&
        point.x < a.x + ((point.y - a.y) / (b.y - a.y) * (b.x - a.x))) {
      inside = !inside;
    }
  }
  return inside;
}

}  // namespace otves::geom
