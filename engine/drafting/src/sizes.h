#pragma once

#include <span>

#include <otves/drafting/plan.h>
#include <otves/geom/openings.h>
#include <otves/geom/wall_joins.h>

#include "rooms.h"

namespace otves::drafting {

// The plan's overall size (docs/adr/0016): across its top and up its left side, from outer face
// to outer face of everything built. Nothing for a plan with no walls.
void write_overall(std::span<const geom::WallFootprint> footprints, TextSize text,
                   Drawing& drawing);

// The sizes of the space in focus: its clear size across, near its top, and up the plan, near
// its left side; and along each side with openings a chain of wall, opening, wall. Each number
// says what a typed value changes. The lines are drawn in `stroke`.
void write_sizes(const RoomSpace& room, std::span<const geom::WallInput> walls,
                 std::span<const geom::PlacedOpening> openings, Stroke stroke, TextSize text,
                 Drawing& drawing);

// The chain of the side the selected opening is in: wall, opening, wall along that side of a
// space, the one a door opens into where there is one (docs/adr/0017). Nothing when `selected`
// is no opening of the plan or its wall is round no space.
void write_opening_sizes(std::span<const RoomSpace> spaces, std::span<const geom::WallInput> walls,
                         std::span<const geom::PlacedOpening> openings, ElementId selected,
                         TextSize text, Drawing& drawing);

}  // namespace otves::drafting
