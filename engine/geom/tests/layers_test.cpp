#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/layers.h>
#include <otves/geom/polygon.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

namespace otves::geom {
namespace {

using namespace otves::literals;

PointMm at(std::int64_t x, std::int64_t y) { return {.x = Mm{x}, .y = Mm{y}}; }

// The outside wall of the reference plans, 420 mm on its centre line.
WallInput wall(std::int64_t n, PointMm start, PointMm end) {
  return {.id = ElementId{Uuid::v7(n, 0, 0)},
          .wall = {.start = start, .end = end, .thickness = 420_mm, .height = 2800_mm}};
}

// Facing brick, insulation and block, from the outer face in.
std::vector<Mm> layers() { return {120_mm, 100_mm, 200_mm}; }

void expect_strip(Strip strip, double from, double to) {
  EXPECT_NEAR(strip.from, from, 1e-12);
  EXPECT_NEAR(strip.to, to, 1e-12);
}

TEST(LayerStrips, RunFromTheRightFaceInAndFromTheLeftWhenMirrored) {
  Wall centred = wall(1, at(0, 0), at(6000, 0)).wall;
  const std::vector<Strip> strips = layer_strips(centred, layers());
  ASSERT_EQ(strips.size(), 3U);
  expect_strip(strips[0], -0.21, -0.09);
  expect_strip(strips[1], -0.09, 0.01);
  expect_strip(strips[2], 0.01, 0.21);

  centred.mirrored = true;
  const std::vector<Strip> mirrored = layer_strips(centred, layers());
  expect_strip(mirrored[0], 0.09, 0.21);
  expect_strip(mirrored[1], -0.01, 0.09);
  expect_strip(mirrored[2], -0.21, -0.01);
}

TEST(LayerStrips, FollowTheLocationLine) {
  Wall by_face = wall(1, at(0, 0), at(6000, 0)).wall;
  by_face.location_line = LocationLine::kRightFace;
  // The points lie on the outer face, so every layer is to the left of them.
  const std::vector<Strip> strips = layer_strips(by_face, layers());
  expect_strip(strips[0], 0.0, 0.12);
  expect_strip(strips[2], 0.22, 0.42);
}

TEST(Clip, GivesEachLayerItsShareOfAMitredCorner) {
  // The second wall turns left at the far end, so the outer face is the longer one.
  const std::vector walls{wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(6000, 4000))};
  const WallJoins joins = join_walls(walls);
  const std::vector<Vec2>& outline = joins.footprints[0].outline;
  const std::vector<Strip> strips = layer_strips(walls[0].wall, layers());

  const std::vector<Vec2> facing = clip(walls[0].wall, outline, strips[0]);
  const std::vector<Vec2> insulation = clip(walls[0].wall, outline, strips[1]);
  const std::vector<Vec2> block = clip(walls[0].wall, outline, strips[2]);

  // Trapezoids between the square end at 0 and the mitre from (6,21; −0,21) to (5,79; 0,21).
  EXPECT_NEAR(signed_area(facing), 0.12 * (6.21 + 6.09) / 2, 1e-9);
  EXPECT_NEAR(signed_area(insulation), 0.10 * (6.09 + 5.99) / 2, 1e-9);
  EXPECT_NEAR(signed_area(block), 0.20 * (5.99 + 5.79) / 2, 1e-9);
  EXPECT_NEAR(signed_area(facing) + signed_area(insulation) + signed_area(block),
              signed_area(outline), 1e-9);
  EXPECT_EQ(facing.size(), 4U);
}

TEST(Clip, AStripBesideTheWallHoldsNothing) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};
  const WallJoins joins = join_walls(walls);

  EXPECT_TRUE(clip(walls[0].wall, joins.footprints[0].outline, {.from = 0.5, .to = 0.7}).empty());
  // The whole wall lies in a strip wider than it.
  EXPECT_NEAR(
      signed_area(clip(walls[0].wall, joins.footprints[0].outline, {.from = -1.0, .to = 1.0})),
      6.0 * 0.42, 1e-9);
}

TEST(Clip, LayersTileAWallBetweenAPartitionAndACorner) {
  // A room corner with a partition standing against the first wall from inside.
  const std::vector walls{wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(6000, 4000)),
                          wall(3, at(0, 4000), at(0, 0)), wall(4, at(3000, 3000), at(3000, 0))};
  const WallJoins joins = join_walls(walls);
  for (std::size_t index = 0; index < 3; ++index) {
    const Wall& one = walls[index].wall;
    double tiled = 0.0;
    for (const Strip strip : layer_strips(one, layers())) {
      tiled += signed_area(clip(one, joins.footprints[index].outline, strip));
    }
    EXPECT_NEAR(tiled, signed_area(joins.footprints[index].outline), 1e-9);
  }
}

}  // namespace
}  // namespace otves::geom
