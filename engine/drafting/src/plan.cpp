#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/drafting/plan.h>
#include <otves/geom/faces.h>
#include <otves/geom/openings.h>
#include <otves/geom/polygon.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

namespace otves::drafting {
namespace {

// A length takes at most this share of its wall.
constexpr double kLabelFill = 0.8;

// The clear space between a wall's face and its length, in text heights.
constexpr double kLabelGap = 0.25;

// «≈» and the space after it are about as wide as two digits.
constexpr int kAboutDigits = 2;

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

// How wide a wall's length is written.
double label_width(Mm length, bool exact, TextSize text) {
  const auto digits = static_cast<int>(std::to_string(length.value()).size());
  return (digits + (exact ? 0 : kAboutDigits)) * text.digit;
}

// Writes each wall's length where it is clear of the other walls and of the lengths already
// written.
void write_lengths(std::span<const geom::WallInput> walls,
                   std::span<const geom::WallFootprint> footprints, TextSize text,
                   Drawing& drawing) {
  for (const geom::WallFootprint& footprint : footprints) {
    const Wall* wall = find(walls, footprint.id);
    if (wall == nullptr) {
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
      const bool on_a_length = std::ranges::any_of(drawing.texts, [&](const Text& written) {
        return geom::length(written.at - at) < width;
      });
      return !on_a_wall && !on_a_length;
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
                             .value = value,
                             .at = place != places.end() ? *place : places.front(),
                             .along = turned ? -dir : dir});
  }
}

}  // namespace

Drawing draft(const Plan& plan, TextSize text) {
  Drawing drawing;
  outline_below(plan.below, drawing);
  const std::vector<geom::WallInput> walls = sorted(plan.walls);
  const geom::WallJoins joins = geom::join_walls(walls);
  for (const geom::WallFootprint& footprint : joins.footprints) {
    const Wall* wall = find(walls, footprint.id);
    const bool chosen = wall != nullptr && wall->has_unit();
    drawing.polygons.push_back({.element = footprint.id,
                                .fill = chosen ? Fill::kWall : Fill::kUnchosen,
                                .stroke = Stroke::kWallEdge,
                                .points = footprint.outline});
  }
  draw_openings(plan.openings, drawing);
  write_lengths(walls, joins.footprints,
                {.height = std::max(text.height, 0.0), .digit = std::max(text.digit, 0.0)},
                drawing);
  return drawing;
}

}  // namespace otves::drafting
