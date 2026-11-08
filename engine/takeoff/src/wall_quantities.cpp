#include <algorithm>
#include <span>
#include <vector>

#include <otves/core/assert.h>
#include <otves/core/units.h>
#include <otves/core/wall.h>
#include <otves/geom/polygon.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {

std::vector<WallQuantities> measure_walls(std::span<const geom::WallInput> walls,
                                          const geom::WallJoins& joins) {
  std::vector<const geom::WallInput*> by_id;
  by_id.reserve(walls.size());
  for (const geom::WallInput& input : walls) {
    by_id.push_back(&input);
  }
  std::ranges::sort(by_id, {}, [](const geom::WallInput* input) { return input->id; });

  std::vector<WallQuantities> result;
  result.reserve(joins.footprints.size());
  for (const geom::WallFootprint& footprint : joins.footprints) {
    const auto found = std::ranges::lower_bound(
        by_id, footprint.id, {}, [](const geom::WallInput* input) { return input->id; });
    const bool known = found != by_id.end() && (*found)->id == footprint.id;
    OTVES_ASSERT(known);
    if (!known) {
      continue;
    }
    const Wall& wall = (*found)->wall;
    const double area = geom::signed_area(footprint.outline);
    const double height = to_metres(wall.height);
    const double length = area / to_metres(wall.thickness);
    result.push_back({.id = footprint.id,
                      .thickness = wall.thickness,
                      .footprint_area = area,
                      .length = length,
                      .face_area = length * height,
                      .volume = area * height});
  }
  return result;
}

}  // namespace otves::takeoff
