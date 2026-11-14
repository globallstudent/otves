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

enum class OpeningKind : std::uint8_t {
  kWindow,
  kDoor,
  kEmpty,  // no door or window in it (пустой проём)
};

// The edge of an opening a door's leaf hangs on (docs/adr/0017).
enum class Hinge : std::uint8_t {
  kNear,  // the edge towards the wall's start
  kFar,
};

// The side of its wall a door's leaf opens to, looking from the wall's start.
enum class Swing : std::uint8_t {
  kLeft,
  kRight,
};

// A window, door or empty opening (проём) through a wall, measured along the wall's location
// line from its start and up from its base. The model keeps what the user set: whether the
// opening still fits its wall is checked on derived geometry, because the wall may change later.
struct Opening {
  ElementId wall;
  Mm offset;  // from the wall's start to the near edge of the opening
  Mm width;
  Mm height;
  Mm sill;  // from the wall's base to the bottom of the opening; zero for a door
  OpeningKind kind = OpeningKind::kWindow;
  // How a door hangs; a window and an empty opening keep these unused.
  Hinge hinge = Hinge::kNear;
  Swing swing = Swing::kLeft;

  constexpr bool operator==(const Opening&) const = default;
};

// Checks values that come from the user; never asserts. Document checks that the wall exists.
[[nodiscard]] std::expected<void, Error> validate(const Opening& opening);

enum class OpeningField : std::uint8_t {
  kOffset,
  kWidth,
  kHeight,
  kSill,
  kKind,
  kHinge,
  kSwing,
};

using OpeningValue = std::variant<Mm, OpeningKind, Hinge, Swing>;

[[nodiscard]] OpeningValue get_field(const Opening& opening, OpeningField field);

// The value's alternative must match the field (OpeningKind for kKind, Hinge for kHinge, Swing
// for kSwing, Mm for the rest); a mismatch is a programming error.
void set_field(Opening& opening, OpeningField field, const OpeningValue& value);

template <>
struct ElementTraits<Opening> {
  using Field = OpeningField;
  using Value = OpeningValue;
  static constexpr std::string_view kName = "opening";
};

}  // namespace otves
