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
};

using OpeningValue = std::variant<Mm, OpeningKind>;

[[nodiscard]] OpeningValue get_field(const Opening& opening, OpeningField field);

// The value's alternative must match the field (OpeningKind for kKind, Mm for the rest); a
// mismatch is a programming error.
void set_field(Opening& opening, OpeningField field, const OpeningValue& value);

template <>
struct ElementTraits<Opening> {
  using Field = OpeningField;
  using Value = OpeningValue;
  static constexpr std::string_view kName = "opening";
};

}  // namespace otves
