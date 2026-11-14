#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <variant>

#include <otves/core/element.h>
#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>

namespace otves {

// Which line of the wall the start and end points lie on, looking from start to end. Builders
// measure outside faces, so a wall drawn along a measured face keeps that face fixed when its
// thickness changes.
enum class LocationLine : std::uint8_t {
  kCenter,
  kLeftFace,
  kRightFace,
};

// A straight wall on one level. Parameters only: geometry is derived by engine/geom. The level
// is set when the wall is added; Document checks that it exists.
struct Wall {
  // The explicit initializer lets geometry code build walls without naming a level.
  ElementId level{};  // NOLINT(readability-redundant-member-init)
  PointMm start;
  PointMm end;
  Mm thickness;
  Mm height;
  LocationLine location_line = LocationLine::kCenter;
  // The brick or block the wall is laid from, as laid: along the wall, across it, one course.
  // All zero until the builder chooses (docs/adr/0008); the explicit initializers let geometry
  // code build walls without naming a unit.
  Mm unit_length{};  // NOLINT(readability-redundant-member-init)
  Mm unit_width{};   // NOLINT(readability-redundant-member-init)
  Mm unit_height{};  // NOLINT(readability-redundant-member-init)
  // Whether the wall's length was typed or measured. A wall read from a sketch is not exact until
  // it gets a length (docs/adr/0012).
  bool exact = true;
  // What kind of wall this is (docs/adr/0014); nil for a wall with no type. A wall with a type
  // has no thickness and no unit of its own: its type decides them. Document checks that the
  // type exists.
  ElementId type{};  // NOLINT(readability-redundant-member-init)
  // The type's layers run from the wall's outer face in: its right face, looking from start to
  // end, or its left one when this is set.
  bool mirrored = false;

  // Whether a masonry unit was chosen for a wall with no type.
  [[nodiscard]] constexpr bool has_unit() const { return unit_length > Mm{0}; }

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
  kUnitLength,
  kUnitWidth,
  kUnitHeight,
  kExact,
  kType,
  kMirrored,
};

using WallValue = std::variant<PointMm, Mm, LocationLine, bool, ElementId>;

[[nodiscard]] WallValue get_field(const Wall& wall, WallField field);

// The value's alternative must match the field (PointMm for kStart/kEnd, LocationLine for
// kLocationLine, bool for kExact and kMirrored, ElementId for kType, Mm for the rest); a mismatch
// is a programming error.
void set_field(Wall& wall, WallField field, const WallValue& value);

template <>
struct ElementTraits<Wall> {
  using Field = WallField;
  using Value = WallValue;
  static constexpr std::string_view kName = "wall";
};

}  // namespace otves
