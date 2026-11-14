#pragma once

#include <optional>
#include <vector>

#include <otves/core/model.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/core/wall_type.h>

namespace otves {

// One layer of a wall as it is built.
struct BuiltLayer {
  LayerFunction function = LayerFunction::kStructure;
  Mm thickness;
  Mm unit_length;
  Mm unit_width;
  Mm unit_height;

  [[nodiscard]] constexpr bool is_masonry() const {
    return function == LayerFunction::kStructure || function == LayerFunction::kFacing;
  }
  [[nodiscard]] constexpr bool has_unit() const { return unit_length > Mm{0}; }

  constexpr bool operator==(const BuiltLayer&) const = default;
};

// What a wall is built of (docs/adr/0014): what it is for and its layers from its outer face in.
// Geometry, drawing and take-off read walls through this and nothing else.
struct Build {
  std::optional<WallRole> role;  // absent for a wall with no type: nobody has said
  std::vector<BuiltLayer> layers;

  // The sum of the layers; zero for a wall whose type is missing or has no layer.
  [[nodiscard]] Mm thickness() const;

  bool operator==(const Build&) const = default;
};

// The layers of a wall type in the order of their positions; layers at one position keep the
// order of their ids. Empty for a type that is not in the model.
[[nodiscard]] std::vector<BuiltLayer> layers_of(const Model& model, ElementId type);

// A wall with a type is built as its type says. A wall with none is one structure layer of its
// own thickness and unit.
[[nodiscard]] Build build_of(const Model& model, const Wall& wall);

// The wall as geometry reads it: as thick as what it is built of and naming no type, so that
// validate() takes it for the plain wall it stands for.
[[nodiscard]] Wall geometric(const Wall& wall, const Build& build);

}  // namespace otves
