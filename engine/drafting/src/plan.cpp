#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <span>
#include <string>
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

#include "rooms.h"
#include "sizes.h"

namespace otves::drafting {
namespace {

// A length takes at most this share of its wall.
constexpr double kLabelFill = 0.8;

// The clear space between a wall's face and its length, in text heights.
constexpr double kLabelGap = 0.25;

// «≈» and the space after it are about as wide as two digits.
constexpr int kAboutDigits = 2;

// A room's name is taken to be at most this many digits wide, and its area this many: the
// engine does not know the words.
constexpr double kNameDigits = 16.0;
constexpr double kAreaDigits = 7.0;

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
        // The leaf is as long as the opening is wide; from the far jamb round to the leaf's tip
        // is a quarter turn to the left of the wall's direction.
        const geom::Vec2 open = geom::perp(far_left - near_left);
        const geom::Vec2 tip = near_left + open;
        drawing.lines.push_back(
            {.element = id, .stroke = Stroke::kLeaf, .from = near_left, .to = tip});
        drawing.arcs.push_back({.element = id,
                                .stroke = Stroke::kLeaf,
                                .centre = near_left,
                                .from = far_left,
                                .to = tip});
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

// How wide a wall's length is written.
double label_width(Mm length, bool exact, TextSize text) {
  const auto digits = static_cast<int>(std::to_string(length.value()).size());
  return (digits + (exact ? 0 : kAboutDigits)) * text.digit;
}

// How wide a written text is along its direction, at most.
double width_of(const Text& written, TextSize text) {
  switch (written.kind) {
    case TextKind::kRoomName:
      return std::min(written.fit, kNameDigits * text.digit);
    case TextKind::kRoomArea:
      return kAreaDigits * text.digit;
    case TextKind::kAboutLength:
      return label_width(Mm{written.value}, false, text);
    case TextKind::kLength:
    case TextKind::kOverall:
    case TextKind::kClear:
    case TextKind::kPier:
    case TextKind::kOpeningWidth:
      break;
  }
  return label_width(Mm{written.value}, true, text);
}

// A text as the box it fills: its middle, its direction and half its size along and across.
struct Box {
  geom::Vec2 at;
  geom::Vec2 along;
  double half_along = 0.0;
  double half_across = 0.0;
};

// Whether the boxes lie apart along `axis`, a unit direction.
bool apart_along(geom::Vec2 axis, const Box& a, const Box& b) {
  const auto reach = [axis](const Box& box) {
    return (box.half_along * std::abs(geom::dot(box.along, axis))) +
           (box.half_across * std::abs(geom::dot(geom::perp(box.along), axis)));
  };
  return std::abs(geom::dot(b.at - a.at, axis)) > reach(a) + reach(b);
}

// Two boxes meet unless a line along a side of one of them parts them.
bool meet(const Box& a, const Box& b) {
  return !apart_along(a.along, a, b) && !apart_along(geom::perp(a.along), a, b) &&
         !apart_along(b.along, a, b) && !apart_along(geom::perp(b.along), a, b);
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

// Writes each wall's length where it is clear of the other walls and of the lengths already
// written. `round_spaces` are the walls round spaces, sorted.
void write_lengths(std::span<const geom::WallInput> walls,
                   std::span<const geom::WallFootprint> footprints,
                   std::span<const ElementId> round_spaces, TextSize text, Drawing& drawing) {
  for (const geom::WallFootprint& footprint : footprints) {
    const Wall* wall = find(walls, footprint.id);
    // A wall round a space is sized by the space, once somebody has given its length.
    if (wall == nullptr ||
        (wall->exact && std::ranges::binary_search(round_spaces, footprint.id))) {
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
      // The length keeps a quarter of a text height clear of every text written before it.
      const Box box{.at = at,
                    .along = dir,
                    .half_along = width / 2,
                    .half_across = (text.height / 2) + (kLabelGap * text.height)};
      const bool on_a_text = std::ranges::any_of(drawing.texts, [&](const Text& written) {
        return meet(box, {.at = written.at,
                          .along = written.along,
                          .half_along = width_of(written, text) / 2,
                          .half_across = text.height / 2});
      });
      return !on_a_wall && !on_a_text;
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
  // Rooms and sizes are written first, so a wall's length keeps clear of them.
  write_rooms(spaces, focused != spaces.end() ? &*focused : nullptr, size, drawing);
  if (focused != spaces.end()) {
    write_sizes(*focused, walls, plan.openings, size, drawing);
  }
  if (!spaces.empty()) {
    write_overall(joins.footprints, size, drawing);
  }
  write_lengths(walls, joins.footprints, walls_round(spaces), size, drawing);
  return drawing;
}

}  // namespace otves::drafting
