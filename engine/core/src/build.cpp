#include <algorithm>
#include <cstdint>
#include <optional>
#include <tuple>
#include <vector>

#include <otves/core/build.h>
#include <otves/core/model.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/core/wall_type.h>

namespace otves {

Mm Build::thickness() const {
  Mm total;
  for (const BuiltLayer& layer : layers) {
    total += layer.thickness;
  }
  return total;
}

std::vector<BuiltLayer> layers_of(const Model& model, ElementId type) {
  std::vector<std::tuple<std::int64_t, ElementId, BuiltLayer>> found;
  model.for_each<WallLayer>([&found, type](ElementId id, const WallLayer& layer) {
    if (layer.type == type) {
      found.emplace_back(layer.position, id,
                         BuiltLayer{.function = layer.function,
                                    .thickness = layer.thickness,
                                    .unit_length = layer.unit_length,
                                    .unit_width = layer.unit_width,
                                    .unit_height = layer.unit_height});
    }
  });
  std::ranges::sort(found, [](const auto& a, const auto& b) {
    return std::tie(std::get<0>(a), std::get<1>(a)) < std::tie(std::get<0>(b), std::get<1>(b));
  });
  std::vector<BuiltLayer> layers;
  layers.reserve(found.size());
  for (const auto& [position, id, layer] : found) {
    layers.push_back(layer);
  }
  return layers;
}

Build build_of(const Model& model, const Wall& wall) {
  if (wall.type.is_nil()) {
    return {.role = std::nullopt,
            .layers = {BuiltLayer{.function = LayerFunction::kStructure,
                                  .thickness = wall.thickness,
                                  .unit_length = wall.unit_length,
                                  .unit_width = wall.unit_width,
                                  .unit_height = wall.unit_height}}};
  }
  const auto* type = model.find<WallType>(wall.type);
  if (type == nullptr) {
    return {};
  }
  return {.role = type->role, .layers = layers_of(model, wall.type)};
}

Wall geometric(const Wall& wall, const Build& build) {
  Wall plain = wall;
  plain.thickness = build.thickness();
  plain.type = {};
  return plain;
}

}  // namespace otves
