#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <variant>

#include <otves/core/element.h>
#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>

namespace otves {

// Which line of the wall the start and end points lie on, looking from start to end. Builders
// measure outside faces, so a wall drawn along a measured face keeps that face fixed when its
// thickness changes.
enum class LocationLine : std::uint8_t {
  kCenter,
  kLeftFace,
  kRightFace,
};

// A straight wall on one level. Parameters only: geometry is derived by engine/geom.
struct Wall {
  PointMm start;
  PointMm end;
  Mm thickness;
  Mm height;
  LocationLine location_line = LocationLine::kCenter;

  constexpr bool operator==(const Wall&) const = default;
};

enum class WallEnd : std::uint8_t {
  kStart,
  kEnd,
};

// Checks values that come from the user; never asserts.
[[nodiscard]] std::expected<void, Error> validate(const Wall& wall);

enum class WallField : std::uint8_t {
  kStart,
  kEnd,
  kThickness,
  kHeight,
  kLocationLine,
};

using WallValue = std::variant<PointMm, Mm, LocationLine>;

[[nodiscard]] WallValue get_field(const Wall& wall, WallField field);

// The value's alternative must match the field (PointMm for kStart/kEnd, Mm for kThickness and
// kHeight, LocationLine for kLocationLine); a mismatch is a programming error.
void set_field(Wall& wall, WallField field, const WallValue& value);

template <>
struct ElementTraits<Wall> {
  using Field = WallField;
  using Value = WallValue;
  static constexpr std::string_view kName = "wall";
};

}  // namespace otves
