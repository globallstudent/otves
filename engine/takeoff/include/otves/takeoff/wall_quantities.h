#pragma once

#include <compare>
#include <cstdint>
#include <span>
#include <vector>

#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/geom/wall_joins.h>

namespace otves::takeoff {

struct OpeningInput {
  ElementId id;
  Opening opening;
};

// What one joined wall measures. Joined outlines tile the plan, so a corner is split between its
// walls and every quantity adds up across walls. Face area and volume are net of openings.
struct WallQuantities {
  ElementId id;
  Mm thickness;
  double footprint_area = 0.0;  // m², the joined plan outline
  double length = 0.0;          // m, footprint area over thickness
  double face_area = 0.0;       // m², one face: length × height, less openings
  double volume = 0.0;          // m³, footprint area × height, less openings × thickness
  double openings_area = 0.0;   // m², the openings cut from one face
};

enum class OpeningIssueKind : std::uint8_t {
  // The opening reaches past its wall's ends, below its base or above its top; only the part
  // inside the wall is cut.
  kOutsideWall,
  // The opening overlaps another opening in the same wall; the shared part is cut once.
  kOverlap,
  // The opening's wall is not among the measured walls; it is ignored.
  kNoWall,
};

struct OpeningIssue {
  ElementId opening;
  OpeningIssueKind kind = OpeningIssueKind::kOutsideWall;

  constexpr auto operator<=>(const OpeningIssue&) const = default;
};

struct Measurement {
  std::vector<WallQuantities> walls;  // one per footprint in the joins, sorted by id
  std::vector<OpeningIssue> issues;   // sorted, no duplicates
};

// Walls are measured as drawn: where join_walls reported an overlap, the shared part counts in
// both walls. Openings sit on the wall's location line, from its start and up from its base.
// Precondition: `joins` is join_walls(walls), and every opening passes validate().
[[nodiscard]] Measurement measure_walls(std::span<const geom::WallInput> walls,
                                        std::span<const OpeningInput> openings,
                                        const geom::WallJoins& joins);

}  // namespace otves::takeoff
