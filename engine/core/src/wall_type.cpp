#include <expected>

#include <otves/core/assert.h>
#include <otves/core/error.h>
#include <otves/core/units.h>
#include <otves/core/wall_type.h>

#include "field.h"

namespace otves {

std::expected<void, Error> validate(const WallType& /*type*/) { return {}; }

// The role is the only field so far.
WallTypeValue get_field(const WallType& type, WallTypeField field) {
  OTVES_ASSERT(field == WallTypeField::kRole);
  return type.role;
}

void set_field(WallType& type, WallTypeField field, const WallTypeValue& value) {
  OTVES_ASSERT(field == WallTypeField::kRole);
  assign(type.role, value);
}

std::expected<void, Error> validate(const WallLayer& layer) {
  if (layer.thickness <= Mm{0}) {
    return make_error(ErrorCode::kThicknessNotPositive, "layer thickness must be positive");
  }
  const bool none =
      layer.unit_length == Mm{0} && layer.unit_width == Mm{0} && layer.unit_height == Mm{0};
  const bool whole =
      layer.unit_length > Mm{0} && layer.unit_width > Mm{0} && layer.unit_height > Mm{0};
  if (!none && !whole) {
    return make_error(ErrorCode::kUnitSizeNotPositive,
                      "masonry unit sizes must be all positive or all zero");
  }
  if (!none && !layer.is_masonry()) {
    return make_error(ErrorCode::kUnitSizeNotPositive,
                      "only structure and facing layers are laid from units");
  }
  return {};
}

WallLayerValue get_field(const WallLayer& layer, WallLayerField field) {
  switch (field) {
    case WallLayerField::kThickness:
      return layer.thickness;
    case WallLayerField::kUnitLength:
      return layer.unit_length;
    case WallLayerField::kUnitWidth:
      return layer.unit_width;
    case WallLayerField::kUnitHeight:
      return layer.unit_height;
  }
  OTVES_ASSERT(false);
  return layer.thickness;
}

void set_field(WallLayer& layer, WallLayerField field, const WallLayerValue& value) {
  switch (field) {
    case WallLayerField::kThickness:
      assign(layer.thickness, value);
      return;
    case WallLayerField::kUnitLength:
      assign(layer.unit_length, value);
      return;
    case WallLayerField::kUnitWidth:
      assign(layer.unit_width, value);
      return;
    case WallLayerField::kUnitHeight:
      assign(layer.unit_height, value);
      return;
  }
  OTVES_ASSERT(false);
}

}  // namespace otves
