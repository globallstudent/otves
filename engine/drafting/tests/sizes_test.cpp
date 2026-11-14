#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/room.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/drafting/plan.h>
#include <otves/geom/openings.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

namespace otves::drafting {
namespace {

using namespace otves::literals;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

PointMm at(std::int64_t x, std::int64_t y) { return {.x = Mm{x}, .y = Mm{y}}; }

geom::WallInput wall(std::int64_t n, PointMm start, PointMm end, bool exact = true) {
  return {
      .id = id(n),
      .wall = {.start = start, .end = end, .thickness = 400_mm, .height = 2800_mm, .exact = exact}};
}

// A room 6 by 4 m between the walls' centre lines, from the origin, drawn counter-clockwise:
// 5.6 by 3.6 m clear.
std::vector<geom::WallInput> box() {
  return {wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(6000, 4000)),
          wall(3, at(6000, 4000), at(0, 4000)), wall(4, at(0, 4000), at(0, 0))};
}

// A window 1.5 m wide.
geom::PlacedOpening window(std::int64_t n, const geom::WallInput& in, Mm offset) {
  return {
      .id = id(n),
      .wall = in.wall,
      .opening = {
          .wall = in.id, .offset = offset, .width = 1500_mm, .height = 1500_mm, .sill = 900_mm}};
}

// A door 900 mm wide that opens to its wall's left.
geom::PlacedOpening door(std::int64_t n, const geom::WallInput& in, Mm offset) {
  return {.id = id(n),
          .wall = in.wall,
          .opening = {.wall = in.id,
                      .offset = offset,
                      .width = 900_mm,
                      .height = 2100_mm,
                      .sill = 0_mm,
                      .kind = OpeningKind::kDoor}};
}

// Label text 200 mm high on the plan, its digits 100 mm wide.
constexpr TextSize kText{.height = 0.2, .digit = 0.1};

std::vector<Text> texts(const Drawing& drawing, TextKind kind) {
  std::vector<Text> found;
  std::ranges::copy_if(drawing.texts, std::back_inserter(found),
                       [kind](const Text& text) { return text.kind == kind; });
  return found;
}

std::vector<std::int64_t> values(const std::vector<Text>& found) {
  std::vector<std::int64_t> all;
  std::ranges::transform(found, std::back_inserter(all), &Text::value);
  return all;
}

std::ptrdiff_t lines(const Drawing& drawing, Stroke stroke) {
  return std::ranges::count(drawing.lines, stroke, &Line::stroke);
}

void expect_near(geom::Vec2 actual, geom::Vec2 expected) {
  EXPECT_NEAR(actual.x, expected.x, 1e-9);
  EXPECT_NEAR(actual.y, expected.y, 1e-9);
}

TEST(DraftSizes, APlanThatClosesASpaceHasItsOverallSizeOutside) {
  const std::vector<geom::WallInput> walls = box();
  const Drawing drawing = draft({.walls = walls, .openings = {}, .below = {}}, kText);

  const std::vector<Text> overall = texts(drawing, TextKind::kOverall);
  ASSERT_EQ(overall.size(), 2U);
  // From outer face to outer face: 6.4 by 4.4 m.
  EXPECT_EQ(values(overall), (std::vector<std::int64_t>{6400, 4400}));
  // Across the top, two text heights out, the number over the line.
  expect_near(overall[0].at, {.x = 3.0, .y = 4.2 + 0.4 + 0.13});
  expect_near(overall[0].along, {.x = 1.0, .y = 0.0});
  // Up the left side, read from the bottom, the number to the left of the line.
  expect_near(overall[1].at, {.x = -0.2 - 0.4 - 0.13, .y = 2.0});
  expect_near(overall[1].along, {.x = 0.0, .y = 1.0});
  // Two dimension lines, two ticks on each, and four lines out from the plan's corners.
  EXPECT_EQ(lines(drawing, Stroke::kDimension), 10);
  EXPECT_EQ(lines(drawing, Stroke::kChain), 0);
}

TEST(DraftSizes, WallsThatCloseNothingHaveTheirLengthsAndNoOverallSize) {
  std::vector<geom::WallInput> walls = box();
  walls.pop_back();
  const Drawing drawing = draft({.walls = walls, .openings = {}, .below = {}}, kText);

  EXPECT_TRUE(texts(drawing, TextKind::kOverall).empty());
  EXPECT_EQ(texts(drawing, TextKind::kLength).size(), 3U);
}

TEST(DraftSizes, AWallRoundASpaceKeepsItsLengthOnlyUntilItIsGiven) {
  std::vector<geom::WallInput> walls = box();
  walls[1].wall.exact = false;
  // A wall that stands free outside is sized as before.
  walls.push_back(wall(5, at(8000, 0), at(8000, 3000)));
  const Drawing drawing = draft({.walls = walls, .openings = {}, .below = {}}, kText);

  const std::vector<Text> rough = texts(drawing, TextKind::kAboutLength);
  ASSERT_EQ(rough.size(), 1U);
  EXPECT_EQ(rough[0].element, id(2));
  const std::vector<Text> given = texts(drawing, TextKind::kLength);
  ASSERT_EQ(given.size(), 1U);
  EXPECT_EQ(given[0].element, id(5));
}

TEST(DraftSizes, TheSpaceInFocusHasItsClearSizesInPlaceOfItsName) {
  const std::vector<geom::WallInput> walls = box();
  const std::vector<NamedRoom> rooms{
      {.id = id(10),
       .room = {.level = id(100), .anchor = at(1000, 1000), .kind = RoomKind::kStudy}}};
  const Plan plan{.walls = walls,
                  .openings = {},
                  .below = {},
                  .builds = {},
                  .rooms = rooms,
                  .focus = at(3000, 2000)};
  const Drawing drawing = draft(plan, kText);

  EXPECT_TRUE(texts(drawing, TextKind::kRoomName).empty());
  EXPECT_TRUE(texts(drawing, TextKind::kRoomArea).empty());
  const std::vector<Text> clear = texts(drawing, TextKind::kClear);
  ASSERT_EQ(clear.size(), 2U);
  EXPECT_EQ(values(clear), (std::vector<std::int64_t>{5600, 3600}));
  // Across, from the left wall to the right one; up, from the lower wall to the upper one.
  EXPECT_EQ(clear[0].other, id(4));
  EXPECT_EQ(clear[0].element, id(2));
  EXPECT_EQ(clear[1].other, id(1));
  EXPECT_EQ(clear[1].element, id(3));
  // Across: 2.6 text heights under the top face, the number over the line.
  expect_near(clear[0].at, {.x = 3.0, .y = 3.8 - 0.52 + 0.13});
  // Up: 2.6 text heights from the left face, the number to the left of the line.
  expect_near(clear[1].at, {.x = 0.2 + 0.52 - 0.13, .y = 2.0});
  expect_near(clear[1].along, {.x = 0.0, .y = 1.0});
  EXPECT_EQ(lines(drawing, Stroke::kChain), 6);

  // With no focus, or with the focus outside, the room has its name back.
  Plan away = plan;
  away.focus = at(9000, 9000);
  EXPECT_EQ(texts(draft(away, kText), TextKind::kRoomName).size(), 1U);
  EXPECT_TRUE(texts(draft(away, kText), TextKind::kClear).empty());
}

TEST(DraftSizes, ASideWithOpeningsHasAChainOfWallOpeningWall) {
  const std::vector<geom::WallInput> walls = box();
  // In the lower wall, which runs the way its side of the room does: a window 2 m from the
  // wall's start, 1.8 m from the room's corner.
  const std::vector<geom::PlacedOpening> openings{window(20, walls[0], 2000_mm)};
  const Plan plan{.walls = walls,
                  .openings = openings,
                  .below = {},
                  .builds = {},
                  .rooms = {},
                  .focus = at(3000, 2000)};
  const Drawing drawing = draft(plan, kText);

  const std::vector<Text> piers = texts(drawing, TextKind::kPier);
  const std::vector<Text> widths = texts(drawing, TextKind::kOpeningWidth);
  ASSERT_EQ(piers.size(), 2U);
  ASSERT_EQ(widths.size(), 1U);
  EXPECT_EQ(values(piers), (std::vector<std::int64_t>{1800, 2300}));
  EXPECT_EQ(widths[0].value, 1500);
  EXPECT_EQ(widths[0].element, id(20));
  // The chain lies 1.2 text heights inside the lower face, its numbers over it.
  expect_near(widths[0].at, {.x = 2.75, .y = 0.2 + 0.24 + 0.13});
  // The first piece of wall leads from the corner, 200 mm along the wall, on to the window;
  // the last one from the far corner, 5 800 mm along, back to it.
  EXPECT_EQ(piers[0].element, id(20));
  EXPECT_EQ(piers[0].from, 200_mm);
  EXPECT_TRUE(piers[0].forward);
  EXPECT_EQ(piers[1].from, 5800_mm);
  EXPECT_FALSE(piers[1].forward);
}

TEST(DraftSizes, AChainReadsTheSameWhicheverWayTheWallWasDrawn) {
  std::vector<geom::WallInput> walls = box();
  // The lower wall drawn from right to left: the window is 2.5 m from its start.
  std::swap(walls[0].wall.start, walls[0].wall.end);
  const std::vector<geom::PlacedOpening> openings{window(20, walls[0], 2500_mm)};
  const Plan plan{.walls = walls,
                  .openings = openings,
                  .below = {},
                  .builds = {},
                  .rooms = {},
                  .focus = at(3000, 2000)};
  const Drawing drawing = draft(plan, kText);

  const std::vector<Text> piers = texts(drawing, TextKind::kPier);
  ASSERT_EQ(piers.size(), 2U);
  EXPECT_EQ(values(piers), (std::vector<std::int64_t>{1800, 2300}));
  // Along the wall the room's first corner is now 5 800 mm from the start, and the window lies
  // before it.
  EXPECT_EQ(piers[0].from, 5800_mm);
  EXPECT_FALSE(piers[0].forward);
  EXPECT_EQ(piers[1].from, 200_mm);
  EXPECT_TRUE(piers[1].forward);
}

TEST(DraftSizes, ANumberTooLongForItsPieceIsLeftOut) {
  const std::vector<geom::WallInput> walls = box();
  // A window 250 mm from the room's corner: «250» is 300 mm wide in this text.
  const std::vector<geom::PlacedOpening> openings{window(20, walls[0], 450_mm)};
  const Plan plan{.walls = walls,
                  .openings = openings,
                  .below = {},
                  .builds = {},
                  .rooms = {},
                  .focus = at(3000, 2000)};
  const Drawing drawing = draft(plan, kText);

  EXPECT_EQ(values(texts(drawing, TextKind::kPier)), (std::vector<std::int64_t>{3850}));
  EXPECT_EQ(texts(drawing, TextKind::kOpeningWidth).size(), 1U);
}

TEST(DraftSizes, AClearSizeGoesFurtherInRatherThanThroughANumber) {
  const std::vector<geom::WallInput> walls = box();
  // In the upper wall, drawn from right to left, a window whose near edge is 1.04 m from the
  // room's left corner: the middle of that piece of wall is where the clear size up the plan
  // would run, 2.6 text heights from the left face.
  const std::vector<geom::PlacedOpening> openings{window(20, walls[2], 3260_mm)};
  const Plan plan{.walls = walls,
                  .openings = openings,
                  .below = {},
                  .builds = {},
                  .rooms = {},
                  .focus = at(3000, 2000)};
  const Drawing drawing = draft(plan, kText);

  const std::vector<Text> piers = texts(drawing, TextKind::kPier);
  ASSERT_EQ(piers.size(), 2U);
  EXPECT_EQ(piers[1].value, 1040);
  expect_near(piers[1].at, {.x = 0.72, .y = 3.8 - 0.24 + 0.13});
  // One step further in: 3.9 text heights from the left face.
  const std::vector<Text> clear = texts(drawing, TextKind::kClear);
  ASSERT_EQ(clear.size(), 2U);
  expect_near(clear[1].at, {.x = 0.2 + 0.78 - 0.13, .y = 2.0});
}

TEST(DraftSizes, ASheetHasEverySpacesSizesBesideItsNameAndListsTheRooms) {
  // Two rooms side by side: the left one 2 m between centre lines, the right one 4 m.
  std::vector<geom::WallInput> walls = box();
  walls.push_back(wall(5, at(2000, 0), at(2000, 4000)));
  const std::vector<NamedRoom> rooms{
      {.id = id(10),
       .room = {.level = id(100), .anchor = at(4000, 2000), .kind = RoomKind::kStudy}}};
  const Plan plan{
      .walls = walls, .openings = {}, .below = {}, .builds = {}, .rooms = rooms, .sheet = true};
  const Drawing drawing = draft(plan, kText);

  // Both rooms have their clear sizes, in the line of a sheet, and the named one its name too.
  EXPECT_EQ(texts(drawing, TextKind::kClear).size(), 4U);
  EXPECT_EQ(lines(drawing, Stroke::kChain), 0);
  EXPECT_EQ(texts(drawing, TextKind::kRoomName).size(), 1U);
  EXPECT_EQ(texts(drawing, TextKind::kRoomArea).size(), 2U);

  // The schedule reads from the left: the room with no name, then the study.
  ASSERT_EQ(drawing.rooms.size(), 2U);
  EXPECT_TRUE(drawing.rooms[0].room.is_nil());
  EXPECT_NEAR(drawing.rooms[0].area, 1.6 * 3.6, 1e-9);
  EXPECT_EQ(drawing.rooms[1].room, id(10));
  EXPECT_EQ(drawing.rooms[1].kind, RoomKind::kStudy);
  EXPECT_NEAR(drawing.rooms[1].area, 3.6 * 3.6, 1e-9);

  // On a screen with nothing in focus there are no room sizes, and the schedule is the same.
  Plan screen = plan;
  screen.sheet = false;
  const Drawing shown = draft(screen, kText);
  EXPECT_TRUE(texts(shown, TextKind::kClear).empty());
  EXPECT_EQ(shown.rooms.size(), 2U);
}

TEST(DraftSizes, AMarkStandsClearOfTheSizesOfTheRoomsItsOpeningIsBetween) {
  // Two rooms, one over the other, with a door between them that opens up the plan.
  std::vector<geom::WallInput> walls = box();
  walls.push_back(wall(5, at(0, 2000), at(6000, 2000)));
  const std::vector<geom::PlacedOpening> openings{door(20, walls[4], 2500_mm)};
  const std::vector<Marked> job{{.id = id(20), .opening = openings[0].opening}};
  Plan plan{.walls = walls, .openings = openings, .below = {}, .marked = job};

  // On a screen with nothing in focus no size is in its way: a text height off the face the
  // door does not open to.
  const std::vector<Text> shown = texts(draft(plan, kText), TextKind::kDoorMark);
  ASSERT_EQ(shown.size(), 1U);
  expect_near(shown[0].at, {.x = 2.95, .y = 1.8 - 0.2});

  // On a sheet the wall between the rooms is sized once, in the first of two rooms as wide as
  // each other: the lower one, where the mark stood. It goes to the other face, into the turn
  // of the door's leaf, which it does not touch.
  plan.sheet = true;
  const Drawing sheet = draft(plan, kText);
  EXPECT_EQ(texts(sheet, TextKind::kOpeningWidth).size(), 1U);
  EXPECT_LT(texts(sheet, TextKind::kOpeningWidth)[0].at.y, 1.8);
  const std::vector<Text> printed = texts(sheet, TextKind::kDoorMark);
  ASSERT_EQ(printed.size(), 1U);
  expect_near(printed[0].at, {.x = 2.95, .y = 2.2 + 0.2});
}

TEST(DraftSizes, ASheetLeavesOutTheLengthsOfTheWallsRoundItsSpaces) {
  // Nobody has given the length of any of these walls; the last one closes nothing.
  std::vector<geom::WallInput> walls;
  for (const geom::WallInput& sketched : box()) {
    walls.push_back(wall(0, sketched.wall.start, sketched.wall.end, false));
    walls.back().id = sketched.id;
  }
  walls.push_back(wall(9, at(9000, 0), at(9000, 3000), false));
  Plan plan{.walls = walls, .openings = {}, .below = {}};

  EXPECT_EQ(texts(draft(plan, kText), TextKind::kAboutLength).size(), 5U);

  // A sheet has the sizes of every space, so only the wall that closes nothing keeps its own.
  plan.sheet = true;
  const std::vector<Text> lengths = texts(draft(plan, kText), TextKind::kAboutLength);
  ASSERT_EQ(lengths.size(), 1U);
  EXPECT_EQ(lengths[0].element, id(9));
}

}  // namespace
}  // namespace otves::drafting
