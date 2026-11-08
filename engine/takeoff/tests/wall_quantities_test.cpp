#include <algorithm>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/error.h>
#include <otves/core/opening.h>
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

// 2ПБ lintels with КМК 2.01.03-19 bearings: 250 mm up to a 1 500 mm opening, 350 mm above.
constexpr LintelSpec kLintels{
    .height = 140_mm, .bearing = 350_mm, .short_bearing = 250_mm, .short_span = 1500_mm};

Measurement measure(const std::vector<geom::WallInput>& walls,
                    const std::vector<OpeningInput>& openings) {
  const auto measurement = measure_walls(walls, openings, kLintels, geom::join_walls(walls));
  EXPECT_TRUE(measurement.has_value());
  return measurement.value_or(Measurement{});
}

std::vector<WallQuantities> measure(const std::vector<geom::WallInput>& walls) {
  return measure(walls, {}).walls;
}

// Window ОК-1: 1 500 × 1 500 on a 900 mm sill, 1 200 mm along wall 1.
OpeningInput window(std::int64_t n, std::int64_t wall_n, Mm offset = 1200_mm, Mm sill = 900_mm,
                    Mm width = 1500_mm, Mm height = 1500_mm) {
  return {.id = id(n),
          .opening = {.wall = id(wall_n),
                      .offset = offset,
                      .width = width,
                      .height = height,
                      .sill = sill,
                      .kind = OpeningKind::kWindow}};
}

bool has_issue(const Measurement& m, std::int64_t n, OpeningIssueKind kind) {
  return std::ranges::contains(m.issues, OpeningIssue{.opening = id(n), .kind = kind});
}

geom::WallInput wall_1() { return wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}, 380_mm); }

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

TEST(WallQuantities, WindowAndItsLintelAreCutFromFaceAndVolume) {
  const Measurement m = measure({wall_1()}, {window(10, 1)});

  ASSERT_EQ(m.walls.size(), 1U);
  const WallQuantities& q = m.walls[0];
  // 1.5 × 1.5 m window, 2.0 × 0.14 m lintel above it.
  const double holes = 2.25 + (2.0 * 0.14);
  EXPECT_NEAR(q.openings_area, 2.25, kTolerance);
  EXPECT_NEAR(q.face_area, (6.4 * 2.8) - holes, kTolerance);
  EXPECT_NEAR(q.volume, (6.4 * 0.38 * 2.8) - (holes * 0.38), kTolerance);
  EXPECT_NEAR(q.footprint_area, 6.4 * 0.38, kTolerance);
  EXPECT_TRUE(m.issues.empty());
}

TEST(WallQuantities, LintelBearingFollowsTheOpeningWidth) {
  const Measurement m =
      measure({wall_1()}, {window(10, 1), window(11, 1, 3500_mm, 900_mm, 1800_mm)});

  ASSERT_EQ(m.lintels.size(), 2U);
  EXPECT_EQ(m.lintels[0].opening, id(10));
  EXPECT_EQ(m.lintels[0].wall, id(1));
  EXPECT_NEAR(m.lintels[0].length, 1.5 + (2 * 0.25), kTolerance);
  EXPECT_NEAR(m.lintels[0].height, 0.14, kTolerance);
  EXPECT_EQ(m.lintels[0].thickness, 380_mm);
  EXPECT_NEAR(m.lintels[1].length, 1.8 + (2 * 0.35), kTolerance);
  EXPECT_TRUE(m.issues.empty());
}

TEST(WallQuantities, OpeningPastTheWallIsClippedAndReported) {
  // 400 mm of the first window fits before the wall's end; the second reaches 600 mm above the
  // top; the third lies wholly past the end and gets no lintel.
  const Measurement m =
      measure({wall_1()},
              {window(10, 1, 6000_mm), window(11, 1, 1200_mm, 1900_mm), window(12, 1, 7000_mm)});

  EXPECT_NEAR(m.walls[0].openings_area, (0.4 * 1.5) + (1.5 * 0.9), kTolerance);
  const std::vector<OpeningIssue> expected{
      {.opening = id(10), .kind = OpeningIssueKind::kOutsideWall},
      {.opening = id(10), .kind = OpeningIssueKind::kShortBearing},
      {.opening = id(11), .kind = OpeningIssueKind::kOutsideWall},
      {.opening = id(11), .kind = OpeningIssueKind::kNoRoomForLintel},
      {.opening = id(12), .kind = OpeningIssueKind::kOutsideWall},
  };
  EXPECT_EQ(m.issues, expected);
  ASSERT_EQ(m.lintels.size(), 1U);
  EXPECT_EQ(m.lintels[0].opening, id(10));
}

TEST(WallQuantities, OpeningAtTheWallEndFitsButItsLintelDoesNot) {
  const Measurement m = measure({wall_1()}, {window(10, 1, 4900_mm)});

  EXPECT_NEAR(m.walls[0].openings_area, 2.25, kTolerance);
  const std::vector<OpeningIssue> expected{
      {.opening = id(10), .kind = OpeningIssueKind::kShortBearing}};
  EXPECT_EQ(m.issues, expected);
  // The lintel is ordered whole; only the part inside the wall is deducted.
  EXPECT_NEAR(m.lintels[0].length, 2.0, kTolerance);
  EXPECT_NEAR(m.walls[0].face_area, (6.4 * 2.8) - 2.25 - (1.75 * 0.14), kTolerance);
}

TEST(WallQuantities, NarrowPierLeavesNoRoomForBothBearings) {
  // A 300 mm pier between two windows: their 250 mm bearings overlap by 200 mm.
  const Measurement m = measure({wall_1()}, {window(10, 1, 1000_mm), window(11, 1, 2800_mm)});

  EXPECT_TRUE(has_issue(m, 10, OpeningIssueKind::kShortBearing));
  EXPECT_TRUE(has_issue(m, 11, OpeningIssueKind::kShortBearing));
  const double holes = (2 * 2.25) + (((2 * 2.0) - 0.2) * 0.14);
  EXPECT_NEAR(m.walls[0].face_area, (6.4 * 2.8) - holes, kTolerance);
}

TEST(WallQuantities, DoorUpToTheWallTopHasNoLintel) {
  const Measurement m = measure({wall_1()}, {window(10, 1, 1200_mm, 0_mm, 900_mm, 2800_mm)});

  EXPECT_TRUE(m.lintels.empty());
  const std::vector<OpeningIssue> expected{
      {.opening = id(10), .kind = OpeningIssueKind::kNoRoomForLintel}};
  EXPECT_EQ(m.issues, expected);
}

TEST(WallQuantities, OverlappingOpeningsAreCutOnce) {
  // The second window starts 500 mm into the first and 300 mm higher.
  const Measurement m =
      measure({wall_1()}, {window(10, 1), window(11, 1, 1700_mm, 1200_mm), window(12, 1, 4500_mm)});

  EXPECT_NEAR(m.walls[0].openings_area, (3 * 2.25) - (1.0 * 1.2), kTolerance);
  EXPECT_TRUE(has_issue(m, 10, OpeningIssueKind::kOverlap));
  EXPECT_TRUE(has_issue(m, 11, OpeningIssueKind::kOverlap));
  EXPECT_FALSE(has_issue(m, 12, OpeningIssueKind::kOverlap));
}

TEST(WallQuantities, OpeningWithoutItsWallIsIgnoredAndReported) {
  const Measurement m = measure({wall_1()}, {window(10, 7)});

  EXPECT_EQ(m.walls[0].openings_area, 0.0);
  EXPECT_TRUE(m.lintels.empty());
  const std::vector<OpeningIssue> expected{{.opening = id(10), .kind = OpeningIssueKind::kNoWall}};
  EXPECT_EQ(m.issues, expected);
}

TEST(WallQuantities, OpeningsFollowTheirOwnWall) {
  const Measurement m = measure({wall(2, {6400_mm, 0_mm}, {6400_mm, 4000_mm}, 380_mm), wall_1()},
                                {window(10, 2), window(11, 1), window(12, 1, 4000_mm)});

  ASSERT_EQ(m.walls.size(), 2U);
  EXPECT_NEAR(m.walls[0].openings_area, 2 * 2.25, kTolerance);
  EXPECT_NEAR(m.walls[1].openings_area, 2.25, kTolerance);
}

TEST(WallQuantities, RejectsInvalidLintelSpec) {
  const std::vector walls{wall_1()};
  LintelSpec flat = kLintels;
  flat.height = 0_mm;
  LintelSpec negative = kLintels;
  negative.short_bearing = -1_mm;

  EXPECT_EQ(measure_walls(walls, {}, flat, geom::join_walls(walls)).error().code,
            ErrorCode::kHeightNotPositive);
  EXPECT_EQ(measure_walls(walls, {}, negative, geom::join_walls(walls)).error().code,
            ErrorCode::kBearingNegative);
}

}  // namespace
}  // namespace otves::takeoff
