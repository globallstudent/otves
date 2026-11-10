#pragma once

#include <compare>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <vector>

#include <otves/core/error.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {

// A brick or block as laid: length along the wall, width across it, height of one course.
struct MasonryUnit {
  Mm length;
  Mm width;
  Mm height;

  constexpr auto operator<=>(const MasonryUnit&) const = default;
};

// Mortar joint thickness: bed joints between courses, head joints between units in a course.
struct Joints {
  Mm bed;
  Mm head;
};

// Unit and joint sizes come from the catalog and the job's norm profile, never from code.
struct MasonrySpec {
  MasonryUnit unit;
  Joints joints;
};

// Checks values that come from the user or from content; never asserts.
[[nodiscard]] std::expected<void, Error> validate(const MasonrySpec& spec);

enum class MasonryIssueKind : std::uint8_t {
  // The thickness is not a whole number of unit widths with head joints between them. Units are
  // counted pro rata, as if the leftover strip were laid from cut units.
  kPartialWythe,
  // The wall is thinner than one unit; it gets no units and no mortar.
  kThinnerThanUnit,
};

struct MasonryIssue {
  ElementId wall;
  MasonryIssueKind kind = MasonryIssueKind::kPartialWythe;

  constexpr auto operator<=>(const MasonryIssue&) const = default;
};

// Net quantities. Waste allowances and rounding to whole units or pallets are estimate rules.
struct WallMasonry {
  ElementId id;
  double units = 0.0;   // pieces, not rounded
  double mortar = 0.0;  // m³
};

// What the walls laid from one unit take together.
struct UnitMasonry {
  MasonryUnit unit;
  double units = 0.0;   // pieces, not rounded
  double mortar = 0.0;  // m³
};

struct Masonry {
  std::vector<WallMasonry> walls;    // in input order
  std::vector<UnitMasonry> by_unit;  // sorted by unit; only units some wall is laid from
  std::vector<MasonryIssue> issues;  // sorted
  double units = 0.0;                // sum over walls, in input order
  double mortar = 0.0;
};

// Units and mortar for walls laid from one unit. A wall of n wythes holds n units per
// (length + head) × (height + bed) cell of its face; mortar is the rest of the volume.
[[nodiscard]] std::expected<Masonry, Error> count_masonry(std::span<const WallQuantities> walls,
                                                          const MasonrySpec& spec);

// The same for walls laid from different units: `units[i]` is the unit of `walls[i]`, absent when
// none was chosen (docs/adr/0008); such a wall gets no units and no mortar.
// Precondition: `units` is as long as `walls`.
[[nodiscard]] std::expected<Masonry, Error> count_masonry(
    std::span<const WallQuantities> walls, std::span<const std::optional<MasonryUnit>> units,
    const Joints& joints);

}  // namespace otves::takeoff
