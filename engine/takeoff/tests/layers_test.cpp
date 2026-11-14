#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/build.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/core/wall_type.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/layers.h>
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {
namespace {

using namespace otves::literals;

constexpr double kTolerance = 1e-9;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

PointMm at(std::int64_t x, std::int64_t y) { return {.x = Mm{x}, .y = Mm{y}}; }

constexpr MasonryUnit kBrick{.length = 250_mm, .width = 120_mm, .height = 65_mm};
constexpr MasonryUnit kBlock{.length = 390_mm, .width = 190_mm, .height = 188_mm};

BuiltLayer masonry(LayerFunction function, Mm thickness, MasonryUnit unit) {
  return {.function = function,
          .thickness = thickness,
          .unit_length = unit.length,
          .unit_width = unit.width,
          .unit_height = unit.height};
}

// Facing brick, insulation and block from the outer face in: 410 mm.
std::vector<BuiltLayer> outside_layers() {
  return {masonry(LayerFunction::kFacing, 120_mm, kBrick),
          BuiltLayer{.function = LayerFunction::kInsulation,
                     .thickness = 100_mm,
                     .unit_length = {},
                     .unit_width = {},
                     .unit_height = {}},
          masonry(LayerFunction::kStructure, 190_mm, kBlock)};
}

BuiltWall wall(std::int64_t n, PointMm start, PointMm end, std::vector<BuiltLayer> layers) {
  Mm thickness;
  for (const BuiltLayer& layer : layers) {
    thickness += layer.thickness;
  }
  return {.id = id(n),
          .wall = {.start = start, .end = end, .thickness = thickness, .height = 2800_mm},
          .layers = std::move(layers)};
}

// Lintels that take nothing: the tests below read plain faces.
constexpr LintelSpec kLintels{
    .height = 140_mm, .bearing = 250_mm, .short_bearing = 120_mm, .short_span = 1500_mm};

Measurement measure(const std::vector<BuiltWall>& walls,
                    const std::vector<OpeningInput>& openings = {}) {
  std::vector<geom::WallInput> inputs;
  inputs.reserve(walls.size());
  for (const BuiltWall& built : walls) {
    inputs.push_back({.id = built.id, .wall = built.wall});
  }
  const auto measured = measure_levels(inputs, openings, kLintels);
  EXPECT_TRUE(measured.has_value());
  return measured.value_or(Measurement{});
}

TEST(Layers, AWallOfOneMasonryLayerIsMeasuredAsItWas) {
  const std::vector walls{
      wall(1, at(0, 0), at(6000, 0), {masonry(LayerFunction::kStructure, 380_mm, kBrick)})};
  const Measurement measurement = measure(walls);

  const Layers layers = measure_layers(walls, measurement);

  ASSERT_EQ(layers.masonry.size(), 1U);
  EXPECT_EQ(layers.masonry[0].id, id(1));
  EXPECT_EQ(layers.masonry[0].thickness, 380_mm);
  EXPECT_EQ(layers.masonry[0].face_area, measurement.walls[0].face_area);
  EXPECT_EQ(layers.masonry[0].volume, measurement.walls[0].volume);
  EXPECT_EQ(layers.units, (std::vector<std::optional<MasonryUnit>>{kBrick}));
  EXPECT_TRUE(layers.insulation.empty());
}

TEST(Layers, AFreeWallGivesEachLayerItsFace) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0), outside_layers())};

  const Layers layers = measure_layers(walls, measure(walls));

  // 6 m long and 2,8 m high: 16,8 m² for every layer.
  ASSERT_EQ(layers.masonry.size(), 2U);
  EXPECT_EQ(layers.masonry[0].thickness, 120_mm);
  EXPECT_NEAR(layers.masonry[0].face_area, 16.8, kTolerance);
  EXPECT_NEAR(layers.masonry[0].volume, 16.8 * 0.12, kTolerance);
  EXPECT_EQ(layers.masonry[1].thickness, 190_mm);
  EXPECT_NEAR(layers.masonry[1].face_area, 16.8, kTolerance);
  EXPECT_EQ(layers.units, (std::vector<std::optional<MasonryUnit>>{kBrick, kBlock}));
  ASSERT_EQ(layers.insulation.size(), 1U);
  EXPECT_EQ(layers.insulation[0].thickness, 100_mm);
  EXPECT_NEAR(layers.insulation[0].area, 16.8, kTolerance);
}

TEST(Layers, TheOuterLayerOfACornerIsLongerThanTheInnerOne) {
  // Two walls round a corner that turns left: the outer face is on the right of both.
  const std::vector walls{wall(1, at(0, 0), at(6000, 0), outside_layers()),
                          wall(2, at(6000, 0), at(6000, 4000), outside_layers())};

  const Layers layers = measure_layers(walls, measure(walls));

  ASSERT_EQ(layers.masonry.size(), 4U);
  // Wall 1 between its square end and the mitre: the facing's middle line reaches 6 145, the
  // block's 5 890.
  EXPECT_NEAR(layers.masonry[0].length, 6.145, kTolerance);
  EXPECT_NEAR(layers.masonry[1].length, 5.890, kTolerance);
  EXPECT_NEAR(layers.masonry[0].face_area, 6.145 * 2.8, kTolerance);
  // Both walls' insulation, by its middle line: 6 035 and 4 035.
  ASSERT_EQ(layers.insulation.size(), 1U);
  EXPECT_NEAR(layers.insulation[0].area, (6.035 + 4.035) * 2.8, kTolerance);
}

TEST(Layers, AnOpeningCutsEveryLayerAlike) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0), outside_layers())};
  const std::vector<OpeningInput> openings{{.id = id(9),
                                            .opening = {.wall = id(1),
                                                        .offset = 2000_mm,
                                                        .width = 1500_mm,
                                                        .height = 1500_mm,
                                                        .sill = 900_mm,
                                                        .kind = OpeningKind::kWindow}}};
  const Measurement measurement = measure(walls, openings);

  const Layers layers = measure_layers(walls, measurement);

  const double face = measurement.walls[0].face_area;
  EXPECT_LT(face, 16.8 - 2.25);
  EXPECT_NEAR(layers.masonry[0].face_area, face, kTolerance);
  EXPECT_NEAR(layers.masonry[1].face_area, face, kTolerance);
  EXPECT_NEAR(layers.insulation[0].area, face, kTolerance);
}

TEST(Layers, AGapTakesNothingAndAMirroredWallStartsFromItsLeftFace) {
  std::vector<BuiltLayer> ventilated = outside_layers();
  ventilated[1].function = LayerFunction::kGap;
  std::vector walls{wall(1, at(0, 0), at(6000, 0), ventilated),
                    wall(2, at(6000, 0), at(6000, 4000), ventilated)};
  for (BuiltWall& built : walls) {
    built.wall.mirrored = true;
  }

  const Layers layers = measure_layers(walls, measure(walls));

  EXPECT_TRUE(layers.insulation.empty());
  ASSERT_EQ(layers.masonry.size(), 4U);
  // The facing now lies on the inside of the corner, where the wall is shorter.
  EXPECT_NEAR(layers.masonry[0].length, 5.855, kTolerance);
  EXPECT_NEAR(layers.masonry[1].length, 6.110, kTolerance);
}

TEST(Layers, InsulationOfOneThicknessAddsUpAcrossWallsAndOthersStayApart) {
  std::vector<BuiltLayer> thick = outside_layers();
  thick[1].thickness = 150_mm;
  const std::vector walls{wall(1, at(0, 0), at(6000, 0), outside_layers()),
                          wall(2, at(0, 5000), at(4000, 5000), outside_layers()),
                          wall(3, at(0, 9000), at(2000, 9000), thick)};

  const Layers layers = measure_layers(walls, measure(walls));

  ASSERT_EQ(layers.insulation.size(), 2U);
  EXPECT_EQ(layers.insulation[0].thickness, 100_mm);
  EXPECT_NEAR(layers.insulation[0].area, (6.0 + 4.0) * 2.8, kTolerance);
  EXPECT_EQ(layers.insulation[1].thickness, 150_mm);
  EXPECT_NEAR(layers.insulation[1].area, 2.0 * 2.8, kTolerance);
}

}  // namespace
}  // namespace otves::takeoff
