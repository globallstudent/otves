#include <expected>

#include <otves/core/assert.h>
#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/wall.h>

#include "field.h"

namespace otves {
namespace {

constexpr bool on_plan(PointMm point) {
  const auto within = [](Mm value) { return value >= -kMaxCoordinate && value <= kMaxCoordinate; };
  return within(point.x) && within(point.y);
}

}  // namespace

std::expected<void, Error> validate(const Wall& wall) {
  if (!on_plan(wall.start) || !on_plan(wall.end)) {
    return make_error(ErrorCode::kOffPlan, "wall point lies more than 100 km from the origin");
  }
  if (wall.start == wall.end) {
    return make_error(ErrorCode::kZeroLength, "wall start and end coincide");
  }
  if (wall.thickness <= Mm{0}) {
    return make_error(ErrorCode::kThicknessNotPositive, "wall thickness must be positive");
  }
  if (wall.height <= Mm{0}) {
    return make_error(ErrorCode::kHeightNotPositive, "wall height must be positive");
  }
  const bool none =
      wall.unit_length == Mm{0} && wall.unit_width == Mm{0} && wall.unit_height == Mm{0};
  const bool whole =
      wall.unit_length > Mm{0} && wall.unit_width > Mm{0} && wall.unit_height > Mm{0};
  if (!none && !whole) {
    return make_error(ErrorCode::kUnitSizeNotPositive,
                      "masonry unit sizes must be all positive or all zero");
  }
  return {};
}

WallValue get_field(const Wall& wall, WallField field) {
  switch (field) {
    case WallField::kStart:
      return wall.start;
    case WallField::kEnd:
      return wall.end;
    case WallField::kThickness:
      return wall.thickness;
    case WallField::kHeight:
      return wall.height;
    case WallField::kLocationLine:
      return wall.location_line;
    case WallField::kUnitLength:
      return wall.unit_length;
    case WallField::kUnitWidth:
      return wall.unit_width;
    case WallField::kUnitHeight:
      return wall.unit_height;
  }
  OTVES_ASSERT(false);
  return wall.start;
}

void set_field(Wall& wall, WallField field, const WallValue& value) {
  switch (field) {
    case WallField::kStart:
      assign(wall.start, value);
      return;
    case WallField::kEnd:
      assign(wall.end, value);
      return;
    case WallField::kThickness:
      assign(wall.thickness, value);
      return;
    case WallField::kHeight:
      assign(wall.height, value);
      return;
    case WallField::kLocationLine:
      assign(wall.location_line, value);
      return;
    case WallField::kUnitLength:
      assign(wall.unit_length, value);
      return;
    case WallField::kUnitWidth:
      assign(wall.unit_width, value);
      return;
    case WallField::kUnitHeight:
      assign(wall.unit_height, value);
      return;
  }
  OTVES_ASSERT(false);
}

}  // namespace otves
