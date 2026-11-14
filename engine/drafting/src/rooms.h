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

// A stretch of a space's floor along a line across or up the plan, as the coordinates of its
// ends along that line.
struct Stretch {
  double from = 0.0;
  double to = 0.0;

  [[nodiscard]] constexpr double length() const { return to - from; }
  [[nodiscard]] constexpr double middle() const { return (from + to) / 2; }
  [[nodiscard]] constexpr bool holds(double value) const { return value >= from && value <= to; }
};

// The stretch of the space's floor across the plan at the height `y` that holds `x`, or else the
// longest one there. Nothing where the line misses the space.
[[nodiscard]] std::optional<Stretch> stretch_across(const geom::Space& space, double y, double x);

// The same up the plan: the stretch through `x` that holds `y`, or else the longest one there.
[[nodiscard]] std::optional<Stretch> stretch_up(const geom::Space& space, double x, double y);

// Where a space's name and area are written.
struct RoomLabel {
  geom::Vec2 at;
  geom::Vec2 along;     // +x, or +y in a space too narrow for a name
  double fit = 0.0;     // the space's free stretch through `at` along the text, in metres
  double across = 0.0;  // and across it
  Stretch wide;         // the floor across the plan through `at`
  Stretch tall;         // and up the plan
};

// The middle of the space's widest stretch: of the one across the plan at the height of its
// centre, then of the one up the plan through that point. Nothing for a space with no floor.
[[nodiscard]] std::optional<RoomLabel> label_of(const geom::Space& space, TextSize text);

// The level's room schedule: every space with a floor, in reading order: from the top of the
// plan down and, at one height, from the left, by where each would be labelled.
[[nodiscard]] std::vector<ListedRoom> list_rooms(std::span<const RoomSpace> spaces, TextSize text);

// The floors, in the order given.
void draw_floors(std::span<const RoomSpace> spaces, Drawing& drawing);

// A room's name is taken to be at most this many digits wide and its area this many: the engine
// does not know the words.
inline constexpr double kNameDigits = 16.0;
inline constexpr double kAreaDigits = 7.0;

// A part of the plan between two corners, along its axes.
struct Bounds {
  geom::Vec2 low;
  geom::Vec2 high;
};

// Each room's name over its area, or the area alone in a space with no name or too low for two
// lines. A space lower than a line gets nothing, and so does `focused`, which is drawn with its
// sizes. A label that would lie on one of `swept`, what the doors' leaves sweep, moves along
// its line to the longest part that is clear, if that part has room for it.
void write_rooms(std::span<const RoomSpace> spaces, const RoomSpace* focused,
                 std::span<const Bounds> swept, TextSize text, Drawing& drawing);

}  // namespace otves::drafting
