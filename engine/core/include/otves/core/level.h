#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <variant>

#include <otves/core/element.h>
#include <otves/core/error.h>
#include <otves/core/units.h>

namespace otves {

// A storey (этаж), from a basement to the attic. Walls stand on a level, and their heights are
// measured up from its floor.
struct Level {
  Mm elevation;  // of the floor above the project zero (±0,000)
  Mm height;     // floor to floor

  constexpr bool operator==(const Level&) const = default;
};

// Checks values that come from the user; never asserts.
[[nodiscard]] std::expected<void, Error> validate(const Level& level);

enum class LevelField : std::uint8_t {
  kElevation,
  kHeight,
};

using LevelValue = std::variant<Mm>;

[[nodiscard]] LevelValue get_field(const Level& level, LevelField field);
void set_field(Level& level, LevelField field, const LevelValue& value);

template <>
struct ElementTraits<Level> {
  using Field = LevelField;
  using Value = LevelValue;
  static constexpr std::string_view kName = "level";
};

}  // namespace otves
