#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <variant>

#include <otves/core/element.h>
#include <otves/core/error.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>

namespace otves {

// What a wall is for. Exterior and bearing walls carry load; partitions do not.
enum class WallRole : std::uint8_t {
  kExterior,
  kBearing,
  kPartition,
};

// A kind of wall the job builds with (docs/adr/0014). Its layers are WallLayer elements that
// name it. It has no name: the app writes one from the role and the sum of the layers.
struct WallType {
  WallRole role = WallRole::kExterior;

  constexpr bool operator==(const WallType&) const = default;
};

[[nodiscard]] std::expected<void, Error> validate(const WallType& type);

enum class WallTypeField : std::uint8_t {
  kRole,
};

using WallTypeValue = std::variant<WallRole>;

[[nodiscard]] WallTypeValue get_field(const WallType& type, WallTypeField field);
void set_field(WallType& type, WallTypeField field, const WallTypeValue& value);

template <>
struct ElementTraits<WallType> {
  using Field = WallTypeField;
  using Value = WallTypeValue;
  static constexpr std::string_view kName = "wall type";
};

// What a layer of a wall does.
enum class LayerFunction : std::uint8_t {
  kStructure,   // the masonry that carries the wall
  kInsulation,  // bought by area
  kFacing,      // masonry laid in front of the wall
  kGap,         // air: nothing is bought for it
};

// One layer of a wall type. Layers of a type run from the wall's outer face in, in the order of
// their positions; Document checks that the type exists.
struct WallLayer {
  ElementId type;
  std::int64_t position = 0;
  LayerFunction function = LayerFunction::kStructure;
  Mm thickness;
  // The brick or block a masonry layer is laid from, as laid; all zero until chosen, and always
  // for insulation and gaps. The explicit initializers let a layer be built without a unit.
  Mm unit_length{};  // NOLINT(readability-redundant-member-init)
  Mm unit_width{};   // NOLINT(readability-redundant-member-init)
  Mm unit_height{};  // NOLINT(readability-redundant-member-init)

  // Whether the layer is laid from units: structure and facing are.
  [[nodiscard]] constexpr bool is_masonry() const {
    return function == LayerFunction::kStructure || function == LayerFunction::kFacing;
  }
  [[nodiscard]] constexpr bool has_unit() const { return unit_length > Mm{0}; }

  constexpr bool operator==(const WallLayer&) const = default;
};

// Checks values that come from the user; never asserts.
[[nodiscard]] std::expected<void, Error> validate(const WallLayer& layer);

enum class WallLayerField : std::uint8_t {
  kThickness,
  kUnitLength,
  kUnitWidth,
  kUnitHeight,
};

using WallLayerValue = std::variant<Mm>;

[[nodiscard]] WallLayerValue get_field(const WallLayer& layer, WallLayerField field);
void set_field(WallLayer& layer, WallLayerField field, const WallLayerValue& value);

template <>
struct ElementTraits<WallLayer> {
  using Field = WallLayerField;
  using Value = WallLayerValue;
  static constexpr std::string_view kName = "wall layer";
};

}  // namespace otves
