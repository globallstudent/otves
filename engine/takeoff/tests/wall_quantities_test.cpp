#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {
namespace {

using namespace otves::literals;

constexpr double kTolerance = 1e-9;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

geom::WallInput wall(std::int64_t n, PointMm start, PointMm end, Mm thickness) {
  return {.id = id(n),
          .wall = {.start = start, .end = end, .thickness = thickness, .height = 2800_mm}};
}

std::vector<WallQuantities> measure(const std::vector<geom::WallInput>& walls) {
  return measure_walls(walls, geom::join_walls(walls));
}

TEST(WallQuantities, FreeWallMeasuresItsRectangle) {
  const auto quantities = measure({wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}, 380_mm)});

  ASSERT_EQ(quantities.size(), 1U);
  const WallQuantities& q = quantities[0];
  EXPECT_EQ(q.id, id(1));
  EXPECT_EQ(q.thickness, 380_mm);
  EXPECT_NEAR(q.footprint_area, 6.4 * 0.38, kTolerance);
  EXPECT_NEAR(q.length, 6.4, kTolerance);
  EXPECT_NEAR(q.face_area, 6.4 * 2.8, kTolerance);
  EXPECT_NEAR(q.volume, 6.4 * 0.38 * 2.8, kTolerance);
}

TEST(WallQuantities, MitredCornerSplitsBetweenItsWalls) {
  const auto quantities = measure({wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}, 380_mm),
                                   wall(2, {0_mm, 0_mm}, {0_mm, 4000_mm}, 380_mm)});

  ASSERT_EQ(quantities.size(), 2U);
  EXPECT_NEAR(quantities[0].length, 6.4, kTolerance);
  EXPECT_NEAR(quantities[1].length, 4.0, kTolerance);
}

TEST(WallQuantities, ButtingPartitionStopsAtTheFace) {
  const auto quantities = measure({wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}, 380_mm),
                                   wall(2, {3200_mm, 0_mm}, {3200_mm, 4000_mm}, 120_mm)});

  ASSERT_EQ(quantities.size(), 2U);
  EXPECT_NEAR(quantities[0].length, 6.4, kTolerance);
  EXPECT_NEAR(quantities[1].length, 4.0 - 0.19, kTolerance);
  EXPECT_NEAR(quantities[1].volume, (4.0 - 0.19) * 0.12 * 2.8, kTolerance);
}

TEST(WallQuantities, ClosedRoomAddsUpToItsCentreLine) {
  // 6 400 × 4 000 between centre lines, one and a half bricks thick.
  const auto quantities = measure({wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}, 380_mm),
                                   wall(2, {6400_mm, 0_mm}, {6400_mm, 4000_mm}, 380_mm),
                                   wall(3, {6400_mm, 4000_mm}, {0_mm, 4000_mm}, 380_mm),
                                   wall(4, {0_mm, 4000_mm}, {0_mm, 0_mm}, 380_mm)});

  double length = 0.0;
  double volume = 0.0;
  for (const WallQuantities& q : quantities) {
    length += q.length;
    volume += q.volume;
  }
  EXPECT_NEAR(length, 20.8, kTolerance);
  EXPECT_NEAR(volume, 20.8 * 0.38 * 2.8, kTolerance);
}

TEST(WallQuantities, OutputIsSortedByIdWhateverTheInputOrder) {
  const auto quantities = measure({wall(3, {0_mm, 0_mm}, {0_mm, 4000_mm}, 250_mm),
                                   wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}, 250_mm),
                                   wall(2, {6400_mm, 0_mm}, {6400_mm, 4000_mm}, 250_mm)});

  ASSERT_EQ(quantities.size(), 3U);
  EXPECT_EQ(quantities[0].id, id(1));
  EXPECT_EQ(quantities[1].id, id(2));
  EXPECT_EQ(quantities[2].id, id(3));
}

}  // namespace
}  // namespace otves::takeoff
