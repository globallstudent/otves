#pragma once

#include <compare>
#include <cstdint>
#include <expected>
#include <span>
#include <vector>

#include <otves/core/error.h>
#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/geom/wall_joins.h>

namespace otves::takeoff {

struct OpeningInput {
  ElementId id;
  Opening opening;
};

// Precast lintels (перемычки) over openings: the wall's full thickness, resting on the masonry on
// both sides. Bearings come from the norm profile, the height from the chosen lintel section.
struct LintelSpec {
  Mm height;         // section height, such as 140 mm for 2ПБ
  Mm bearing;        // on each side of the opening
  Mm short_bearing;  // on each side of an opening no wider than short_span
  Mm short_span;
};

// Checks values that come from content or the user; never asserts.
[[nodiscard]] std::expected<void, Error> validate(const LintelSpec& spec);

struct Lintel {
  ElementId opening;
  ElementId wall;
  double length = 0.0;  // m to order: opening width plus both bearings
  double height = 0.0;  // m
  Mm thickness;         // the wall's
};

// What one joined wall measures. Joined outlines tile the plan, so a corner is split between its
// walls and every quantity adds up across walls. Face area and volume are net of openings and
// lintels: neither is masonry.
struct WallQuantities {
  ElementId id;
  Mm thickness;
  double footprint_area = 0.0;  // m², the joined plan outline
  double length = 0.0;          // m, footprint area over thickness
  double face_area = 0.0;       // m², one face: length × height, less openings and lintels
  double volume = 0.0;          // m³, footprint area × height, less their face × thickness
  double openings_area = 0.0;   // m², the openings cut from one face
  // m, the height of the ring beam the wall's level asks for on top of it; zero for none. The
  // caller sets it from the level (docs/adr/0008); measuring leaves it zero.
  double ring_beam = 0.0;
  // m, the section of the strip footing the level asks for under the wall; zero for none. Set by
  // the caller, as ring_beam (docs/adr/0010).
  double footing_width = 0.0;
  double footing_depth = 0.0;
};

// A place where walls meet, with the height they share there.
struct JunctionQuantities {
  std::vector<ElementId> walls;  // sorted
  double height = 0.0;           // m, the lowest of the walls
  // Whether the walls' level asks for meshes where walls meet; set by the caller, as ring_beam.
  bool meshes = false;
};

enum class OpeningIssueKind : std::uint8_t {
  // The opening reaches past its wall's ends, below its base or above its top; only the part
  // inside the wall is cut.
  kOutsideWall,
  // The opening overlaps another opening in the same wall; the shared part is cut once.
  kOverlap,
  // The opening's wall is not among the measured walls; it is ignored.
  kNoWall,
  // The lintel's bearing runs past the wall's end, into another opening or into another lintel:
  // the pier beside the opening is too narrow.
  kShortBearing,
  // The lintel rises above the wall's top; only the part inside the wall is deducted.
  kNoRoomForLintel,
};

struct OpeningIssue {
  ElementId opening;
  OpeningIssueKind kind = OpeningIssueKind::kOutsideWall;

  constexpr auto operator<=>(const OpeningIssue&) const = default;
};

struct Measurement {
  std::vector<WallQuantities> walls;          // one per footprint in the joins, sorted by id
  std::vector<Lintel> lintels;                // one per opening inside a wall, sorted by opening
  std::vector<JunctionQuantities> junctions;  // in the order of the joins' junctions
  std::vector<OpeningIssue> issues;           // sorted, no duplicates
};

// Walls are measured as drawn: where join_walls reported an overlap, the shared part counts in
// both walls. Openings sit on the wall's location line, from its start and up from its base; a
// lintel sits on top of each. Fails only when `lintels` is invalid.
// Precondition: `joins` is join_walls(walls), and every opening passes validate().
[[nodiscard]] Result<Measurement> measure_walls(std::span<const geom::WallInput> walls,
                                                std::span<const OpeningInput> openings,
                                                const LintelSpec& lintels,
                                                const geom::WallJoins& joins);

// Joins and measures the walls of each level on their own, so walls on different floors never
// meet, and merges the results: walls sorted by id, lintels by opening, junctions level by level.
[[nodiscard]] Result<Measurement> measure_levels(std::span<const geom::WallInput> walls,
                                                 std::span<const OpeningInput> openings,
                                                 const LintelSpec& lintels);

}  // namespace otves::takeoff
