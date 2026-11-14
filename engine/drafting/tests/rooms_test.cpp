#include <algorithm>
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

geom::WallInput wall(std::int64_t n, PointMm start, PointMm end, Mm thickness = 400_mm) {
  return {.id = id(n),
          .wall = {.start = start, .end = end, .thickness = thickness, .height = 2800_mm}};
}

// A room of `width` by `depth` between the walls' centre lines, from the origin.
std::vector<geom::WallInput> box(std::int64_t width, std::int64_t depth) {
  return {wall(1, at(0, 0), at(width, 0)), wall(2, at(width, 0), at(width, depth)),
          wall(3, at(width, depth), at(0, depth)), wall(4, at(0, depth), at(0, 0))};
}

NamedRoom room(std::int64_t n, PointMm anchor, RoomKind kind) {
  return {.id = id(n), .room = {.level = id(100), .anchor = anchor, .kind = kind}};
}

// Label text 300 mm high on the plan, its digits 150 mm wide.
constexpr TextSize kText{.height = 0.3, .digit = 0.15};

Drawing draft_rooms(const std::vector<geom::WallInput>& walls,
                    const std::vector<NamedRoom>& rooms) {
  return draft({.walls = walls, .openings = {}, .below = {}, .builds = {}, .rooms = rooms}, kText);
}

std::vector<Polygon> floors(const Drawing& drawing) {
  std::vector<Polygon> found;
  std::ranges::copy_if(drawing.polygons, std::back_inserter(found),
                       [](const Polygon& polygon) { return polygon.fill == Fill::kFloor; });
  return found;
}

std::vector<Text> texts(const Drawing& drawing, TextKind kind) {
  std::vector<Text> found;
  std::ranges::copy_if(drawing.texts, std::back_inserter(found),
                       [kind](const Text& text) { return text.kind == kind; });
  return found;
}

void expect_near(geom::Vec2 actual, geom::Vec2 expected) {
  EXPECT_NEAR(actual.x, expected.x, 1e-9);
  EXPECT_NEAR(actual.y, expected.y, 1e-9);
}

TEST(DraftRooms, ASpaceWithNoNameIsItsFloorAndItsArea) {
  const Drawing drawing = draft_rooms(box(6000, 4000), {});

  // The floor comes before the walls, so they are painted over its edge.
  ASSERT_EQ(floors(drawing).size(), 1U);
  EXPECT_EQ(drawing.polygons[0].fill, Fill::kFloor);
  EXPECT_TRUE(drawing.polygons[0].element.is_nil());
  EXPECT_EQ(drawing.polygons[0].stroke, Stroke::kNone);

  EXPECT_TRUE(texts(drawing, TextKind::kRoomName).empty());
  const std::vector<Text> areas = texts(drawing, TextKind::kRoomArea);
  ASSERT_EQ(areas.size(), 1U);
  // 5.6 × 3.6 m between the faces, in cm².
  EXPECT_EQ(areas[0].value, 201'600);
  expect_near(areas[0].at, {.x = 3.0, .y = 2.0});
  expect_near(areas[0].along, {.x = 1.0, .y = 0.0});
  // Four fifths of the 5.6 m across.
  EXPECT_NEAR(areas[0].fit, 4.48, 1e-9);
}

TEST(DraftRooms, ARoomIsItsNameOverItsArea) {
  const Drawing drawing =
      draft_rooms(box(6000, 4000), {room(10, at(1000, 1000), RoomKind::kBedroom)});

  EXPECT_EQ(floors(drawing)[0].element, id(10));
  const std::vector<Text> names = texts(drawing, TextKind::kRoomName);
  const std::vector<Text> areas = texts(drawing, TextKind::kRoomArea);
  ASSERT_EQ(names.size(), 1U);
  ASSERT_EQ(areas.size(), 1U);
  EXPECT_EQ(names[0].element, id(10));
  EXPECT_EQ(names[0].value, static_cast<std::int64_t>(RoomKind::kBedroom));
  EXPECT_EQ(areas[0].element, id(10));
  // Two lines round the middle of the room, wherever its anchor lies.
  expect_near(names[0].at, {.x = 3.0, .y = 2.15});
  expect_near(areas[0].at, {.x = 3.0, .y = 1.85});
}

TEST(DraftRooms, ANarrowRoomIsWrittenUpThePlan) {
  // A corridor 1 m clear and 5.6 m long: a name does not fit across it.
  const Drawing drawing =
      draft_rooms(box(1400, 6000), {room(10, at(700, 3000), RoomKind::kCorridor)});

  const std::vector<Text> names = texts(drawing, TextKind::kRoomName);
  ASSERT_EQ(names.size(), 1U);
  expect_near(names[0].along, {.x = 0.0, .y = 1.0});
  // The name is the line to the left: the top of text read from the bottom up.
  expect_near(names[0].at, {.x = 0.7 - 0.15, .y = 3.0});
  expect_near(texts(drawing, TextKind::kRoomArea)[0].at, {.x = 0.7 + 0.15, .y = 3.0});
  EXPECT_NEAR(names[0].fit, 5.6 * 0.8, 1e-9);
}

TEST(DraftRooms, ARoomTooLowForTwoLinesKeepsItsAreaAndOneTooLowForALineNothing) {
  // 0.5 m clear: one line of 0.3 m.
  const Drawing low = draft_rooms(box(6000, 900), {room(10, at(3000, 450), RoomKind::kStorage)});
  EXPECT_TRUE(texts(low, TextKind::kRoomName).empty());
  ASSERT_EQ(texts(low, TextKind::kRoomArea).size(), 1U);
  expect_near(texts(low, TextKind::kRoomArea)[0].at, {.x = 3.0, .y = 0.45});

  // 0.2 m clear.
  const Drawing slit = draft_rooms(box(6000, 600), {});
  EXPECT_EQ(floors(slit).size(), 1U);
  EXPECT_TRUE(slit.texts.empty() || texts(slit, TextKind::kRoomArea).empty());
}

TEST(DraftRooms, AnLShapedRoomIsLabelledOnItsFloor) {
  // A 6 × 6 m square with its upper right 3 × 3 m left out: the centre of area lies at
  // (2.5, 2.5), on the floor; the widest stretch there runs the whole lower part.
  const std::vector<geom::WallInput> walls{
      wall(1, at(0, 0), at(6000, 0)),          wall(2, at(6000, 0), at(6000, 3000)),
      wall(3, at(6000, 3000), at(3000, 3000)), wall(4, at(3000, 3000), at(3000, 6000)),
      wall(5, at(3000, 6000), at(0, 6000)),    wall(6, at(0, 6000), at(0, 0))};
  const Drawing drawing = draft_rooms(walls, {});
  const std::vector<Text> areas = texts(drawing, TextKind::kRoomArea);
  ASSERT_EQ(areas.size(), 1U);
  // Across: from 0.2 to 5.8 at that height, so x = 3.0; up at x = 3.0: the lower part only.
  expect_near(areas[0].at, {.x = 3.0, .y = 1.5});
}

TEST(DraftRooms, ARoomStandingFreeInAnotherIsPaintedOverIt) {
  const std::vector<geom::WallInput> walls{wall(1, at(0, 0), at(10'000, 0)),
                                           wall(2, at(10'000, 0), at(10'000, 8000)),
                                           wall(3, at(10'000, 8000), at(0, 8000)),
                                           wall(4, at(0, 8000), at(0, 0)),
                                           wall(5, at(3000, 3000), at(6000, 3000), 200_mm),
                                           wall(6, at(6000, 3000), at(6000, 5000), 200_mm),
                                           wall(7, at(6000, 5000), at(3000, 5000), 200_mm),
                                           wall(8, at(3000, 5000), at(3000, 3000), 200_mm)};
  const std::vector<NamedRoom> rooms{room(20, at(4500, 4000), RoomKind::kBoiler),
                                     room(21, at(1000, 1000), RoomKind::kLivingRoom)};
  const Drawing drawing = draft_rooms(walls, rooms);

  const std::vector<Polygon> painted = floors(drawing);
  ASSERT_EQ(painted.size(), 2U);
  EXPECT_EQ(painted[0].element, id(21));
  EXPECT_EQ(painted[1].element, id(20));
  // The outer room's label keeps off the inner one: of the two stretches beside it at mid
  // height, 2.7 m to the left and 3.7 m to the right, it takes the wider.
  const std::vector<Text> names = texts(drawing, TextKind::kRoomName);
  const auto outer = std::ranges::find(names, id(21), &Text::element);
  ASSERT_NE(outer, names.end());
  expect_near(outer->at, {.x = 7.95, .y = 4.15});
}

// Whether the text is written up the plan and outside the 2 m room at the origin.
bool upright_outside(const Text& text) {
  return text.along.y > 0.5 && (text.at.x < 0.0 || text.at.x > 2.0);
}

TEST(DraftRooms, WallLengthsKeepClearOfARoomsLabel) {
  // A room 1.6 m clear each way, sketched: the lengths of its walls, written inside by their
  // left faces until somebody gives them, would lie on its label.
  std::vector<geom::WallInput> walls = box(2000, 2000);
  for (geom::WallInput& input : walls) {
    input.wall.exact = false;
  }
  const Drawing drawing = draft_rooms(walls, {room(10, at(1000, 1000), RoomKind::kToilet)});
  const std::vector<Text> names = texts(drawing, TextKind::kRoomName);
  ASSERT_EQ(names.size(), 1U);
  const std::vector<Text> lengths = texts(drawing, TextKind::kAboutLength);
  ASSERT_EQ(lengths.size(), 4U);
  // Over and under the label there is room for a line; beside it there is not, so the lengths
  // of the two upright walls went to their other faces, outside the room.
  EXPECT_TRUE(std::ranges::all_of(lengths, [&names](const Text& length) {
    return geom::length(length.at - names[0].at) >= 0.4;
  }));
  EXPECT_EQ(std::ranges::count_if(lengths, upright_outside), 2);
}

TEST(DraftRooms, ALabelMovesOffWhatADoorSweepsWhereThereIsRoomForIt) {
  // Two rooms, one over the other; a door in the wall between them opens down the plan into the
  // lower one, 1.6 m deep, over the middle of its floor.
  std::vector<geom::WallInput> walls = box(6000, 4000);
  walls.push_back(wall(5, at(0, 2000), at(6000, 2000)));
  const std::vector<geom::PlacedOpening> openings{{.id = id(20),
                                                   .wall = walls[4].wall,
                                                   .opening = {.wall = id(5),
                                                               .offset = 2800_mm,
                                                               .width = 900_mm,
                                                               .height = 2100_mm,
                                                               .sill = 0_mm,
                                                               .kind = OpeningKind::kDoor,
                                                               .swing = Swing::kRight}}};
  Plan plan{.walls = walls, .openings = openings, .below = {}};

  // The leaf sweeps from 2.8 to 3.7 m: the area goes to the middle of the longer part beside
  // it, from the wall at 0.2 m. The upper room's stays in its middle.
  const std::vector<Text> areas = texts(draft(plan, kText), TextKind::kRoomArea);
  ASSERT_EQ(areas.size(), 2U);
  expect_near(areas[0].at, {.x = 1.5, .y = 1.0});
  expect_near(areas[1].at, {.x = 3.0, .y = 3.0});

  // A name is taken to need more room than that part has: it stays where it was.
  const std::vector<NamedRoom> rooms{room(10, at(3000, 1000), RoomKind::kHall)};
  plan.rooms = rooms;
  const std::vector<Text> names = texts(draft(plan, kText), TextKind::kRoomName);
  ASSERT_EQ(names.size(), 1U);
  expect_near(names[0].at, {.x = 3.0, .y = 1.0 + 0.15});
}

}  // namespace
}  // namespace otves::drafting
