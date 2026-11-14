#pragma once

#include <span>
#include <vector>

#include <otves/core/model.h>
#include <otves/core/uuid.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/layers.h>

// The walls of the model as geometry, drawing and take-off read them (docs/adr/0014): each as
// thick as what it is built of, with its layers from the outer face in.
namespace otves::bridge {

// Every wall, in the model's order. A wall whose type is missing or has no layer is left out:
// it has no thickness to stand on the plan with.
[[nodiscard]] std::vector<takeoff::BuiltWall> built_walls(const Model& model);

// The walls on one level.
[[nodiscard]] std::vector<takeoff::BuiltWall> built_walls(const Model& model, ElementId level);

// The same walls as geometry takes them.
[[nodiscard]] std::vector<geom::WallInput> inputs(std::span<const takeoff::BuiltWall> walls);

}  // namespace otves::bridge
