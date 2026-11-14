#include "rooms.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/room.h>
#include <otves/core/uuid.h>
#include <otves/drafting/plan.h>
#include <otves/geom/polygon.h>
#include <otves/geom/spaces.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

namespace otves::drafting {
namespace {

// A name needs about this many digit widths; a space narrower than that and taller than it is
// wide is written up the plan.
constexpr double kNameDigits = 8.0;

// A text takes at most this share of the stretch it lies in.
constexpr double kLabelFill = 0.8;

constexpr double kCm2PerM2 = 10'000.0;

// The centre of area divides the sum over an outline's edges by three times its doubled area.
constexpr double kCentreShare = 3.0;

// Along which axis a stretch runs. The other coordinate is the line it lies on.
enum class Axis : std::uint8_t { kX, kY };

constexpr double along(geom::Vec2 point, Axis axis) { return axis == Axis::kX ? point.x : point.y; }
constexpr double across(geom::Vec2 point, Axis axis) {
  return axis == Axis::kX ? point.y : point.x;
}

// Where the line crosses the outline's edges. An edge is crossed when its ends lie on different
// sides of the line, an end on the line counting as below it, so a corner is counted once.
void add_crossings(std::span<const geom::Vec2> outline, Axis axis, double line,
                   std::vector<double>& crossings) {
  for (std::size_t i = 0; i < outline.size(); ++i) {
    const geom::Vec2 a = outline[i];
    const geom::Vec2 b = outline[(i + 1) % outline.size()];
    const double from = across(a, axis);
    const double to = across(b, axis);
    if ((from > line) != (to > line)) {
      crossings.push_back(along(a, axis) +
                          ((line - from) / (to - from) * (along(b, axis) - along(a, axis))));
    }
  }
}

// The stretches of floor on a line: between its crossings of the outline and of the islands,
// taken in pairs.
std::vector<Stretch> stretches(const geom::Space& space, Axis axis, double line) {
  std::vector<double> crossings;
  add_crossings(space.outline, axis, line, crossings);
  for (const std::vector<geom::Vec2>& island : space.islands) {
    add_crossings(island, axis, line, crossings);
  }
  std::ranges::sort(crossings);
  std::vector<Stretch> found;
  for (std::size_t i = 0; i + 1 < crossings.size(); i += 2) {
    found.push_back({.from = crossings[i], .to = crossings[i + 1]});
  }
  return found;
}

// The stretch that holds `value`, or else the longest; the first of equals.
std::optional<Stretch> pick(std::span<const Stretch> found, std::optional<double> value) {
  std::optional<Stretch> best;
  for (const Stretch& stretch : found) {
    if (value && stretch.holds(*value)) {
      return stretch;
    }
    if (!best || stretch.length() > best->length()) {
      best = stretch;
    }
  }
  return best;
}

// The centre of area of an outline. Precondition: its area is not zero.
geom::Vec2 centre(std::span<const geom::Vec2> outline) {
  geom::Vec2 sum;
  double twice = 0.0;
  for (std::size_t i = 0; i < outline.size(); ++i) {
    const geom::Vec2 a = outline[i];
    const geom::Vec2 b = outline[(i + 1) % outline.size()];
    const double part = geom::cross(a, b);
    sum = sum + ((a + b) * part);
    twice += part;
  }
  return sum * (1.0 / (kCentreShare * twice));
}

}  // namespace

std::optional<Stretch> stretch_across(const geom::Space& space, double y, double x) {
  return pick(stretches(space, Axis::kX, y), x);
}

std::optional<Stretch> stretch_up(const geom::Space& space, double x, double y) {
  return pick(stretches(space, Axis::kY, x), y);
}

std::vector<RoomSpace> room_spaces(std::span<const geom::WallInput> walls,
                                   std::span<const NamedRoom> rooms) {
  std::vector<geom::Space> spaces = geom::find_spaces(walls);
  std::vector<geom::Anchor> anchors;
  anchors.reserve(rooms.size());
  for (const NamedRoom& named : rooms) {
    anchors.push_back({.room = named.id, .at = named.room.anchor});
  }
  const geom::Names names = geom::name_spaces(spaces, anchors);
  std::vector<RoomSpace> found;
  found.reserve(spaces.size());
  for (std::size_t i = 0; i < spaces.size(); ++i) {
    RoomSpace space{.space = std::move(spaces[i]), .room = {}};
    const std::optional<ElementId> name = names.rooms[i];
    const auto named = name ? std::ranges::find(rooms, *name, &NamedRoom::id) : rooms.end();
    if (named != rooms.end()) {
      space.room = named->id;
      space.kind = named->room.kind;
    }
    found.push_back(std::move(space));
  }
  // The outline's own area: a space with a room standing free in it is larger than that room.
  std::ranges::stable_sort(found, [](const RoomSpace& lhs, const RoomSpace& rhs) {
    return geom::signed_area(lhs.space.outline) > geom::signed_area(rhs.space.outline);
  });
  return found;
}

std::optional<RoomLabel> label_of(const geom::Space& space, TextSize text) {
  if (space.outline.empty() || geom::signed_area(space.outline) <= 0.0) {
    return std::nullopt;
  }
  const geom::Vec2 middle = centre(space.outline);
  const std::optional<Stretch> wide = pick(stretches(space, Axis::kX, middle.y), std::nullopt);
  if (!wide) {
    return std::nullopt;
  }
  const std::optional<Stretch> tall = pick(stretches(space, Axis::kY, wide->middle()), middle.y);
  if (!tall) {
    return std::nullopt;
  }
  const geom::Vec2 at{.x = wide->middle(), .y = tall->middle()};
  // The stretch across the plan may be another one at the height the point moved to.
  const Stretch level = pick(stretches(space, Axis::kX, at.y), at.x).value_or(*wide);
  const bool upright = level.length() < kNameDigits * text.digit && tall->length() > level.length();
  if (upright) {
    return RoomLabel{.at = at,
                     .along = {.x = 0.0, .y = 1.0},
                     .fit = tall->length() * kLabelFill,
                     .across = level.length(),
                     .wide = level,
                     .tall = *tall};
  }
  return RoomLabel{.at = at,
                   .along = {.x = 1.0, .y = 0.0},
                   .fit = level.length() * kLabelFill,
                   .across = tall->length(),
                   .wide = level,
                   .tall = *tall};
}

void draw_floors(std::span<const RoomSpace> spaces, Drawing& drawing) {
  for (const RoomSpace& room : spaces) {
    drawing.polygons.push_back({.element = room.room,
                                .fill = Fill::kFloor,
                                .stroke = Stroke::kNone,
                                .points = room.space.outline});
  }
}

void write_rooms(std::span<const RoomSpace> spaces, const RoomSpace* focused, TextSize text,
                 Drawing& drawing) {
  for (const RoomSpace& room : spaces) {
    const std::optional<RoomLabel> label = label_of(room.space, text);
    if (&room == focused || !label || label->across < text.height) {
      continue;
    }
    const bool named = !room.room.is_nil() && label->across >= 2 * text.height;
    const geom::Vec2 up = geom::perp(label->along) * (text.height / 2);
    if (named) {
      drawing.texts.push_back({.element = room.room,
                               .kind = TextKind::kRoomName,
                               .value = static_cast<std::int64_t>(room.kind),
                               .at = label->at + up,
                               .along = label->along,
                               .fit = label->fit});
    }
    drawing.texts.push_back({.element = room.room,
                             .kind = TextKind::kRoomArea,
                             .value = std::llround(room.space.area * kCm2PerM2),
                             .at = named ? label->at - up : label->at,
                             .along = label->along,
                             .fit = label->fit});
  }
}

}  // namespace otves::drafting
