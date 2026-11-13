#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/geom/openings.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

// A plan as a display list (docs/adr/0013): what to draw and where, in plan metres. A painter maps
// each fill and stroke to a paint and writes the numbers. It positions nothing.
namespace otves::drafting {

enum class Fill : std::uint8_t {
  kNone,
  kWall,      // masonry
  kUnchosen,  // a wall with no material yet
  kGap,       // what an opening leaves of its wall
};

enum class Stroke : std::uint8_t {
  kNone,
  kWallEdge,
  kBelow,  // a wall of the level below
  kFrame,  // a window's faces and glazing
  kLeaf,   // a door's leaf and swing
};

enum class TextKind : std::uint8_t {
  kLength,       // a wall's length
  kAboutLength,  // one that nobody has typed or measured
};

// Every primitive names the element it draws, so a painter can mark the selected one; nil for
// what cannot be picked.
struct Polygon {
  ElementId element;
  Fill fill = Fill::kNone;
  Stroke stroke = Stroke::kNone;  // of its outline
  std::vector<geom::Vec2> points;
};

struct Line {
  ElementId element;
  Stroke stroke = Stroke::kNone;
  geom::Vec2 from;
  geom::Vec2 to;
};

// Around `centre` from `from` to `to`, counter-clockwise. Both lie at one radius.
struct Arc {
  ElementId element;
  Stroke stroke = Stroke::kNone;
  geom::Vec2 centre;
  geom::Vec2 from;
  geom::Vec2 to;
};

// A number written with its middle at `at`, along the unit direction `along`, which never points
// to the left or straight down: text reads from the left, and on an upright wall from the bottom.
struct Text {
  ElementId element;
  TextKind kind = TextKind::kLength;
  Mm value;
  geom::Vec2 at;
  geom::Vec2 along;
};

// Painted in this order, each list in the order given.
struct Drawing {
  std::vector<Polygon> polygons;
  std::vector<Line> lines;
  std::vector<Arc> arcs;
  std::vector<Text> texts;
};

// How big the painter's label text is on the plan at its zoom, in metres: the height of a line
// and the width of a digit.
struct TextSize {
  double height = 0.0;
  double digit = 0.0;
};

// One level to draw: its walls, the openings in them, and the walls of the level under it.
struct Plan {
  std::span<const geom::WallInput> walls;
  std::span<const geom::PlacedOpening> openings;
  std::span<const geom::WallInput> below;
};

// The level as a drawing.
//  - The walls of the level below, in outline.
//  - Each wall's joined outline, filled as masonry or as a wall with no material yet.
//  - Each opening's gap through its wall with its jambs; a window's faces and glazing line; a
//    door's leaf standing open at right angles, hinged at its near end on the wall's left face,
//    with its swing.
//  - Each wall's length beside it and along it, by its left face at the middle; where another
//    wall or another length lies there, by the other face or nearer an end. A length longer than
//    four fifths of its wall is left out.
// Precondition: every wall passes validate() and ids are unique. The result does not depend on
// the order of the inputs.
[[nodiscard]] Drawing draft(const Plan& plan, TextSize text);

}  // namespace otves::drafting
