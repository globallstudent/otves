#pragma once

#include <optional>
#include <vector>

#include <otves/core/command.h>
#include <otves/core/error.h>
#include <otves/core/model.h>
#include <otves/core/room.h>
#include <otves/core/uuid.h>
#include <otves/drafting/plan.h>
#include <otves/geom/editing.h>
#include <otves/geom/spaces.h>

#include "convert.h"

namespace otves::bridge {

// The opening a tap places: in the wall on the level nearest to the tap within reach, centred on
// the tap and moved along the wall to fit between its ends.
[[nodiscard]] Result<AddOpening> place_opening(const Model& model,
                                               const OpeningPlacement& placement);

// Where a wall being drawn on a level would end: by the level's walls, and by the wall ends of the
// level under it, so walls stand on walls. kNotFound when the level does not exist.
[[nodiscard]] Result<geom::Aimed> aim(const Model& model, const Sight& sight);
[[nodiscard]] Result<geom::Aimed> aim(const Model& model, const MeasuredSight& sight);

// The commands that give a wall its length: the moves that carry what is joined to it
// (geom::resize_wall), each checked to succeed, and the wall made exact if it was not. kNotFound
// when the wall does not exist.
[[nodiscard]] Result<std::vector<Command>> resize(const Model& model, const Resize& resize);

// The commands that give a size on the plan its typed value (docs/adr/0016): an opening's width
// with its near edge kept, a piece of wall by moving the opening it leads to, a wall's length as
// resize() gives it. kNotFound when the element does not exist, kDoesNotFit when the opening
// would leave its wall.
[[nodiscard]] Result<std::vector<Command>> dimension(const Model& model, const Sized& sized);

// The walls a stroke means on its level (geom::straighten), each checked to be buildable and
// not exact. kNotFound when the level does not exist.
[[nodiscard]] Result<std::vector<Command>> sketch(const Model& model, const Stroke& stroke);

// The level as a display list (drafting::draft): its walls and openings, over the walls of the
// level under it. kNotFound when the level does not exist.
[[nodiscard]] Result<drafting::Drawing> draw(const Model& model, const View& view);

// A space the walls of a level close, with the room that names it (docs/adr/0015).
struct FoundSpace {
  geom::Space space;
  ElementId room;  // nil for a space with no name
  RoomKind kind = RoomKind::kRoom;
};

// What a tap found: an element, or else the space it lies in.
struct Found {
  std::optional<ElementId> element;
  std::optional<FoundSpace> space;
};

// What a tap picks on its level: an opening or wall under the point, else the nearest opening,
// else the nearest wall within reach, else the space the point lies in. kNotFound when the level
// does not exist.
[[nodiscard]] Result<Found> pick(const Model& model, const Tap& tap);

}  // namespace otves::bridge
