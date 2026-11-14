#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <otves/core/build.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/room.h>
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
  kWall,        // masonry that carries the wall
  kUnchosen,    // masonry with no material yet
  kGap,         // what an opening leaves of its wall
  kFacing,      // masonry laid in front of the wall
  kInsulation,  // a layer of insulation
  kFloor,       // a space the walls close
};

enum class Stroke : std::uint8_t {
  kNone,
  kWallEdge,
  kBelow,      // a wall of the level below
  kFrame,      // a window's faces and glazing
  kLeaf,       // a door's leaf and swing
  kLayer,      // where two layers of a wall meet
  kDimension,  // a dimension line, its ticks and the lines that lead to it
  kChain,      // the same for the sizes of the space in focus
};

enum class TextKind : std::uint8_t {
  kLength,       // a wall's length
  kAboutLength,  // one that nobody has typed or measured
  kRoomName,     // a room's name: the value is its RoomKind
  kRoomArea,     // a space's floor: the value is its area in cm²
  // Sizes in millimetres (docs/adr/0016). All but the first are handles: a typed value changes
  // the model.
  kOverall,       // the plan from outer face to outer face
  kClear,         // across the space in focus, between two of its walls
  kPier,          // a piece of wall between a corner or an opening and the next opening
  kOpeningWidth,  // an opening's width
  // An opening's mark: the value is its number among the marks of its kind, which the text's
  // kind names so the painter can write its letters.
  kWindowMark,
  kDoorMark,
  kEmptyMark,
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
// The value is millimetres unless the kind says otherwise. `fit` is the room there is for the
// text along its direction, in metres: the painter leaves out a text wider than that. Zero for a
// text that was fitted here.
// A size that is a handle says what a typed value changes (docs/adr/0016): `element` is the
// opening whose width it is, or the opening a piece of wall leads to. For a piece of wall,
// `from` is where its fixed end lies along the opening's wall from that wall's start, and
// `forward` whether the opening lies further along the wall than that end. For a clear size,
// `element` is the wall at its far end, to the right or the top of the plan, and `other` the
// wall at its near end; both nil where a typed value cannot move a wall.
struct Text {
  ElementId element;
  TextKind kind = TextKind::kLength;
  std::int64_t value = 0;
  geom::Vec2 at;
  geom::Vec2 along;
  double fit = 0.0;
  Mm from{};  // NOLINT(readability-redundant-member-init)
  bool forward = false;
  ElementId other{};  // NOLINT(readability-redundant-member-init)
};

// A line of a level's room schedule: a space, the room that names it (nil for none) and its
// floor area in m².
struct ListedRoom {
  ElementId room;
  RoomKind kind = RoomKind::kRoom;
  double area = 0.0;
};

// A line of a job's opening schedule: a mark, as the kind of its text and its number, the size
// it stands for and how many openings have it.
struct ListedOpening {
  TextKind kind = TextKind::kWindowMark;
  std::int64_t number = 0;
  Mm width;
  Mm height;
  std::int64_t count = 0;
};

// Painted in this order, each list in the order given. The schedules are what a sheet prints
// beside the plan: the level's rooms in reading order, from the top of the plan down and from
// the left, and the job's openings by mark.
struct Drawing {
  std::vector<Polygon> polygons;
  std::vector<Line> lines;
  std::vector<Arc> arcs;
  std::vector<Text> texts;
  std::vector<ListedRoom> rooms{};        // NOLINT(readability-redundant-member-init)
  std::vector<ListedOpening> openings{};  // NOLINT(readability-redundant-member-init)
};

// How big the painter's label text is on the plan at its zoom, in metres: the height of a line
// and the width of a digit.
struct TextSize {
  double height = 0.0;
  double digit = 0.0;
};

// What a wall is built of, from its outer face in (docs/adr/0014).
struct WallBuild {
  ElementId wall;
  std::vector<BuiltLayer> layers;
};

// The mark of an opening (docs/adr/0017): its number among the marks of its kind.
struct Mark {
  ElementId opening;
  std::int64_t number = 0;
};

// An opening as marks see it.
struct Marked {
  ElementId id;
  Opening opening;
};

// The marks of a job's openings, sorted by opening. Openings of one kind, width and height share
// a mark, and a kind's marks are numbered from 1 in the order of the oldest opening of each
// size: ids grow with time, so a new size never renumbers the others.
[[nodiscard]] std::vector<Mark> mark_openings(std::span<const Marked> openings);

// The job's opening schedule: one line for each mark, windows first, then doors, then empty
// openings, each by its number.
[[nodiscard]] std::vector<ListedOpening> list_openings(std::span<const Marked> openings);

// A room of the level: the name of the space that holds its anchor (docs/adr/0015).
struct NamedRoom {
  ElementId id;
  Room room;
};

// One level to draw: its walls, the openings in them, and the walls of the level under it.
// `builds` says what walls are built of; a wall with none there is one masonry layer, laid from
// the wall's own unit. `rooms` name the spaces the walls close. The explicit initializers let a
// plan be drawn without them.
struct Plan {
  std::span<const geom::WallInput> walls;
  std::span<const geom::PlacedOpening> openings;
  std::span<const geom::WallInput> below;
  std::span<const WallBuild> builds{};  // NOLINT(readability-redundant-member-init)
  std::span<const NamedRoom> rooms{};   // NOLINT(readability-redundant-member-init)
  // The space that holds this point is drawn with its sizes instead of its name.
  std::optional<PointMm> focus{};  // NOLINT(readability-redundant-member-init)
  // Every opening of the job: marks run through all its levels. An opening of the level that
  // is not among them has no mark.
  std::span<const Marked> marked{};  // NOLINT(readability-redundant-member-init)
  // The selected element; an opening is drawn with the chain of the side it is in.
  ElementId selected{};  // NOLINT(readability-redundant-member-init)
  // For a sheet: every space is drawn with its sizes beside its name.
  bool sheet = false;
};

// The level as a drawing.
//  - The walls of the level below, in outline.
//  - The floor of each space the walls close, the largest first, so a room standing free inside
//    another is painted over it. A floor names its room, or nothing when the space has no name.
//  - Each wall's joined outline, filled as masonry or as a wall with no material yet. A wall of
//    several layers is its layers' strips of that outline, each filled as what it is, under the
//    outline itself: insulation, facing, masonry, and nothing for a gap.
//  - Each opening's gap through its wall with its jambs; a window's faces and glazing line; a
//    door's leaf standing open at right angles on the edge it hangs on and the face it opens to,
//    with its swing; and the opening's mark beside it, on the side a leaf does not open to.
//  - Each wall's length beside it and along it, by its left face at the middle; where another
//    wall or another length lies there, by the other face or nearer an end. A length longer than
//    four fifths of its wall is left out, and so is the length of a wall round a space once it
//    was typed or measured: the space's sizes say more.
//  - In each space its room's name over its area, in the middle of its widest stretch: along x,
//    or up the plan in a space too narrow for a name. Each says how much room it has.
//  - In the space in focus, in place of its name, its clear size across and up the plan and a
//    chain along each side with openings: wall, opening, wall (docs/adr/0016).
//  - Once the walls close a space, the plan's overall size across its top and up its left side.
// Precondition: every wall passes validate() and ids are unique. The result does not depend on
// the order of the inputs.
[[nodiscard]] Drawing draft(const Plan& plan, TextSize text);

}  // namespace otves::drafting
