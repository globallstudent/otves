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
  return {};
}

LevelValue get_field(const Level& level, LevelField field) {
  switch (field) {
    case LevelField::kElevation:
      return level.elevation;
    case LevelField::kHeight:
      return level.height;
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
  }
  OTVES_ASSERT(false);
}

}  // namespace otves
