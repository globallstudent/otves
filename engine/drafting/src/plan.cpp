#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include <otves/core/build.h>
#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/core/wall_type.h>
#include <otves/drafting/plan.h>
#include <otves/geom/faces.h>
#include <otves/geom/layers.h>
#include <otves/geom/openings.h>
#include <otves/geom/polygon.h>
#include <otves/geom/spaces.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

#include "boxes.h"
#include "rooms.h"
#include "sizes.h"

namespace otves::drafting {
namespace {

// A length takes at most this share of its wall.
constexpr double kLabelFill = 0.8;

// The clear space between a wall's face and its length, in text heights.
constexpr double kLabelGap = 0.25;

// How far from its wall's face an opening's mark may stand, in text heights to its middle: beside
// the face, past a chain whose numbers are on the wall's side of it, or past its numbers.
constexpr std::array kMarkGaps{1.0, 1.9, 3.0};

// Half the height of a mark's letters, in text heights: less than half a line, so a mark may
// stand between a wall and the number of a clear size beside it.
constexpr double kMarkRise = 0.4;

// A direction this close to upright counts as upright.
constexpr double kUpright = 1e-9;

// Where along a wall its length may be written, the middle first.
constexpr std::array kLabelPlaces{0.5, 0.3, 0.7};

std::vector<geom::WallInput> sorted(std::span<const geom::WallInput> walls) {
  std::vector<geom::WallInput> out(walls.begin(), walls.end());
  std::ranges::sort(out, {}, &geom::WallInput::id);
  return out;
}

const Wall* find(std::span<const geom::WallInput> walls, ElementId id) {
  const auto found = std::ranges::lower_bound(walls, id, {}, &geom::WallInput::id);
  return found != walls.end() && found->id == id ? &found->wall : nullptr;
}

constexpr geom::Vec2 middle(geom::Vec2 a, geom::Vec2 b) { return (a + b) * (1.0 / 2); }

void outline_below(std::span<const geom::WallInput> below, Drawing& drawing) {
  for (geom::WallFootprint& footprint : geom::join_walls(below).footprints) {
    drawing.polygons.push_back({.element = {},
                                .fill = Fill::kNone,
                                .stroke = Stroke::kBelow,
                                .points = std::move(footprint.outline)});
  }
}

// A door's leaf standing open at right angles: the corner of the opening it hangs on, the
// corner across the opening on the same face, and where its free edge stands.
struct Leaf {
  geom::Vec2 hinge;
  geom::Vec2 jamb;
  geom::Vec2 tip;
};

// The corners are those of the opening's outline: near and far end on the wall's right face,
// then far and near on its left. The leaf is as long as the opening is wide and stands out of
// the wall on the side it opens to.
Leaf leaf_of(const Opening& opening, const std::array<geom::Vec2, 4>& corners) {
  const auto [near_right, far_right, far_left, near_left] = corners;
  const bool left = opening.swing == Swing::kLeft;
  const bool near = opening.hinge == Hinge::kNear;
  const geom::Vec2 near_corner = left ? near_left : near_right;
  const geom::Vec2 far_corner = left ? far_left : far_right;
  const geom::Vec2 hinge = near ? near_corner : far_corner;
  const geom::Vec2 jamb = near ? far_corner : near_corner;
  // A quarter turn of the opening's width: to the left of the wall's direction, or its right.
  const geom::Vec2 out = geom::perp(far_corner - near_corner) * (left ? 1.0 : -1.0);
  return {.hinge = hinge, .jamb = jamb, .tip = hinge + out};
}

// The gap and the plan symbol of each opening, in the order of their ids. Corners come
// counter-clockwise: near and far end on the wall's right face, then far and near on its left.
void draw_openings(std::span<const geom::PlacedOpening> openings, Drawing& drawing) {
  std::vector<geom::PlacedOpening> ordered(openings.begin(), openings.end());
  std::ranges::sort(ordered, {}, &geom::PlacedOpening::id);
  for (const geom::PlacedOpening& placed : ordered) {
    const auto [near_right, far_right, far_left, near_left] =
        geom::opening_outline(placed.wall, placed.opening);
    const ElementId id = placed.id;
    drawing.polygons.push_back({.element = id,
                                .fill = Fill::kGap,
                                .stroke = Stroke::kNone,
                                .points = {near_right, far_right, far_left, near_left}});
    drawing.lines.push_back(
        {.element = id, .stroke = Stroke::kWallEdge, .from = near_right, .to = near_left});
    drawing.lines.push_back(
        {.element = id, .stroke = Stroke::kWallEdge, .from = far_right, .to = far_left});
    switch (placed.opening.kind) {
      case OpeningKind::kWindow:
        drawing.lines.push_back(
            {.element = id, .stroke = Stroke::kFrame, .from = near_right, .to = far_right});
        drawing.lines.push_back(
            {.element = id, .stroke = Stroke::kFrame, .from = near_left, .to = far_left});
        drawing.lines.push_back({.element = id,
                                 .stroke = Stroke::kFrame,
                                 .from = middle(near_right, near_left),
                                 .to = middle(far_right, far_left)});
        break;
      case OpeningKind::kDoor: {
        const Leaf leaf = leaf_of(placed.opening, {near_right, far_right, far_left, near_left});
        drawing.lines.push_back(
            {.element = id, .stroke = Stroke::kLeaf, .from = leaf.hinge, .to = leaf.tip});
        // An arc runs counter-clockwise: from the jamb round to the tip, or the other way.
        const bool turns_left = geom::cross(leaf.jamb - leaf.hinge, leaf.tip - leaf.hinge) > 0.0;
        drawing.arcs.push_back({.element = id,
                                .stroke = Stroke::kLeaf,
                                .centre = leaf.hinge,
                                .from = turns_left ? leaf.jamb : leaf.tip,
                                .to = turns_left ? leaf.tip : leaf.jamb});
        break;
      }
      case OpeningKind::kEmpty:
        break;
    }
  }
}

Fill fill_of(const BuiltLayer& layer) {
  switch (layer.function) {
    case LayerFunction::kStructure:
      return layer.has_unit() ? Fill::kWall : Fill::kUnchosen;
    case LayerFunction::kFacing:
      return layer.has_unit() ? Fill::kFacing : Fill::kUnchosen;
    case LayerFunction::kInsulation:
      return Fill::kInsulation;
    case LayerFunction::kGap:
      break;
  }
  return Fill::kNone;
}

// What a wall of at most one layer is filled as.
Fill fill_of(std::span<const BuiltLayer> layers) {
  return layers.empty() ? Fill::kUnchosen : fill_of(layers.front());
}

// A wall of several layers: each layer's strip of the joined outline, then the outline itself
// over them, so the wall's edge is drawn once and whole.
void draw_layers(const Wall& wall, const geom::WallFootprint& footprint,
                 std::span<const BuiltLayer> layers, Drawing& drawing) {
  std::vector<Mm> thicknesses;
  thicknesses.reserve(layers.size());
  for (const BuiltLayer& layer : layers) {
    thicknesses.push_back(layer.thickness);
  }
  const std::vector<geom::Strip> strips = geom::layer_strips(wall, thicknesses);
  for (std::size_t index = 0; index < layers.size(); ++index) {
    std::vector<geom::Vec2> strip = geom::clip(wall, footprint.outline, strips[index]);
    if (!strip.empty()) {
      drawing.polygons.push_back({.element = footprint.id,
                                  .fill = fill_of(layers[index]),
                                  .stroke = Stroke::kLayer,
                                  .points = std::move(strip)});
    }
  }
  drawing.polygons.push_back({.element = footprint.id,
                              .fill = Fill::kNone,
                              .stroke = Stroke::kWallEdge,
                              .points = footprint.outline});
}

// What each door's leaf sweeps as it opens: the square on the opening that its swing lies in.
std::vector<Bounds> swept_by(std::span<const geom::PlacedOpening> openings) {
  std::vector<Bounds> swept;
  for (const geom::PlacedOpening& placed : openings) {
    if (placed.opening.kind != OpeningKind::kDoor) {
      continue;
    }
    const Leaf leaf = leaf_of(placed.opening, geom::opening_outline(placed.wall, placed.opening));
    const std::array corners{leaf.hinge, leaf.jamb, leaf.tip, leaf.tip + (leaf.jamb - leaf.hinge)};
    Bounds bounds{.low = leaf.hinge, .high = leaf.hinge};
    for (const geom::Vec2 corner : corners) {
      bounds.low = {.x = std::min(bounds.low.x, corner.x), .y = std::min(bounds.low.y, corner.y)};
      bounds.high = {.x = std::max(bounds.high.x, corner.x),
                     .y = std::max(bounds.high.y, corner.y)};
    }
    swept.push_back(bounds);
  }
  return swept;
}

TextKind mark_kind(OpeningKind kind) {
  switch (kind) {
    case OpeningKind::kWindow:
      return TextKind::kWindowMark;
    case OpeningKind::kDoor:
      return TextKind::kDoorMark;
    case OpeningKind::kEmpty:
      break;
  }
  return TextKind::kEmptyMark;
}

// What a mark keeps off.
enum class Clear : std::uint8_t {
  kSizes,   // the texts written before it and the dimension lines
  kLeaves,  // and the leaves of doors with their swings
};

// Whether a mark may be written in the box.
bool is_clear(const Box& box, const Drawing& drawing, TextSize text, Clear of) {
  const bool on_a_text = std::ranges::any_of(
      drawing.texts, [&](const Text& written) { return meet(box, box_of(written, text)); });
  const bool on_a_line = std::ranges::any_of(drawing.lines, [&box, of](const Line& line) {
    const bool kept_off = line.stroke == Stroke::kDimension || line.stroke == Stroke::kChain ||
                          (of == Clear::kLeaves && line.stroke == Stroke::kLeaf);
    return kept_off && crosses(box, line.from, line.to);
  });
  const bool on_a_swing =
      of == Clear::kLeaves &&
      std::ranges::any_of(drawing.arcs, [&box](const Arc& arc) { return crosses(box, arc); });
  return !on_a_text && !on_a_line && !on_a_swing;
}

// Each opening's mark, or the mark of `only` when one is named, beside the opening's middle and
// along its wall: by the face its leaf does not open to, or else by the other one, the nearest
// place that is clear of the sizes written there and of the doors' leaves; where no place is,
// the nearest that is clear of the sizes.
void write_marks(std::span<const geom::PlacedOpening> openings, std::span<const Mark> marks,
                 std::optional<ElementId> only, TextSize text, Drawing& drawing) {
  std::vector<geom::PlacedOpening> ordered(openings.begin(), openings.end());
  std::ranges::sort(ordered, {}, &geom::PlacedOpening::id);
  for (const geom::PlacedOpening& placed : ordered) {
    const auto mark = std::ranges::lower_bound(marks, placed.id, {}, &Mark::opening);
    if (mark == marks.end() || mark->opening != placed.id || (only && *only != placed.id)) {
      continue;
    }
    const auto [near_right, far_right, far_left, near_left] =
        geom::opening_outline(placed.wall, placed.opening);
    const bool left = placed.opening.swing == Swing::kLeft;
    const geom::Vec2 closed = middle(left ? near_right : near_left, left ? far_right : far_left);
    const geom::Vec2 open = middle(left ? near_left : near_right, left ? far_left : far_right);
    const geom::Vec2 through = left ? near_right - near_left : near_left - near_right;
    const geom::Vec2 out = through * (text.height / geom::length(through));
    const geom::Vec2 run = far_right - near_right;
    const geom::Vec2 dir = run * (1.0 / geom::length(run));
    // Upright, as a length is: never towards the left, and on an upright wall from the bottom up.
    const bool turned = dir.x < -kUpright || (std::abs(dir.x) <= kUpright && dir.y < 0.0);
    const geom::Vec2 along = turned ? -dir : dir;
    std::vector<geom::Vec2> places;
    for (const double gap : kMarkGaps) {
      places.push_back(closed + (out * gap));
      places.push_back(open - (out * gap));
    }
    const auto clear_place = [&](Clear of) {
      return std::ranges::find_if(places, [&](geom::Vec2 at) {
        return is_clear({.at = at,
                         .along = along,
                         .half_along = kMarkDigits * text.digit / 2,
                         .half_across = kMarkRise * text.height},
                        drawing, text, of);
      });
    };
    auto place = clear_place(Clear::kLeaves);
    if (place == places.end()) {
      place = clear_place(Clear::kSizes);
    }
    drawing.texts.push_back({.element = placed.id,
                             .kind = mark_kind(placed.opening.kind),
                             .value = mark->number,
                             .at = place != places.end() ? *place : places.front(),
                             .along = along});
  }
}

// The walls that some space's outline runs along, sorted.
std::vector<ElementId> walls_round(std::span<const RoomSpace> spaces) {
  std::vector<ElementId> round;
  for (const RoomSpace& room : spaces) {
    round.insert(round.end(), room.space.sides.begin(), room.space.sides.end());
  }
  std::ranges::sort(round);
  const auto [first, last] = std::ranges::unique(round);
  round.erase(first, last);
  return round;
}

// Whose lengths are written.
enum class Lengths : std::uint8_t {
  // Of every wall but one round a space whose length somebody has given: its space sizes it.
  kUntilGiven,
  // Of no wall round a space: every space is drawn with its sizes.
  kOffSpaces,
};

// The walls that get a length, by the list of those round spaces, sorted.
struct Lengthed {
  std::span<const ElementId> round_spaces;
  Lengths which = Lengths::kUntilGiven;

  [[nodiscard]] bool has(ElementId id, const Wall& wall) const {
    const bool sized = which == Lengths::kOffSpaces || wall.exact;
    return !(sized && std::ranges::binary_search(round_spaces, id));
  }
};

// Writes each wall's length where it is clear of the other walls, of the texts already written
// and of the dimension lines.
void write_lengths(std::span<const geom::WallInput> walls,
                   std::span<const geom::WallFootprint> footprints, Lengthed lengthed,
                   TextSize text, Drawing& drawing) {
  for (const geom::WallFootprint& footprint : footprints) {
    const Wall* wall = find(walls, footprint.id);
    if (wall == nullptr || !lengthed.has(footprint.id, *wall)) {
      continue;
    }
    const geom::Vec2 start = geom::to_vec2(wall->start);
    const geom::Vec2 run = geom::to_vec2(wall->end) - start;
    const double length = geom::length(run);
    const Mm value = round_to_mm(length);
    const double width = label_width(value, wall->exact, text);
    if (width > length * kLabelFill) {
      continue;
    }
    const geom::Vec2 dir = run * (1.0 / length);
    const geom::Vec2 left = geom::perp(dir);
    const geom::Faces faces = geom::faces(*wall);
    const double clearance = (kLabelGap * text.height) + (text.height / 2);
    const geom::Vec2 half = dir * (width / 2);
    const auto is_free = [&](geom::Vec2 at) {
      const bool on_a_wall = std::ranges::any_of(footprints, [&](const geom::WallFootprint& other) {
        return other.id != footprint.id &&
               (geom::contains(other.outline, at) || geom::contains(other.outline, at - half) ||
                geom::contains(other.outline, at + half));
      });
      // The length keeps a quarter of a text height clear of every text written before it,
      // and off the dimension lines: by a chain it would read as one of the chain's sizes.
      const Box box{.at = at,
                    .along = dir,
                    .half_along = width / 2,
                    .half_across = (text.height / 2) + (kLabelGap * text.height)};
      return !on_a_wall && is_clear(box, drawing, text, Clear::kSizes);
    };
    std::vector<geom::Vec2> places;
    for (const double part : kLabelPlaces) {
      const geom::Vec2 on_line = start + (run * part);
      places.push_back(on_line + (left * (faces.left + clearance)));
      places.push_back(on_line - (left * (faces.right + clearance)));
    }
    const auto place = std::ranges::find_if(places, is_free);
    // Upright: never towards the left, and on an upright wall from the bottom up.
    const bool turned = dir.x < 0.0 || (dir.x == 0.0 && dir.y < 0.0);
    drawing.texts.push_back({.element = footprint.id,
                             .kind = wall->exact ? TextKind::kLength : TextKind::kAboutLength,
                             .value = value.value(),
                             .at = place != places.end() ? *place : places.front(),
                             .along = turned ? -dir : dir});
  }
}

}  // namespace

std::vector<Mark> mark_openings(std::span<const Marked> openings) {
  std::vector<Marked> oldest_first(openings.begin(), openings.end());
  std::ranges::sort(oldest_first, {}, &Marked::id);
  // One of each kind, width and height, in the order of the oldest opening of that size.
  struct Size {
    OpeningKind kind = OpeningKind::kWindow;
    Mm width;
    Mm height;
    std::int64_t number = 0;
  };
  std::vector<Size> sizes;
  std::vector<Mark> marks;
  marks.reserve(oldest_first.size());
  for (const Marked& marked : oldest_first) {
    const Opening& opening = marked.opening;
    auto size = std::ranges::find_if(sizes, [&opening](const Size& known) {
      return known.kind == opening.kind && known.width == opening.width &&
             known.height == opening.height;
    });
    if (size == sizes.end()) {
      const auto of_kind = std::ranges::count(sizes, opening.kind, &Size::kind);
      sizes.push_back({.kind = opening.kind,
                       .width = opening.width,
                       .height = opening.height,
                       .number = of_kind + 1});
      size = std::prev(sizes.end());
    }
    marks.push_back({.opening = marked.id, .number = size->number});
  }
  return marks;
}

std::vector<ListedOpening> list_openings(std::span<const Marked> openings) {
  const std::vector<Mark> marks = mark_openings(openings);
  std::vector<ListedOpening> listed;
  for (const Marked& marked : openings) {
    const auto mark = std::ranges::lower_bound(marks, marked.id, {}, &Mark::opening);
    const TextKind kind = mark_kind(marked.opening.kind);
    const auto line = std::ranges::find_if(listed, [&](const ListedOpening& known) {
      return known.kind == kind && known.number == mark->number;
    });
    if (line != listed.end()) {
      ++line->count;
    } else {
      listed.push_back({.kind = kind,
                        .number = mark->number,
                        .width = marked.opening.width,
                        .height = marked.opening.height,
                        .count = 1});
    }
  }
  std::ranges::sort(listed, [](const ListedOpening& lhs, const ListedOpening& rhs) {
    return std::tie(lhs.kind, lhs.number) < std::tie(rhs.kind, rhs.number);
  });
  return listed;
}

Drawing draft(const Plan& plan, TextSize text) {
  Drawing drawing;
  outline_below(plan.below, drawing);
  const std::vector<geom::WallInput> walls = sorted(plan.walls);
  const std::vector<RoomSpace> spaces = room_spaces(walls, plan.rooms);
  draw_floors(spaces, drawing);
  const geom::WallJoins joins = geom::join_walls(walls);
  for (const geom::WallFootprint& footprint : joins.footprints) {
    const Wall* wall = find(walls, footprint.id);
    if (wall == nullptr) {
      continue;
    }
    const auto build = std::ranges::find(plan.builds, footprint.id, &WallBuild::wall);
    if (build == plan.builds.end() || build->layers.size() < 2) {
      const bool chosen =
          build == plan.builds.end() ? wall->has_unit() : fill_of(build->layers) != Fill::kUnchosen;
      drawing.polygons.push_back({.element = footprint.id,
                                  .fill = chosen ? Fill::kWall : Fill::kUnchosen,
                                  .stroke = Stroke::kWallEdge,
                                  .points = footprint.outline});
      continue;
    }
    draw_layers(*wall, footprint, build->layers, drawing);
  }
  draw_openings(plan.openings, drawing);
  const TextSize size{.height = std::max(text.height, 0.0), .digit = std::max(text.digit, 0.0)};
  const auto focused =
      plan.focus ? std::ranges::find_if(spaces,
                                        [at = geom::to_vec2(*plan.focus)](const RoomSpace& room) {
                                          return geom::holds(room.space, at);
                                        })
                 : spaces.end();
  // Rooms and sizes are written first, so a wall's length keeps clear of them. A sheet has the
  // sizes of every space, and each room's name where they leave room for it; a screen has the
  // sizes of the space in focus in place of its name.
  const std::vector<Bounds> swept = swept_by(plan.openings);
  if (plan.sheet) {
    write_sheet_sizes(spaces, walls, plan.openings, size, drawing);
    write_rooms(spaces, nullptr, {.swept = swept, .texts = true}, size, drawing);
  } else {
    write_rooms(spaces, focused == spaces.end() ? nullptr : &*focused,
                {.swept = swept, .texts = false}, size, drawing);
    if (focused != spaces.end()) {
      write_sizes(*focused, walls, plan.openings, Stroke::kChain, size, drawing);
    } else if (!plan.selected.is_nil()) {
      write_opening_sizes(spaces, walls, plan.openings, plan.selected, size, drawing);
    }
  }
  // A sheet has every opening's mark; a screen shows the selected opening's alone.
  write_marks(plan.openings, mark_openings(plan.marked),
              plan.sheet ? std::nullopt : std::optional{plan.selected}, size, drawing);
  if (!spaces.empty()) {
    write_overall(joins.footprints, size, drawing);
  }
  drawing.rooms = list_rooms(spaces, size);
  drawing.openings = list_openings(plan.marked);
  const std::vector<ElementId> round = walls_round(spaces);
  write_lengths(
      walls, joins.footprints,
      {.round_spaces = round, .which = plan.sheet ? Lengths::kOffSpaces : Lengths::kUntilGiven},
      size, drawing);
  return drawing;
}

}  // namespace otves::drafting
