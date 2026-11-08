#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {
namespace {

using namespace otves::literals;

constexpr double kTolerance = 1e-9;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

// Single brick 1НФ (ГОСТ 530-2012) with SP 70.13330.2012 9.2.4 joints: 12 mm bed, 10 mm head.
constexpr MasonrySpec kBrick1nf{.unit = {.length = 250_mm, .width = 120_mm, .height = 65_mm},
                                .joints = {.bed = 12_mm, .head = 10_mm}};

// One square metre of face.
WallQuantities square_metre(std::int64_t n, Mm thickness) {
  return {.id = id(n),
          .thickness = thickness,
          .footprint_area = to_metres(thickness),
          .length = 1.0,
          .face_area = 1.0,
          .volume = to_metres(thickness)};
}

Masonry count(const std::vector<WallQuantities>& walls, const MasonrySpec& spec = kBrick1nf) {
  const auto masonry = count_masonry(walls, spec);
  EXPECT_TRUE(masonry.has_value());
  return masonry.value_or(Masonry{});
}

TEST(Masonry, MatchesTheBrickTablePerSquareMetre) {
  // docs/research: 1НФ, 12/10 mm joints, units and net mortar per m² of wall face.
  struct Row {
    Mm thickness;
    double units;
    double mortar;
  };
  const std::vector<Row> table{{120_mm, 50, 0.023},
                               {250_mm, 100, 0.055},
                               {380_mm, 150, 0.088},
                               {510_mm, 200, 0.120},
                               {640_mm, 250, 0.153}};

  for (const Row& row : table) {
    const Masonry masonry = count({square_metre(1, row.thickness)});
    ASSERT_EQ(masonry.walls.size(), 1U);
    EXPECT_NEAR(masonry.walls[0].units, row.units, 0.3) << row.thickness.value() << " mm";
    EXPECT_NEAR(masonry.walls[0].mortar, row.mortar, 0.0005) << row.thickness.value() << " mm";
    EXPECT_TRUE(masonry.issues.empty());
  }
}

TEST(Masonry, UnitsFollowTheFaceCell) {
  const Masonry masonry = count({square_metre(1, 250_mm)});

  // Two wythes of 0.26 × 0.077 m cells.
  EXPECT_NEAR(masonry.walls[0].units, 2.0 / (0.26 * 0.077), kTolerance);
  EXPECT_NEAR(masonry.walls[0].mortar, 0.25 - (2.0 / (0.26 * 0.077) * 0.25 * 0.12 * 0.065),
              kTolerance);
}

TEST(Masonry, ThickenedBrickAndBlocks) {
  const MasonrySpec brick_1_4nf{.unit = {.length = 250_mm, .width = 120_mm, .height = 88_mm},
                                .joints = kBrick1nf.joints};
  const MasonrySpec glued_block{.unit = {.length = 625_mm, .width = 300_mm, .height = 250_mm},
                                .joints = {.bed = 0_mm, .head = 0_mm}};
  const MasonrySpec concrete_stone{.unit = {.length = 390_mm, .width = 190_mm, .height = 188_mm},
                                   .joints = {.bed = 12_mm, .head = 10_mm}};

  EXPECT_NEAR(count({square_metre(1, 250_mm)}, brick_1_4nf).units, 76.9, 0.05);
  const Masonry block = count({square_metre(1, 300_mm)}, glued_block);
  EXPECT_NEAR(block.units, 6.4, kTolerance);
  EXPECT_NEAR(block.mortar, 0.0, kTolerance);
  EXPECT_NEAR(count({square_metre(1, 190_mm)}, concrete_stone).units, 12.5, kTolerance);
}

TEST(Masonry, ThicknessBetweenWythesIsCountedProRataAndReported) {
  const Masonry masonry = count({square_metre(1, 300_mm)});

  EXPECT_NEAR(masonry.walls[0].units, (310.0 / 130.0) / (0.26 * 0.077), kTolerance);
  EXPECT_GT(masonry.walls[0].mortar, 0.0);
  ASSERT_EQ(masonry.issues.size(), 1U);
  EXPECT_EQ(masonry.issues[0],
            (MasonryIssue{.wall = id(1), .kind = MasonryIssueKind::kPartialWythe}));
}

TEST(Masonry, WallThinnerThanAUnitGetsNothing) {
  const Masonry masonry = count({square_metre(1, 100_mm), square_metre(2, 250_mm)});

  ASSERT_EQ(masonry.walls.size(), 2U);
  EXPECT_EQ(masonry.walls[0].units, 0.0);
  EXPECT_EQ(masonry.walls[0].mortar, 0.0);
  EXPECT_EQ(masonry.units, masonry.walls[1].units);
  ASSERT_EQ(masonry.issues.size(), 1U);
  EXPECT_EQ(masonry.issues[0].kind, MasonryIssueKind::kThinnerThanUnit);
}

TEST(Masonry, TotalsAddUpAndIssuesAreSorted) {
  const Masonry masonry =
      count({square_metre(3, 300_mm), square_metre(1, 380_mm), square_metre(2, 100_mm)});

  EXPECT_EQ(masonry.units,
            masonry.walls[0].units + masonry.walls[1].units + masonry.walls[2].units);
  EXPECT_EQ(masonry.mortar,
            masonry.walls[0].mortar + masonry.walls[1].mortar + masonry.walls[2].mortar);
  ASSERT_EQ(masonry.issues.size(), 2U);
  EXPECT_EQ(masonry.issues[0].wall, id(2));
  EXPECT_EQ(masonry.issues[1].wall, id(3));
}

TEST(Masonry, RejectsInvalidSpec) {
  MasonrySpec flat = kBrick1nf;
  flat.unit.height = 0_mm;
  MasonrySpec negative = kBrick1nf;
  negative.joints.head = -1_mm;

  const std::vector walls{square_metre(1, 250_mm)};
  EXPECT_EQ(count_masonry(walls, flat).error().code, ErrorCode::kUnitSizeNotPositive);
  EXPECT_EQ(count_masonry(walls, negative).error().code, ErrorCode::kJointNegative);
}

TEST(Masonry, ClosedRoomFromJoinedWalls) {
  // 6 400 × 4 000 between centre lines, one and a half bricks thick, 2 800 high.
  const auto wall = [](std::int64_t n, PointMm start, PointMm end) {
    return geom::WallInput{
        .id = id(n), .wall = {.start = start, .end = end, .thickness = 380_mm, .height = 2800_mm}};
  };
  const std::vector walls{
      wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}), wall(2, {6400_mm, 0_mm}, {6400_mm, 4000_mm}),
      wall(3, {6400_mm, 4000_mm}, {0_mm, 4000_mm}), wall(4, {0_mm, 4000_mm}, {0_mm, 0_mm})};

  const LintelSpec lintels{
      .height = 140_mm, .bearing = 350_mm, .short_bearing = 250_mm, .short_span = 1500_mm};
  const Masonry masonry = count(measure_walls(walls, {}, lintels, geom::join_walls(walls))->walls);

  const double face = 20.8 * 2.8;
  EXPECT_NEAR(masonry.units, face * 3.0 / (0.26 * 0.077), 1e-6);
  EXPECT_NEAR(masonry.mortar + (masonry.units * 0.25 * 0.12 * 0.065), face * 0.38, 1e-9);
}

}  // namespace
}  // namespace otves::takeoff
