#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/polygon.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

namespace otves::geom {
namespace {

using namespace otves::literals;

constexpr double kTolerance = 1e-9;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

WallInput wall(std::int64_t n, PointMm start, PointMm end, Mm thickness,
               LocationLine location_line = LocationLine::kCenter) {
  return {.id = id(n),
          .wall = {.start = start,
                   .end = end,
                   .thickness = thickness,
                   .height = 2800_mm,
                   .location_line = location_line}};
}

const WallFootprint& footprint(const WallJoins& joins, std::int64_t n) {
  const auto found = std::ranges::find(joins.footprints, id(n), &WallFootprint::id);
  EXPECT_NE(found, joins.footprints.end());
  return *found;
}

void expect_outline(const WallJoins& joins, std::int64_t n, const std::vector<Vec2>& expected) {
  const std::vector<Vec2>& actual = footprint(joins, n).outline;
  ASSERT_EQ(actual.size(), expected.size());
  for (std::size_t i = 0; i < actual.size(); ++i) {
    EXPECT_NEAR(actual[i].x, expected[i].x, kTolerance) << "wall " << n << ", point " << i;
    EXPECT_NEAR(actual[i].y, expected[i].y, kTolerance) << "wall " << n << ", point " << i;
  }
}

double total_area(const WallJoins& joins) {
  double total = 0.0;
  for (const WallFootprint& footprint : joins.footprints) {
    total += signed_area(footprint.outline);
  }
  return total;
}

std::vector<ElementId> ids(const WallJoins& joins) {
  std::vector<ElementId> values;
  values.reserve(joins.footprints.size());
  for (const WallFootprint& footprint : joins.footprints) {
    values.push_back(footprint.id);
  }
  return values;
}

// Every coordinate in output order, for bit-exact comparison.
std::vector<double> coordinates(const WallJoins& joins) {
  std::vector<double> values;
  for (const WallFootprint& footprint : joins.footprints) {
    for (const Vec2 point : footprint.outline) {
      values.push_back(point.x);
      values.push_back(point.y);
    }
  }
  return values;
}

TEST(WallJoins, FreeWallIsARectangleAroundItsCentreLine) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}, 380_mm)};
  const WallJoins joins = join_walls(walls);

  expect_outline(joins, 1, {{0, -0.19}, {6.4, -0.19}, {6.4, 0.19}, {0, 0.19}});
  EXPECT_NEAR(total_area(joins), 6.4 * 0.38, kTolerance);
  EXPECT_TRUE(joins.issues.empty());
}

TEST(WallJoins, LeftFaceLocationKeepsTheBodyOnTheRight) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}, 380_mm, LocationLine::kLeftFace)};
  expect_outline(join_walls(walls), 1, {{0, -0.38}, {6.4, -0.38}, {6.4, 0}, {0, 0}});
}

TEST(WallJoins, CornerIsMitred) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {4000_mm, 0_mm}, 380_mm),
                          wall(2, {4000_mm, 0_mm}, {4000_mm, 3000_mm}, 380_mm)};
  const WallJoins joins = join_walls(walls);

  expect_outline(joins, 1, {{0, -0.19}, {4.19, -0.19}, {3.81, 0.19}, {0, 0.19}});
  expect_outline(joins, 2, {{4.19, -0.19}, {4.19, 3}, {3.81, 3}, {3.81, 0.19}});
  EXPECT_NEAR(total_area(joins), (4.0 + 3.0) * 0.38, kTolerance);
}

TEST(WallJoins, LoopDrawnAlongOuterFacesFillsOuterMinusInner) {
  // Counter-clockwise, so the right face of every wall is the outside of the house.
  const std::vector walls{
      wall(1, {0_mm, 0_mm}, {10000_mm, 0_mm}, 380_mm, LocationLine::kRightFace),
      wall(2, {10000_mm, 0_mm}, {10000_mm, 8000_mm}, 380_mm, LocationLine::kRightFace),
      wall(3, {10000_mm, 8000_mm}, {0_mm, 8000_mm}, 380_mm, LocationLine::kRightFace),
      wall(4, {0_mm, 8000_mm}, {0_mm, 0_mm}, 380_mm, LocationLine::kRightFace)};
  const WallJoins joins = join_walls(walls);

  expect_outline(joins, 1, {{0, 0}, {10, 0}, {9.62, 0.38}, {0.38, 0.38}});
  for (const WallFootprint& footprint : joins.footprints) {
    EXPECT_GT(signed_area(footprint.outline), 0.0);
  }
  EXPECT_NEAR(total_area(joins), (10.0 * 8.0) - (9.24 * 7.24), kTolerance);
  EXPECT_TRUE(joins.issues.empty());
}

TEST(WallJoins, PartitionIsCutBackToTheFaceItMeets) {
  const WallInput outer = wall(1, {0_mm, 0_mm}, {10000_mm, 0_mm}, 380_mm);

  // From −100 mm the partition already passes the centre line; it still stops at the face.
  for (const Mm from : {-100_mm, 0_mm, 100_mm, 190_mm}) {
    const std::vector walls{outer, wall(2, {5000_mm, from}, {5000_mm, 4000_mm}, 120_mm)};
    const WallJoins joins = join_walls(walls);

    expect_outline(joins, 1, {{0, -0.19}, {10, -0.19}, {10, 0.19}, {0, 0.19}});
    expect_outline(joins, 2, {{5.06, 0.19}, {5.06, 4}, {4.94, 4}, {4.94, 0.19}});
    EXPECT_TRUE(joins.issues.empty());
  }
}

TEST(WallJoins, PartitionBelowIsCutBackToTheLowerFace) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {10000_mm, 0_mm}, 380_mm),
                          wall(2, {5000_mm, 0_mm}, {5000_mm, -3000_mm}, 120_mm)};
  expect_outline(join_walls(walls), 2, {{4.94, -0.19}, {4.94, -3}, {5.06, -3}, {5.06, -0.19}});
}

TEST(WallJoins, PartitionsMeetingInsideAWallAreCutBackToBothFaces) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {10000_mm, 0_mm}, 380_mm),
                          wall(2, {5000_mm, 0_mm}, {5000_mm, 3000_mm}, 120_mm),
                          wall(3, {5000_mm, 0_mm}, {5000_mm, -3000_mm}, 120_mm)};
  const WallJoins joins = join_walls(walls);

  expect_outline(joins, 2, {{5.06, 0.19}, {5.06, 3}, {4.94, 3}, {4.94, 0.19}});
  expect_outline(joins, 3, {{4.94, -0.19}, {4.94, -3}, {5.06, -3}, {5.06, -0.19}});
  EXPECT_NEAR(total_area(joins), (10.0 * 0.38) + (2 * 2.81 * 0.12), kTolerance);
  EXPECT_TRUE(joins.issues.empty());
}

TEST(WallJoins, TwoEndsInsideAWallFromOneSideOverlap) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {10000_mm, 0_mm}, 380_mm),
                          wall(2, {5000_mm, 0_mm}, {5000_mm, 3000_mm}, 120_mm),
                          wall(3, {5000_mm, 0_mm}, {8000_mm, 3000_mm}, 120_mm)};
  const std::vector<JoinIssue> expected{{.at = {5000_mm, 0_mm}, .kind = JoinIssueKind::kOverlap}};
  EXPECT_EQ(join_walls(walls).issues, expected);
}

TEST(WallJoins, CrossingWallsOverlap) {
  const WallInput outer = wall(1, {0_mm, 0_mm}, {10000_mm, 0_mm}, 380_mm);
  const std::vector<JoinIssue> expected{{.at = {5000_mm, 0_mm}, .kind = JoinIssueKind::kOverlap}};

  const std::vector cross{outer, wall(2, {5000_mm, -3000_mm}, {5000_mm, 3000_mm}, 380_mm)};
  EXPECT_EQ(join_walls(cross).issues, expected);
  // The lower end sticks out 310 mm past the face.
  const std::vector through{outer, wall(2, {5000_mm, -500_mm}, {5000_mm, 3000_mm}, 120_mm)};
  EXPECT_EQ(join_walls(through).issues, expected);
}

TEST(WallJoins, ThreeWallsAtOnePointTileItWithoutGapsOrOverlaps) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {-4000_mm, 0_mm}, 380_mm),
                          wall(2, {0_mm, 0_mm}, {0_mm, 3000_mm}, 380_mm),
                          wall(3, {0_mm, 0_mm}, {4000_mm, 0_mm}, 380_mm)};
  const WallJoins joins = join_walls(walls);

  expect_outline(joins, 2, {{0.19, 0.19}, {0.19, 3}, {-0.19, 3}, {-0.19, 0.19}, {0, 0}});
  // Union: the 8 m run plus the branch from the run's face to its end.
  EXPECT_NEAR(total_area(joins), (8.0 * 0.38) + (2.81 * 0.38), kTolerance);
  EXPECT_TRUE(joins.issues.empty());
}

TEST(WallJoins, StraightRunsOfDifferentThicknessMeetSquare) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {3000_mm, 0_mm}, 510_mm),
                          wall(2, {3000_mm, 0_mm}, {6000_mm, 0_mm}, 380_mm)};
  const WallJoins joins = join_walls(walls);

  expect_outline(joins, 1, {{0, -0.255}, {3, -0.255}, {3, 0.255}, {0, 0.255}});
  expect_outline(joins, 2, {{3, -0.19}, {6, -0.19}, {6, 0.19}, {3, 0.19}});
}

TEST(WallJoins, ResultDoesNotDependOnInputOrder) {
  std::vector walls{wall(1, {0_mm, 0_mm}, {10000_mm, 0_mm}, 380_mm),
                    wall(2, {10000_mm, 0_mm}, {10000_mm, 8000_mm}, 380_mm),
                    wall(3, {10000_mm, 8000_mm}, {0_mm, 8000_mm}, 380_mm),
                    wall(4, {0_mm, 8000_mm}, {0_mm, 0_mm}, 380_mm),
                    wall(5, {4000_mm, 0_mm}, {4000_mm, 8000_mm}, 120_mm)};
  const WallJoins forward = join_walls(walls);
  std::ranges::reverse(walls);
  const WallJoins backward = join_walls(walls);

  EXPECT_EQ(ids(forward), ids(backward));
  EXPECT_EQ(coordinates(forward), coordinates(backward));
  EXPECT_EQ(forward.issues, backward.issues);
}

// Bits written on arm64; CI checks them on x86-64. A fused multiply-add changes the last bits.
TEST(WallJoins, ObliqueJoinsGiveTheSameBitsOnEveryPlatform) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {4000_mm, 1000_mm}, 380_mm),
                          wall(2, {4000_mm, 1000_mm}, {3000_mm, 4000_mm}, 250_mm),
                          wall(3, {3000_mm, 4000_mm}, {0_mm, 0_mm}, 120_mm)};
  const std::vector expected{
      -0x1.17a4e8bc7d32fp-2, -0x1.0e757a94eb32cp-2, 0x1.0ba408b19accdp+2,  0x1.b301f1304fa69p-1,
      0x1.e8b7ee9cca665p+1,  0x1.267f0767d82cbp+0,  0x1.17a4e8bc7d32ep-2,  0x1.0e757a94eb32dp-2,
      0x1.0ba408b19accdp+2,  0x1.b301f1304fa69p-1,  0x1.88b8e5c48449bp+1,  0x1.0c36ff9413ecdp+2,
      0x1.77471a3b7bb65p+1,  0x1.e79200d7d8265p+1,  0x1.e8b7ee9cca665p+1,  0x1.267f0767d82cbp+0,
      0x1.88b8e5c48449bp+1,  0x1.0c36ff9413ecdp+2,  -0x1.17a4e8bc7d32fp-2, -0x1.0e757a94eb32cp-2,
      0x1.17a4e8bc7d32ep-2,  0x1.0e757a94eb32dp-2,  0x1.77471a3b7bb65p+1,  0x1.e79200d7d8265p+1};
  EXPECT_EQ(coordinates(join_walls(walls)), expected);
}

TEST(WallJoins, WallsLeavingAPointTogetherOverlap) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {4000_mm, 0_mm}, 380_mm),
                          wall(2, {0_mm, 0_mm}, {2000_mm, 0_mm}, 380_mm)};
  const WallJoins joins = join_walls(walls);

  const std::vector<JoinIssue> expected{{.at = {0_mm, 0_mm}, .kind = JoinIssueKind::kOverlap},
                                        {.at = {2000_mm, 0_mm}, .kind = JoinIssueKind::kOverlap}};
  EXPECT_EQ(joins.issues, expected);
}

TEST(WallJoins, WallInsideAThickerWallOverlaps) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {10000_mm, 0_mm}, 640_mm),
                          wall(2, {5000_mm, 0_mm}, {5000_mm, 100_mm}, 120_mm)};
  const WallJoins joins = join_walls(walls);

  const std::vector<JoinIssue> expected{{.at = {5000_mm, 0_mm}, .kind = JoinIssueKind::kOverlap},
                                        {.at = {5000_mm, 100_mm}, .kind = JoinIssueKind::kOverlap}};
  EXPECT_EQ(joins.issues, expected);
  expect_outline(joins, 2, {{5.06, 0}, {5.06, 0.1}, {4.94, 0.1}, {4.94, 0}});
}

TEST(WallJoins, NearlyParallelWallsAreCutSquare) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {5000_mm, 0_mm}, 380_mm),
                          wall(2, {0_mm, 0_mm}, {5000_mm, 10_mm}, 380_mm)};
  const WallJoins joins = join_walls(walls);

  ASSERT_FALSE(joins.issues.empty());
  EXPECT_EQ(joins.issues.front(),
            (JoinIssue{.at = {0_mm, 0_mm}, .kind = JoinIssueKind::kSharpAngle}));
  expect_outline(joins, 1, {{0, -0.19}, {5, -0.19}, {5, 0.19}, {0, 0.19}});
}

TEST(WallJoins, JunctionsListWhereWallsMeet) {
  // A room with a partition butting into its bottom wall, and a free wall elsewhere.
  const std::vector walls{wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}, 380_mm),
                          wall(2, {6400_mm, 0_mm}, {6400_mm, 4000_mm}, 380_mm),
                          wall(3, {6400_mm, 4000_mm}, {0_mm, 4000_mm}, 380_mm),
                          wall(4, {0_mm, 4000_mm}, {0_mm, 0_mm}, 380_mm),
                          wall(5, {3200_mm, 0_mm}, {3200_mm, 2000_mm}, 120_mm),
                          wall(6, {10000_mm, 0_mm}, {12000_mm, 0_mm}, 250_mm)};

  const WallJoins joins = join_walls(walls);

  const std::vector<Junction> expected{
      {.at = {0_mm, 0_mm}, .walls = {id(1), id(4)}},
      {.at = {0_mm, 4000_mm}, .walls = {id(3), id(4)}},
      {.at = {3200_mm, 0_mm}, .walls = {id(1), id(5)}},
      {.at = {6400_mm, 0_mm}, .walls = {id(1), id(2)}},
      {.at = {6400_mm, 4000_mm}, .walls = {id(2), id(3)}},
  };
  EXPECT_EQ(joins.junctions, expected);
}

TEST(WallJoins, ThreeWallsAtOnePointFormOneJunction) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {4000_mm, 0_mm}, 380_mm),
                          wall(2, {0_mm, 0_mm}, {-4000_mm, 0_mm}, 380_mm),
                          wall(3, {0_mm, 0_mm}, {0_mm, 3000_mm}, 250_mm)};

  const WallJoins joins = join_walls(walls);

  ASSERT_EQ(joins.junctions.size(), 1U);
  EXPECT_EQ(joins.junctions[0].walls, (std::vector{id(1), id(2), id(3)}));
}

TEST(WallJoins, StraightRunAndOverlapAreNotJunctions) {
  const std::vector straight{wall(1, {0_mm, 0_mm}, {4000_mm, 0_mm}, 380_mm),
                             wall(2, {4000_mm, 0_mm}, {8000_mm, 0_mm}, 250_mm)};
  const std::vector overlap{wall(1, {0_mm, 0_mm}, {4000_mm, 0_mm}, 380_mm),
                            wall(2, {0_mm, 0_mm}, {2000_mm, 0_mm}, 380_mm)};

  EXPECT_TRUE(join_walls(straight).junctions.empty());
  EXPECT_TRUE(join_walls(overlap).junctions.empty());
}

}  // namespace
}  // namespace otves::geom
