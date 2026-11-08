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

}  // namespace otves::geom
