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

#include "boxes.h"

namespace otves::drafting {
namespace {

// A name needs about this many digit widths; a space narrower than that and taller than it is
// wide is written up the plan.
constexpr double kShortName = 8.0;

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

// The label moved along its line off what the doors sweep. `named` says whether it is two
// lines, a name over an area, or the area alone.
RoomLabel off_swept(RoomLabel label, std::span<const Bounds> swept, bool named, TextSize text) {
  const Axis axis = label.along.y != 0.0 ? Axis::kY : Axis::kX;
  const Stretch whole = axis == Axis::kY ? label.tall : label.wide;
  const double line = across(label.at, axis);
  const double rise = (named ? 1.0 : 1.0 / 2) * text.height;
  std::vector<Stretch> taken;
  for (const Bounds& bounds : swept) {
    if (across(bounds.low, axis) < line + rise && across(bounds.high, axis) > line - rise) {
      taken.push_back({.from = along(bounds.low, axis), .to = along(bounds.high, axis)});
    }
  }
  const double width = (named ? kNameDigits : kAreaDigits) * text.digit;
  const double at = along(label.at, axis);
  const bool lies_on = std::ranges::any_of(taken, [&](const Stretch& part) {
    return part.from < at + (width / 2) && part.to > at - (width / 2);
  });
  if (!lies_on) {
    return label;
  }
  // The longest part of the stretch between what is taken; the first of equals.
  std::ranges::sort(taken, {}, &Stretch::from);
  taken.push_back({.from = whole.to, .to = whole.to});
  Stretch clear{.from = whole.from, .to = whole.from};
  double reached = whole.from;
  for (const Stretch& part : taken) {
    const double until = std::min(part.from, whole.to);
    if (until - reached > clear.length()) {
      clear = {.from = reached, .to = until};
    }
    reached = std::max(reached, part.to);
  }
  if (clear.length() * kLabelFill < width) {
    return label;
  }
  (axis == Axis::kY ? label.at.y : label.at.x) = clear.middle();
  label.fit = std::min(label.fit, clear.length());
  return label;
}

// How far a label steps across its line to look for a place clear of other texts, in text
// heights.
constexpr double kLabelStep = 0.5;

// The label moved across its line off the texts already written: the nearest place where its
// lines lie on the floor and meet none of them. Where there is none it stays.
RoomLabel off_texts(const RoomLabel& label, const geom::Space& space, bool named,
                    std::span<const Text> written, TextSize text) {
  const Axis axis = label.along.y != 0.0 ? Axis::kY : Axis::kX;
  const Stretch room = axis == Axis::kY ? label.wide : label.tall;
  const double rise = (named ? 1.0 : 1.0 / 2) * text.height;
  const double width = std::min(label.fit, (named ? kNameDigits : kAreaDigits) * text.digit);
  const double at = along(label.at, axis);
  const auto moved = [&label, axis](double by) {
    RoomLabel there = label;
    (axis == Axis::kY ? there.at.x : there.at.y) += by;
    return there;
  };
  const auto is_free = [&](const RoomLabel& there) {
    const double level = across(there.at, axis);
    if (level - rise < room.from || level + rise > room.to) {
      return false;
    }
    // The floor is as wide there as the label.
    const std::optional<Stretch> floor = pick(stretches(space, axis, level), at);
    if (!floor || floor->from > at - (width / 2) || floor->to < at + (width / 2)) {
      return false;
    }
    const Box box{
        .at = there.at, .along = there.along, .half_along = width / 2, .half_across = rise};
    return std::ranges::none_of(written,
                                [&](const Text& other) { return meet(box, box_of(other, text)); });
  };
  const double step = kLabelStep * text.height;
  if (step <= 0.0) {
    return label;
  }
  const auto steps = static_cast<int>(room.length() / step);
  for (int away = 0; away <= steps; ++away) {
    const double by = away * step;
    for (const double way : {by, -by}) {
      if (const RoomLabel there = moved(way); is_free(there)) {
        return there;
      }
    }
  }
  return label;
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
  const bool upright = level.length() < kShortName * text.digit && tall->length() > level.length();
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

std::vector<ListedRoom> list_rooms(std::span<const RoomSpace> spaces, TextSize text) {
  struct Placed {
    geom::Vec2 at;
    ListedRoom room;
  };
  std::vector<Placed> placed;
  for (const RoomSpace& room : spaces) {
    if (const std::optional<RoomLabel> label = label_of(room.space, text)) {
      placed.push_back({.at = label->at,
                        .room = {.room = room.room, .kind = room.kind, .area = room.space.area}});
    }
  }
  std::ranges::stable_sort(placed, [](const Placed& lhs, const Placed& rhs) {
    return lhs.at.y != rhs.at.y ? lhs.at.y > rhs.at.y : lhs.at.x < rhs.at.x;
  });
  std::vector<ListedRoom> listed;
  listed.reserve(placed.size());
  for (const Placed& entry : placed) {
    listed.push_back(entry.room);
  }
  return listed;
}

void draw_floors(std::span<const RoomSpace> spaces, Drawing& drawing) {
  for (const RoomSpace& room : spaces) {
    drawing.polygons.push_back({.element = room.room,
                                .fill = Fill::kFloor,
                                .stroke = Stroke::kNone,
                                .points = room.space.outline});
  }
}

void write_rooms(std::span<const RoomSpace> spaces, const RoomSpace* focused, Avoid avoid,
                 TextSize text, Drawing& drawing) {
  // Labels keep off what was written before them, not off each other: each has its own floor.
  const std::size_t before = drawing.texts.size();
  for (const RoomSpace& room : spaces) {
    const std::optional<RoomLabel> found = label_of(room.space, text);
    if (&room == focused || !found || found->across < text.height) {
      continue;
    }
    const bool named = !room.room.is_nil() && found->across >= 2 * text.height;
    std::optional<RoomLabel> label = off_swept(*found, avoid.swept, named, text);
    if (avoid.texts) {
      label = off_texts(*label, room.space, named,
                        std::span<const Text>{drawing.texts}.first(before), text);
    }
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
