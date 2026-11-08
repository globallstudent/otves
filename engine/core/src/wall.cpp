#include <expected>
#include <variant>

#include <otves/core/assert.h>
#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/wall.h>

namespace otves {
namespace {

constexpr bool on_plan(PointMm point) {
  const auto within = [](Mm value) { return value >= -kMaxCoordinate && value <= kMaxCoordinate; };
  return within(point.x) && within(point.y);
}

template <typename T>
void assign(T& target, const WallValue& value) {
  const T* typed = std::get_if<T>(&value);
  OTVES_ASSERT(typed != nullptr);
  if (typed != nullptr) {
    target = *typed;
  }
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
  }
  OTVES_ASSERT(false);
}

}  // namespace otves
