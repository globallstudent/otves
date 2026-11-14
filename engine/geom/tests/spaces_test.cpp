#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/polygon.h>
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

// A room of `width` by `depth` between the walls' centre lines, drawn counter-clockwise from
// `corner`. Ids run from `first`.
std::vector<WallInput> box(std::int64_t first, PointMm corner, std::int64_t width,
                           std::int64_t depth, Mm thickness = 400_mm) {
  const std::int64_t x = corner.x.value();
  const std::int64_t y = corner.y.value();
  return {wall(first, at(x, y), at(x + width, y), thickness),
          wall(first + 1, at(x + width, y), at(x + width, y + depth), thickness),
          wall(first + 2, at(x + width, y + depth), at(x, y + depth), thickness),
          wall(first + 3, at(x, y + depth), at(x, y), thickness)};
}

double footprints(const std::vector<WallInput>& walls) {
  double total = 0.0;
  for (const WallFootprint& footprint : join_walls(walls).footprints) {
    total += signed_area(footprint.outline);
  }
  return total;
}

double floors(const std::vector<Space>& spaces) {
  double total = 0.0;
  for (const Space& space : spaces) {
    total += space.area;
  }
  return total;
}

std::vector<double> areas(const std::vector<Space>& spaces) {
  std::vector<double> all;
  all.reserve(spaces.size());
  for (const Space& space : spaces) {
    all.push_back(space.area);
  }
  std::ranges::sort(all);
  return all;
}

TEST(FindSpaces, ARoomIsBoundedByTheInnerFacesOfItsWalls) {
  const std::vector<Space> spaces = find_spaces(box(1, at(0, 0), 6000, 4000));
  ASSERT_EQ(spaces.size(), 1U);
  EXPECT_EQ(spaces[0].outline.size(), 4U);
  EXPECT_NEAR(spaces[0].area, 5.6 * 3.6, 1e-12);
  EXPECT_NEAR(spaces[0].perimeter, 2 * (5.6 + 3.6), 1e-12);
  EXPECT_TRUE(spaces[0].islands.empty());
  EXPECT_TRUE(holds(spaces[0], {3.0, 2.0}));
  EXPECT_FALSE(holds(spaces[0], {0.1, 2.0}));  // in the wall
  EXPECT_FALSE(holds(spaces[0], {-1.0, 2.0}));
}

TEST(FindSpaces, TheWayTheWallsWereDrawnDoesNotMatter) {
  std::vector<WallInput> walls = box(1, at(0, 0), 6000, 4000);
  std::swap(walls[1].wall.start, walls[1].wall.end);
  std::swap(walls[3].wall.start, walls[3].wall.end);
  std::ranges::reverse(walls);
  const std::vector<Space> spaces = find_spaces(walls);
  ASSERT_EQ(spaces.size(), 1U);
  EXPECT_NEAR(spaces[0].area, 5.6 * 3.6, 1e-12);
  EXPECT_GT(signed_area(spaces[0].outline), 0.0);
}

TEST(FindSpaces, FollowTheLocationLine) {
  std::vector<WallInput> walls = box(1, at(0, 0), 6000, 4000);
  // Drawn counter-clockwise, the right face is the outer one: the points are the outer size.
  for (WallInput& input : walls) {
    input.wall.location_line = LocationLine::kRightFace;
  }
  EXPECT_NEAR(find_spaces(walls)[0].area, 5.2 * 3.2, 1e-12);
  for (WallInput& input : walls) {
    input.wall.location_line = LocationLine::kLeftFace;
  }
  EXPECT_NEAR(find_spaces(walls)[0].area, 6.0 * 4.0, 1e-12);
}

TEST(FindSpaces, WallsThatDoNotCloseHoldNoSpace) {
  std::vector<WallInput> walls = box(1, at(0, 0), 6000, 4000);
  walls.pop_back();
  EXPECT_TRUE(find_spaces(walls).empty());
  // An end 10 mm short of the corner leaves the room open too.
  walls.push_back(wall(4, at(0, 4000), at(0, 10)));
  EXPECT_TRUE(find_spaces(walls).empty());
}

TEST(FindSpaces, APartitionStandingAgainstTwoWallsMakesTwoRooms) {
  std::vector<WallInput> walls = box(1, at(0, 0), 6000, 4000);
  walls.push_back(wall(5, at(2000, 0), at(2000, 4000), 120_mm));
  const std::vector<Space> spaces = find_spaces(walls);
  ASSERT_EQ(spaces.size(), 2U);
  const std::vector<double> sizes = areas(spaces);
  EXPECT_NEAR(sizes[0], (2.0 - 0.2 - 0.06) * 3.6, 1e-12);
  EXPECT_NEAR(sizes[1], (4.0 - 0.2 - 0.06) * 3.6, 1e-12);
  EXPECT_EQ(spaces[0].outline.size(), 4U);
  EXPECT_EQ(spaces[1].outline.size(), 4U);
  // Walls and floors tile the plan out to its outer faces.
  EXPECT_NEAR(floors(spaces) + footprints(walls), 6.4 * 4.4, 1e-12);
}

TEST(FindSpaces, ARoomMayHaveAnyShape) {
  // An L: a 6 × 6 m square with its upper right 3 × 3 m left out.
  const std::vector<WallInput> walls{
      wall(1, at(0, 0), at(6000, 0)),          wall(2, at(6000, 0), at(6000, 3000)),
      wall(3, at(6000, 3000), at(3000, 3000)), wall(4, at(3000, 3000), at(3000, 6000)),
      wall(5, at(3000, 6000), at(0, 6000)),    wall(6, at(0, 6000), at(0, 0))};
  const std::vector<Space> spaces = find_spaces(walls);
  ASSERT_EQ(spaces.size(), 1U);
  EXPECT_EQ(spaces[0].outline.size(), 6U);
  EXPECT_NEAR(spaces[0].area, (5.6 * 2.6) + (2.6 * 3.0), 1e-12);
  EXPECT_TRUE(holds(spaces[0], {1.5, 4.5}));
  EXPECT_FALSE(holds(spaces[0], {4.5, 4.5}));
}

TEST(FindSpaces, AWallEndingInsideARoomStandsInIt) {
  std::vector<WallInput> walls = box(1, at(0, 0), 6000, 4000);
  // From the lower wall 1 m into the room.
  walls.push_back(wall(5, at(3000, 0), at(3000, 1000), 200_mm));
  const std::vector<Space> spaces = find_spaces(walls);
  ASSERT_EQ(spaces.size(), 1U);
  EXPECT_EQ(spaces[0].outline.size(), 8U);
  EXPECT_NEAR(spaces[0].area, (5.6 * 3.6) - (0.2 * 0.8), 1e-12);
  EXPECT_NEAR(floors(spaces) + footprints(walls), 6.4 * 4.4, 1e-12);
  EXPECT_FALSE(holds(spaces[0], {3.0, 0.5}));
}

TEST(FindSpaces, WhatStandsFreeInARoomIsAnIsland) {
  std::vector<WallInput> walls = box(1, at(0, 0), 6000, 4000);
  walls.push_back(wall(5, at(2000, 2000), at(4000, 2000), 200_mm));
  const std::vector<Space> spaces = find_spaces(walls);
  ASSERT_EQ(spaces.size(), 1U);
  ASSERT_EQ(spaces[0].islands.size(), 1U);
  EXPECT_NEAR(spaces[0].area, (5.6 * 3.6) - (2.0 * 0.2), 1e-12);
  EXPECT_NEAR(spaces[0].perimeter, (2 * (5.6 + 3.6)) + (2 * (2.0 + 0.2)), 1e-12);
  EXPECT_LT(signed_area(spaces[0].islands[0]), 0.0);
  EXPECT_FALSE(holds(spaces[0], {3.0, 2.0}));
  EXPECT_TRUE(holds(spaces[0], {3.0, 3.0}));
}

TEST(FindSpaces, ARoomInsideARoomComesOffItsFloor) {
  std::vector<WallInput> walls = box(1, at(0, 0), 10'000, 8000);
  const std::vector<WallInput> inner = box(5, at(3000, 3000), 3000, 2000, 200_mm);
  walls.insert(walls.end(), inner.begin(), inner.end());
  const std::vector<Space> spaces = find_spaces(walls);
  ASSERT_EQ(spaces.size(), 2U);
  const std::vector<double> sizes = areas(spaces);
  EXPECT_NEAR(sizes[0], 2.8 * 1.8, 1e-12);
  EXPECT_NEAR(sizes[1], (9.6 * 7.6) - (3.2 * 2.2), 1e-12);
  EXPECT_NEAR(floors(spaces) + footprints(walls), 10.4 * 8.4, 1e-12);
}

TEST(FindSpaces, WallsOfDifferentThicknessInOneLineStep) {
  // The lower wall is two walls in one line, 400 and 200 mm thick.
  const std::vector<WallInput> walls{
      wall(1, at(0, 0), at(3000, 0)), wall(2, at(3000, 0), at(6000, 0), 200_mm),
      wall(3, at(6000, 0), at(6000, 4000)), wall(4, at(6000, 4000), at(0, 4000)),
      wall(5, at(0, 4000), at(0, 0))};
  const std::vector<Space> spaces = find_spaces(walls);
  ASSERT_EQ(spaces.size(), 1U);
  EXPECT_EQ(spaces[0].outline.size(), 6U);
  EXPECT_NEAR(spaces[0].area, (5.6 * 3.6) + (2.8 * 0.1), 1e-12);
}

TEST(FindSpaces, WallsAtAnAngleCloseARoomToo) {
  // A right triangle with legs of 4 m and 3 m, walls 200 mm thick on their centre lines.
  const std::vector<WallInput> walls{wall(1, at(0, 0), at(4000, 0), 200_mm),
                                     wall(2, at(4000, 0), at(0, 3000), 200_mm),
                                     wall(3, at(0, 3000), at(0, 0), 200_mm)};
  const std::vector<Space> spaces = find_spaces(walls);
  ASSERT_EQ(spaces.size(), 1U);
  EXPECT_EQ(spaces[0].outline.size(), 3U);
  // The inner triangle is the outer one with its inradius, 1 m, less 0.1 m: scaled by 0.9.
  EXPECT_NEAR(spaces[0].area, 6.0 * 0.9 * 0.9, 1e-12);
  EXPECT_NEAR(floors(spaces) + footprints(walls), 6.0 * 1.1 * 1.1, 1e-9);
}

constexpr std::int64_t kCell = 3000;
constexpr int kCells = 4;

// A grid of rooms, each side of each cell a wall of its own, with one side in four inside the
// grid left out.
std::vector<WallInput> grid(std::mt19937& random) {
  std::vector<WallInput> walls;
  std::int64_t next = 1;
  for (int row = 0; row <= kCells; ++row) {
    for (int column = 0; column <= kCells; ++column) {
      const bool edge_row = row == 0 || row == kCells;
      const bool edge_column = column == 0 || column == kCells;
      if (column < kCells && (edge_row || random() % 4 != 0)) {
        walls.push_back(wall(next++, at(column * kCell, row * kCell),
                             at((column + 1) * kCell, row * kCell), 300_mm));
      }
      if (row < kCells && (edge_column || random() % 4 != 0)) {
        walls.push_back(wall(next++, at(column * kCell, row * kCell),
                             at(column * kCell, (row + 1) * kCell), 300_mm));
      }
    }
  }
  return walls;
}

// Where a side is missing two cells are one room, so the walls and the floors still tile the
// plan, whatever order the walls come in.
TEST(FindSpaces, WallsAndFloorsTileAnyGridOfRooms) {
  // NOLINTNEXTLINE(bugprone-random-generator-seed): a fixed seed repeats the same plans.
  std::mt19937 random{20261114};
  for (int round = 0; round < 20; ++round) {
    std::vector<WallInput> walls = grid(random);
    const std::vector<Space> spaces = find_spaces(walls);
    EXPECT_GE(spaces.size(), 1U);
    EXPECT_NEAR(floors(spaces) + footprints(walls), 12.3 * 12.3, 1e-9) << "round " << round;

    std::ranges::shuffle(walls, random);
    EXPECT_EQ(areas(find_spaces(walls)), areas(spaces));
  }
}

TEST(NameSpaces, TheSpaceThatHoldsAnAnchorTakesItsRoom) {
  std::vector<WallInput> walls = box(1, at(0, 0), 6000, 4000);
  walls.push_back(wall(5, at(2000, 0), at(2000, 4000), 120_mm));
  const std::vector<Space> spaces = find_spaces(walls);
  const std::size_t small = spaces[0].area < spaces[1].area ? 0 : 1;

  const std::vector<Anchor> anchors{{.room = id(10), .at = at(1000, 2000)}};
  const Names names = name_spaces(spaces, anchors);
  ASSERT_EQ(names.rooms.size(), 2U);
  EXPECT_EQ(names.rooms[small], id(10));
  EXPECT_EQ(names.rooms[1 - small], std::nullopt);
  EXPECT_TRUE(names.issues.empty());
}

TEST(NameSpaces, AnAnchorOutsideEverySpaceAndASecondOneInASpaceAreIssues) {
  const std::vector<Space> spaces = find_spaces(box(1, at(0, 0), 6000, 4000));
  // The younger room comes first: the order given does not decide who keeps the space.
  const std::vector<Anchor> anchors{{.room = id(12), .at = at(4000, 2000)},
                                    {.room = id(11), .at = at(1000, 2000)},
                                    {.room = id(13), .at = at(9000, 2000)},
                                    {.room = id(14), .at = at(100, 2000)}};
  const Names names = name_spaces(spaces, anchors);
  EXPECT_EQ(names.rooms[0], id(11));
  const std::vector<AnchorIssue> expected{{.room = id(12), .kind = AnchorIssueKind::kSharedSpace},
                                          {.room = id(13), .kind = AnchorIssueKind::kNoSpace},
                                          {.room = id(14), .kind = AnchorIssueKind::kNoSpace}};
  EXPECT_EQ(names.issues, expected);
}

}  // namespace
}  // namespace otves::geom
