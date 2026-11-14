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
// the bottom wall, the right, the top and the left. 5.6 by 3.6 m clear.
std::vector<WallInput> box() {
  return {wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(6000, 4000)),
          wall(3, at(6000, 4000), at(0, 4000)), wall(4, at(0, 4000), at(0, 0))};
}

// Where the wall goes, or nothing when it is refused.
std::optional<WallLine> crossing(const std::vector<WallInput>& walls, const Across& asked) {
  const Result<WallLine> found = across(walls, asked);
  return found ? std::optional{*found} : std::nullopt;
}

// Why the wall is refused, or nothing when it is not.
std::optional<ErrorCode> refusal(const std::vector<WallInput>& walls, const Across& asked) {
  const Result<WallLine> found = across(walls, asked);
  return found ? std::nullopt : std::optional{found.error().code};
}

TEST(Across, AWallRunsFromWallToWallAtTheDistanceFromTheWallNamed) {
  const std::vector<WallInput> walls = box();

  // 1.5 m under the top wall's face at 3.8 m, from the right wall's line to the left wall's.
  const Across asked{.in = at(3000, 2000), .from = Hand::kTop, .distance = 1500_mm};
  const WallLine under_top{
      .start = at(6000, 2300), .end = at(0, 2300), .line = LocationLine::kRightFace};
  EXPECT_EQ(crossing(walls, asked), under_top);

  // 2 m from the left wall's face at 0.2 m, from the top wall down to the bottom one.
  EXPECT_EQ(
      crossing(walls, {.in = at(3000, 2000), .from = Hand::kLeft, .distance = 2000_mm}),
      (WallLine{.start = at(2200, 4000), .end = at(2200, 0), .line = LocationLine::kRightFace}));

  // The order of the walls does not matter.
  std::vector<WallInput> reversed = walls;
  std::ranges::reverse(reversed);
  EXPECT_EQ(crossing(reversed, asked), under_top);
}

TEST(Across, TheWallJoinsTheWallsItMeetsAndLeavesTheDistanceClear) {
  std::vector<WallInput> walls = box();
  const std::optional<WallLine> found =
      crossing(walls, {.in = at(3000, 2000), .from = Hand::kTop, .distance = 1500_mm});
  ASSERT_TRUE(found.has_value());
  const WallLine placed = found.value_or(WallLine{});
  WallInput partition = wall(5, placed.start, placed.end, 120_mm);
  partition.wall.location_line = placed.line;
  walls.push_back(partition);

  // Two rooms: the one the distance was typed for is 1.5 m deep whatever the partition is made
  // of, and the other has what is left of 3.6 m less the partition.
  std::vector<double> areas;
  for (const Space& space : find_spaces(walls)) {
    areas.push_back(space.area);
  }
  std::ranges::sort(areas);
  ASSERT_EQ(areas.size(), 2U);
  EXPECT_NEAR(areas[0], 5.6 * 1.5, 1e-9);
  EXPECT_NEAR(areas[1], 5.6 * (3.6 - 1.5 - 0.12), 1e-9);
}

TEST(Across, InARoomWithACornerCutOutTheWallCrossesThePartBesideThePoint) {
  // An L: 6 by 6 m with its upper right quarter cut out.
  const std::vector<WallInput> walls{
      wall(1, at(0, 0), at(6000, 0)),          wall(2, at(6000, 0), at(6000, 3000)),
      wall(3, at(6000, 3000), at(3000, 3000)), wall(4, at(3000, 3000), at(3000, 6000)),
      wall(5, at(3000, 6000), at(0, 6000)),    wall(6, at(0, 6000), at(0, 0))};

  // 4 m over the bottom wall's face the room is only its left arm wide.
  const WallLine arm{.start = at(0, 4200), .end = at(3000, 4200), .line = LocationLine::kRightFace};
  EXPECT_EQ(crossing(walls, {.in = at(1500, 4500), .from = Hand::kBottom, .distance = 4000_mm}),
            arm);
  // A point that the new wall does not pass beside gets the longest stretch there is.
  EXPECT_EQ(crossing(walls, {.in = at(4500, 1500), .from = Hand::kBottom, .distance = 4000_mm}),
            arm);
}

TEST(Across, AWallThatCannotStandIsRefused) {
  std::vector<WallInput> walls = box();

  EXPECT_EQ(refusal(walls, {.in = at(9000, 2000), .from = Hand::kTop, .distance = 1500_mm}),
            ErrorCode::kNotFound);
  EXPECT_EQ(refusal(walls, {.in = at(3000, 2000), .from = Hand::kTop, .distance = 0_mm}),
            ErrorCode::kDoesNotFit);
  // The room is 3.6 m deep.
  EXPECT_EQ(refusal(walls, {.in = at(3000, 2000), .from = Hand::kTop, .distance = 3600_mm}),
            ErrorCode::kDoesNotFit);

  // A wall that stands free in the room is no wall to measure from or to stand against.
  walls.push_back(wall(9, at(3000, 1500), at(3000, 2500)));
  EXPECT_EQ(refusal(walls, {.in = at(1500, 2000), .from = Hand::kRight, .distance = 500_mm}),
            ErrorCode::kDoesNotFit);
  EXPECT_EQ(refusal(walls, {.in = at(1500, 2000), .from = Hand::kTop, .distance = 1800_mm}),
            ErrorCode::kDoesNotFit);
  // Clear of it, a wall still fits.
  EXPECT_TRUE(
      crossing(walls, {.in = at(1500, 2000), .from = Hand::kTop, .distance = 500_mm}).has_value());
}

}  // namespace
}  // namespace otves::geom
