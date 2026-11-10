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
  }
  OTVES_ASSERT(false);
}

}  // namespace otves
