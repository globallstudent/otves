#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/editing.h>
#include <otves/geom/spaces.h>
#include <otves/geom/wall_joins.h>

namespace otves::geom {
namespace {

using namespace otves::literals;

PointMm at(std::int64_t x, std::int64_t y) { return {.x = Mm{x}, .y = Mm{y}}; }

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

WallInput wall(std::int64_t n, PointMm start, PointMm end, Mm thickness = 400_mm) {
  return {.id = id(n),
          .wall = {.start = start, .end = end, .thickness = thickness, .height = 2800_mm}};
}

// A room 6 by 4 m between the walls' centre lines, from the origin, drawn counter-clockwise:
// the bottom wall, the right, the top and the left. Its inner faces stand at 0.2 and 5.8 m
// across and at 0.2 and 3.8 m up; its outer ones 0.2 m outside the centre lines.
std::vector<WallInput> box() {
  return {wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(6000, 4000)),
          wall(3, at(6000, 4000), at(0, 4000)), wall(4, at(0, 4000), at(0, 0))};
}

// A touch with a 300 mm reach on a 100 mm grid, for a room that lies right of it and above.
RoomAsk ask(PointMm touched, Mm width, Mm depth) {
  return {.at = touched, .reach = 300_mm, .grid = 100_mm, .width = width, .depth = depth};
}

// A wall along its inner face, its body outside.
WallLine line(PointMm start, PointMm end) {
  return {.start = start, .end = end, .line = LocationLine::kLeftFace};
}

// The walls to build, or nothing when the room is refused.
std::optional<std::vector<WallLine>> built(const std::vector<WallInput>& walls,
                                           const RoomAsk& asked) {
  const Result<std::vector<WallLine>> found = room_walls(walls, asked);
  return found ? std::optional{*found} : std::nullopt;
}

// Why the room is refused, or nothing when it is not.
std::optional<ErrorCode> refusal(const std::vector<WallInput>& walls, const RoomAsk& asked) {
  const Result<std::vector<WallLine>> found = room_walls(walls, asked);
  return found ? std::nullopt : std::optional{found.error().code};
}

// The plan with the new walls built 120 mm thick, their ids from 10.
std::vector<WallInput> with(std::vector<WallInput> walls, const std::vector<WallLine>& lines) {
  std::int64_t next = 10;
  for (const WallLine& placed : lines) {
    WallInput added = wall(next++, placed.start, placed.end, 120_mm);
    added.wall.location_line = placed.line;
    walls.push_back(added);
  }
  return walls;
}

// The floor areas of the plan's spaces, the smallest first.
std::vector<double> areas(const std::vector<WallInput>& walls) {
  std::vector<double> found;
  for (const Space& space : find_spaces(walls)) {
    found.push_back(space.area);
  }
  std::ranges::sort(found);
  return found;
}

TEST(RoomWalls, ARoomInTheOpenIsFourWallsRoundItsClearSize) {
  // The touch falls on the grid: 1 200 across, 5 700 up.
  const std::optional<std::vector<WallLine>> room =
      built({}, ask(at(1234, 5678), 4000_mm, 3000_mm));

  ASSERT_TRUE(room.has_value());
  const std::vector<WallLine> lines = room.value_or(std::vector<WallLine>{});
  EXPECT_EQ(lines,
            (std::vector<WallLine>{
                line(at(1200, 5700), at(5200, 5700)), line(at(5200, 5700), at(5200, 8700)),
                line(at(5200, 8700), at(1200, 8700)), line(at(1200, 8700), at(1200, 5700))}));
  // The walls close a room of that size, whatever they are made of.
  const std::vector<WallInput> walls = with({}, lines);
  EXPECT_TRUE(join_walls(walls).issues.empty());
  const std::vector<double> floors = areas(walls);
  ASSERT_EQ(floors.size(), 1U);
  EXPECT_NEAR(floors[0], 4.0 * 3.0, 1e-9);

  // To the left of the touch and below it.
  RoomAsk other = ask(at(1234, 5678), 4000_mm, 3000_mm);
  other.right = false;
  other.up = false;
  const std::optional<std::vector<WallLine>> mirrored = built({}, other);
  ASSERT_TRUE(mirrored.has_value());
  EXPECT_EQ(mirrored.value_or(std::vector<WallLine>{}).front(),
            line(at(-2800, 2700), at(1200, 2700)));
}

TEST(RoomWalls, ARoomInACornerOfAnotherUsesTheWallsThere) {
  const std::vector<WallInput> walls = box();

  // The touch is nearest the inner corner at 200, 200. The bottom and the left side lie along
  // the faces there, so two walls are built, and each ends on the line of the wall it meets.
  const std::optional<std::vector<WallLine>> room =
      built(walls, ask(at(300, 350), 1600_mm, 2000_mm));
  ASSERT_TRUE(room.has_value());
  const std::vector<WallLine> lines = room.value_or(std::vector<WallLine>{});
  EXPECT_EQ(lines, (std::vector<WallLine>{line(at(1800, 0), at(1800, 2200)),
                                          line(at(1800, 2200), at(0, 2200))}));

  // The new room is as large as asked; the old one keeps the rest of its 5.6 by 3.6 m.
  const std::vector<WallInput> all = with(walls, lines);
  EXPECT_TRUE(join_walls(all).issues.empty());
  const std::vector<double> floors = areas(all);
  ASSERT_EQ(floors.size(), 2U);
  EXPECT_NEAR(floors[0], 1.6 * 2.0, 1e-9);
  EXPECT_NEAR(floors[1], (5.6 * 3.6) - (1.72 * 2.12), 1e-9);

  // The order of the walls does not matter.
  std::vector<WallInput> reversed = walls;
  std::ranges::reverse(reversed);
  EXPECT_EQ(built(reversed, ask(at(300, 350), 1600_mm, 2000_mm)), room);
}

TEST(RoomWalls, ARoomAgainstAWallOutsideSharesThatWall) {
  const std::vector<WallInput> walls = box();

  // No corner is within reach of the touch: it falls on the right wall's outer face, at 6 200.
  const std::optional<std::vector<WallLine>> room =
      built(walls, ask(at(6250, 1000), 3000_mm, 2000_mm));
  ASSERT_TRUE(room.has_value());
  const std::vector<WallLine> lines = room.value_or(std::vector<WallLine>{});
  EXPECT_EQ(lines, (std::vector<WallLine>{line(at(6000, 1000), at(9200, 1000)),
                                          line(at(9200, 1000), at(9200, 3000)),
                                          line(at(9200, 3000), at(6000, 3000))}));
  const std::vector<double> floors = areas(with(walls, lines));
  ASSERT_EQ(floors.size(), 2U);
  EXPECT_NEAR(floors[0], 3.0 * 2.0, 1e-9);
}

TEST(RoomWalls, ARoomThatCannotStandIsRefused) {
  const std::vector<WallInput> walls = box();

  EXPECT_EQ(refusal(walls, ask(at(3000, 2000), 0_mm, 2000_mm)), ErrorCode::kDoesNotFit);
  // From the middle of the box a room 5 m wide runs through its right wall.
  EXPECT_EQ(refusal(walls, ask(at(3000, 2000), 5000_mm, 1000_mm)), ErrorCode::kDoesNotFit);
  // On top of the box, from its outer corner, and wider than the box: the top wall runs
  // along part of its bottom side only.
  EXPECT_EQ(refusal(walls, ask(at(-150, 4250), 8000_mm, 2000_mm)), ErrorCode::kDoesNotFit);
  // The box itself: every side is a wall already.
  EXPECT_EQ(refusal(walls, ask(at(300, 300), 5600_mm, 3600_mm)), ErrorCode::kDoesNotFit);
  // As wide as the box, on top of it: its top wall is the room's bottom side.
  EXPECT_FALSE(refusal(walls, ask(at(-150, 4250), 6400_mm, 2000_mm)).has_value());
}

}  // namespace
}  // namespace otves::geom
