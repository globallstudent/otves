#include "built.h"

#include <map>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/build.h>
#include <otves/core/model.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/layers.h>

namespace otves::bridge {
namespace {

std::vector<takeoff::BuiltWall> built(const Model& model, std::optional<ElementId> level) {
  // A house has a handful of types and many walls of each.
  std::map<ElementId, std::vector<BuiltLayer>> by_type;
  std::vector<takeoff::BuiltWall> walls;
  walls.reserve(model.count<Wall>());
  model.for_each<Wall>([&](ElementId id, const Wall& wall) {
    if (level && wall.level != *level) {
      return;
    }
    Build build;
    if (wall.type.is_nil()) {
      build = build_of(model, wall);
    } else {
      const auto known = by_type.find(wall.type);
      build.layers = known != by_type.end()
                         ? known->second
                         : by_type.emplace(wall.type, layers_of(model, wall.type)).first->second;
    }
    if (build.thickness() > Mm{0}) {
      walls.push_back(
          {.id = id, .wall = geometric(wall, build), .layers = std::move(build.layers)});
    }
  });
  return walls;
}

}  // namespace

std::vector<takeoff::BuiltWall> built_walls(const Model& model) {
  return built(model, std::nullopt);
}

std::vector<takeoff::BuiltWall> built_walls(const Model& model, ElementId level) {
  return built(model, level);
}

std::vector<geom::WallInput> inputs(std::span<const takeoff::BuiltWall> walls) {
  std::vector<geom::WallInput> out;
  out.reserve(walls.size());
  for (const takeoff::BuiltWall& built : walls) {
    out.push_back({.id = built.id, .wall = built.wall});
  }
  return out;
}

}  // namespace otves::bridge
