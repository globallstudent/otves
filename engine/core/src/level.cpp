#include <expected>

#include <otves/core/assert.h>
#include <otves/core/error.h>
#include <otves/core/level.h>
#include <otves/core/units.h>

#include "field.h"

namespace otves {

std::expected<void, Error> validate(const Level& level) {
  if (level.height <= Mm{0}) {
    return make_error(ErrorCode::kHeightNotPositive, "level height must be positive");
  }
  if (level.ring_beam < Mm{0}) {
    return make_error(ErrorCode::kHeightNotPositive, "ring beam height must not be negative");
  }
  const bool none = level.footing_width == Mm{0} && level.footing_depth == Mm{0};
  if (!none && (level.footing_width <= Mm{0} || level.footing_depth <= Mm{0})) {
    return make_error(ErrorCode::kHeightNotPositive,
                      "a footing's width and depth must be both positive or both zero");
  }
  return {};
}

LevelValue get_field(const Level& level, LevelField field) {
  switch (field) {
    case LevelField::kElevation:
      return level.elevation;
    case LevelField::kHeight:
      return level.height;
    case LevelField::kRingBeam:
      return level.ring_beam;
    case LevelField::kJunctionMeshes:
      return level.junction_meshes;
    case LevelField::kFootingWidth:
      return level.footing_width;
    case LevelField::kFootingDepth:
      return level.footing_depth;
  }
  OTVES_ASSERT(false);
  return level.elevation;
}

void set_field(Level& level, LevelField field, const LevelValue& value) {
  switch (field) {
    case LevelField::kElevation:
      assign(level.elevation, value);
      return;
    case LevelField::kHeight:
      assign(level.height, value);
      return;
    case LevelField::kRingBeam:
      assign(level.ring_beam, value);
      return;
    case LevelField::kJunctionMeshes:
      assign(level.junction_meshes, value);
      return;
    case LevelField::kFootingWidth:
      assign(level.footing_width, value);
      return;
    case LevelField::kFootingDepth:
      assign(level.footing_depth, value);
      return;
  }
  OTVES_ASSERT(false);
}

}  // namespace otves
