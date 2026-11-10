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
  // Seismic detailing the builder chose for this storey (docs/adr/0008); the take-off counts only
  // what is set here. The explicit initializer lets a level be built without naming a ring beam.
  // Height of the beam on top of the load-bearing walls; zero for none.
  Mm ring_beam{};  // NOLINT(readability-redundant-member-init)
  // Meshes in the bed joints where load-bearing walls meet.
  bool junction_meshes = false;
  // The section of the strip footing under the load-bearing walls (docs/adr/0010); both zero for
  // none. The explicit initializers let a level be built without naming a footing.
  Mm footing_width{};  // NOLINT(readability-redundant-member-init)
  Mm footing_depth{};  // NOLINT(readability-redundant-member-init)

  constexpr bool operator==(const Level&) const = default;
};

// Checks values that come from the user; never asserts.
[[nodiscard]] std::expected<void, Error> validate(const Level& level);

enum class LevelField : std::uint8_t {
  kElevation,
  kHeight,
  kRingBeam,
  kJunctionMeshes,
  kFootingWidth,
  kFootingDepth,
};

using LevelValue = std::variant<Mm, bool>;

[[nodiscard]] LevelValue get_field(const Level& level, LevelField field);
// The value's alternative must match the field (bool for kJunctionMeshes, Mm for the rest); a
// mismatch is a programming error.
void set_field(Level& level, LevelField field, const LevelValue& value);

template <>
struct ElementTraits<Level> {
  using Field = LevelField;
  using Value = LevelValue;
  static constexpr std::string_view kName = "level";
};

}  // namespace otves
