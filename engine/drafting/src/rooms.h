#pragma once

#include <optional>
#include <span>
#include <vector>

#include <otves/core/room.h>
#include <otves/core/uuid.h>
#include <otves/drafting/plan.h>
#include <otves/geom/spaces.h>
#include <otves/geom/wall_joins.h>

namespace otves::drafting {

// A space the walls close, with the room that names it.
struct RoomSpace {
  geom::Space space;
  ElementId room;  // nil for a space with no name
  RoomKind kind = RoomKind::kRoom;
};

// The spaces of a level, the largest first, each with the oldest room whose anchor it holds.
[[nodiscard]] std::vector<RoomSpace> room_spaces(std::span<const geom::WallInput> walls,
                                                 std::span<const NamedRoom> rooms);

// Where a space's name and area are written.
struct RoomLabel {
  geom::Vec2 at;
  geom::Vec2 along;     // +x, or +y in a space too narrow for a name
  double fit = 0.0;     // the space's free stretch through `at` along the text, in metres
  double across = 0.0;  // and across it
};

// The middle of the space's widest stretch: of the one across the plan at the height of its
// centre, then of the one up the plan through that point. Nothing for a space with no floor.
[[nodiscard]] std::optional<RoomLabel> label_of(const geom::Space& space, TextSize text);

// The floors, in the order given.
void draw_floors(std::span<const RoomSpace> spaces, Drawing& drawing);

// Each room's name over its area, or the area alone in a space with no name or too low for two
// lines. A space lower than a line gets nothing.
void write_rooms(std::span<const RoomSpace> spaces, TextSize text, Drawing& drawing);

}  // namespace otves::drafting
