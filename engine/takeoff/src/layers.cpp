#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/build.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall_type.h>
#include <otves/geom/layers.h>
#include <otves/geom/polygon.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/layers.h>
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {
namespace {

std::optional<MasonryUnit> unit_of(const BuiltLayer& layer) {
  if (!layer.has_unit()) {
    return std::nullopt;
  }
  return MasonryUnit{
      .length = layer.unit_length, .width = layer.unit_width, .height = layer.unit_height};
}

bool is_one_masonry_layer(const BuiltWall& built) {
  return built.layers.size() == 1 && built.layers.front().is_masonry();
}

// The joined outline of every wall that has to be cut into layers, level by level.
std::map<ElementId, std::vector<geom::Vec2>> outlines_of(std::span<const BuiltWall> walls) {
  std::map<ElementId, std::vector<geom::Vec2>> outlines;
  if (std::ranges::all_of(walls, is_one_masonry_layer)) {
    return outlines;
  }
  std::map<ElementId, std::vector<geom::WallInput>> levels;
  for (const BuiltWall& built : walls) {
    levels[built.wall.level].push_back({.id = built.id, .wall = built.wall});
  }
  for (const auto& [level, level_walls] : levels) {
    for (geom::WallFootprint& footprint : geom::join_walls(level_walls).footprints) {
      outlines.emplace(footprint.id, std::move(footprint.outline));
    }
  }
  return outlines;
}

}  // namespace

Layers measure_layers(std::span<const BuiltWall> walls, const Measurement& measurement) {
  const std::map<ElementId, std::vector<geom::Vec2>> outlines = outlines_of(walls);
  Layers layers;
  std::map<Mm, double> insulated;
  for (const BuiltWall& built : walls) {
    const auto measured =
        std::ranges::lower_bound(measurement.walls, built.id, {}, &WallQuantities::id);
    if (measured == measurement.walls.end() || measured->id != built.id) {
      continue;
    }
    if (is_one_masonry_layer(built)) {
      layers.masonry.push_back(*measured);
      layers.units.push_back(unit_of(built.layers.front()));
      continue;
    }
    const auto outline = outlines.find(built.id);
    if (outline == outlines.end()) {
      continue;
    }
    std::vector<Mm> thicknesses;
    thicknesses.reserve(built.layers.size());
    for (const BuiltLayer& layer : built.layers) {
      thicknesses.push_back(layer.thickness);
    }
    const std::vector<geom::Strip> strips = geom::layer_strips(built.wall, thicknesses);
    const double height = to_metres(built.wall.height);
    // What openings and lintels take from one face of the wall, and so from every layer.
    const double cut = (measured->length * height) - measured->face_area;
    for (std::size_t index = 0; index < built.layers.size(); ++index) {
      const BuiltLayer& layer = built.layers[index];
      if (layer.function == LayerFunction::kGap) {
        continue;
      }
      const double thickness = to_metres(layer.thickness);
      const double area =
          std::max(0.0, geom::signed_area(geom::clip(built.wall, outline->second, strips[index])));
      const double length = area / thickness;
      const double face = std::max(0.0, (length * height) - cut);
      if (layer.is_masonry()) {
        layers.masonry.push_back({.id = built.id,
                                  .thickness = layer.thickness,
                                  .footprint_area = area,
                                  .length = length,
                                  .face_area = face,
                                  .volume = face * thickness,
                                  .openings_area = measured->openings_area});
        layers.units.push_back(unit_of(layer));
      } else {
        insulated[layer.thickness] += face;
      }
    }
  }
  layers.insulation.reserve(insulated.size());
  for (const auto& [thickness, area] : insulated) {
    layers.insulation.push_back({.thickness = thickness, .area = area});
  }
  return layers;
}

}  // namespace otves::takeoff
