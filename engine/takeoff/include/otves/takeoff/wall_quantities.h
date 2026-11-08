#pragma once

#include <span>
#include <vector>

#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/geom/wall_joins.h>

namespace otves::takeoff {

// What one joined wall measures. Joined outlines tile the plan, so a corner is split between its
// walls and every quantity adds up across walls.
struct WallQuantities {
  ElementId id;
  Mm thickness;
  double footprint_area = 0.0;  // m², the joined plan outline
  double length = 0.0;          // m, footprint area over thickness
  double face_area = 0.0;       // m², one face: length × height
  double volume = 0.0;          // m³, footprint area × height
};

// One entry per footprint in `joins`, sorted by id. Walls are measured as drawn: where join_walls
// reported an overlap, the shared part counts in both walls.
// Precondition: `joins` is join_walls(walls).
[[nodiscard]] std::vector<WallQuantities> measure_walls(std::span<const geom::WallInput> walls,
                                                        const geom::WallJoins& joins);

}  // namespace otves::takeoff
