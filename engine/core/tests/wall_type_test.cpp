#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/build.h>
#include <otves/core/command.h>
#include <otves/core/document.h>
#include <otves/core/error.h>
#include <otves/core/level.h>
#include <otves/core/op.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/core/wall_type.h>

namespace otves {
namespace {

using namespace literals;

DocumentEnvironment test_environment() {
  const auto now = std::make_shared<std::int64_t>(1'762'387'200'000);
  const auto seed = std::make_shared<std::uint64_t>(0);
  return DocumentEnvironment{
      .device = DeviceId{Uuid{1, 1}},
      .unix_ms = [now] { return (*now)++; },
      .random_u64 = [seed] { return ++*seed * 0x9E37'79B9'7F4A'7C15ULL; },
  };
}

ElementId made(const Result<std::optional<ElementId>>& result) {
  EXPECT_TRUE(result.has_value() && result->has_value());
  return result.value_or(std::nullopt).value_or(ElementId{});
}

ElementId add_level(Document& document) {
  return made(document.execute(AddLevel{.level = {.elevation = 0_mm, .height = 3000_mm}}));
}

// 1НФ brick laid flat, as a facing wythe or in a wall.
WallLayer brick(LayerFunction function, Mm thickness) {
  return {.type = {},
          .function = function,
          .thickness = thickness,
          .unit_length = 250_mm,
          .unit_width = 120_mm,
          .unit_height = 65_mm};
}

WallLayer wool(Mm thickness) {
  return {.type = {}, .function = LayerFunction::kInsulation, .thickness = thickness};
}

// The outside wall of the reference plans: 120 of facing brick, 100 of insulation, 250 of brick.
AddWallType outside_wall() {
  return {.role = WallRole::kExterior,
          .layers = {brick(LayerFunction::kFacing, 120_mm), wool(100_mm),
                     brick(LayerFunction::kStructure, 250_mm)}};
}

// A wall of a type: it has no thickness of its own.
Wall wall_of(ElementId level, ElementId type, Mm length = 6000_mm) {
  return {.level = level,
          .start = {.x = 0_mm, .y = 0_mm},
          .end = {.x = length, .y = 0_mm},
          .thickness = 0_mm,
          .height = 2800_mm,
          .type = type};
}

Wall plain_wall(ElementId level) {
  return {.level = level,
          .start = {.x = 0_mm, .y = 0_mm},
          .end = {.x = 6000_mm, .y = 0_mm},
          .thickness = 380_mm,
          .height = 2800_mm,
          .unit_length = 250_mm,
          .unit_width = 120_mm,
          .unit_height = 65_mm};
}

std::vector<ElementId> layer_ids(const Document& document) {
  std::vector<ElementId> ids;
  document.model().for_each<WallLayer>(
      [&ids](ElementId id, const WallLayer&) { ids.push_back(id); });
  return ids;
}

TEST(WallType, IsItsRoleAndItsLayersFromTheOuterFaceIn) {
  Document document{test_environment()};
  const ElementId level = add_level(document);

  const ElementId type = made(document.execute(outside_wall()));

  const OpGroup& group = document.log().back();
  EXPECT_EQ(group.kind, CommandKind::kAddWallType);
  EXPECT_EQ(group.ops.size(), 4U);
  const ElementId wall = made(document.execute(AddWall{.wall = wall_of(level, type)}));
  const Build build = build_of(document.model(), *document.model().find<Wall>(wall));
  EXPECT_EQ(build.role, WallRole::kExterior);
  ASSERT_EQ(build.layers.size(), 3U);
  EXPECT_EQ(build.layers[0].function, LayerFunction::kFacing);
  EXPECT_EQ(build.layers[1].function, LayerFunction::kInsulation);
  EXPECT_FALSE(build.layers[1].has_unit());
  EXPECT_EQ(build.layers[2].thickness, 250_mm);
  EXPECT_EQ(build.thickness(), 470_mm);

  // One step back takes the wall, the next one the type with its layers.
  ASSERT_TRUE(document.undo());
  ASSERT_TRUE(document.undo());
  EXPECT_EQ(document.model().count<WallType>(), 0U);
  EXPECT_EQ(document.model().count<WallLayer>(), 0U);
}

TEST(WallType, AWallWithNoTypeIsOneLayerOfItsOwn) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  const ElementId wall = made(document.execute(AddWall{.wall = plain_wall(level)}));

  const Build build = build_of(document.model(), *document.model().find<Wall>(wall));

  EXPECT_EQ(build.role, std::nullopt);
  ASSERT_EQ(build.layers.size(), 1U);
  EXPECT_EQ(build.layers[0], (BuiltLayer{.function = LayerFunction::kStructure,
                                         .thickness = 380_mm,
                                         .unit_length = 250_mm,
                                         .unit_width = 120_mm,
                                         .unit_height = 65_mm}));
  EXPECT_EQ(build.thickness(), 380_mm);
}

TEST(WallType, ALayerEditedOnceChangesEveryWallOfItsType) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  const ElementId type = made(document.execute(outside_wall()));
  const ElementId first = made(document.execute(AddWall{.wall = wall_of(level, type)}));
  const ElementId second = made(document.execute(AddWall{.wall = wall_of(level, type, 4000_mm)}));
  const ElementId insulation = layer_ids(document)[1];

  ASSERT_TRUE(document.execute(SetWallLayer{.layer = insulation,
                                            .thickness = 150_mm,
                                            .unit_length = 0_mm,
                                            .unit_width = 0_mm,
                                            .unit_height = 0_mm}));

  EXPECT_EQ(document.log().back().kind, CommandKind::kSetWallLayer);
  EXPECT_EQ(document.log().back().ops.size(), 1U);
  for (const ElementId wall : {first, second}) {
    EXPECT_EQ(build_of(document.model(), *document.model().find<Wall>(wall)).thickness(), 520_mm);
  }
  ASSERT_TRUE(document.execute(SetWallTypeRole{.type = type, .role = WallRole::kBearing}));
  EXPECT_EQ(build_of(document.model(), *document.model().find<Wall>(first)).role,
            WallRole::kBearing);
}

TEST(WallType, AWallOfATypeHasNoThicknessOrUnitOfItsOwn) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  const ElementId type = made(document.execute(outside_wall()));

  Wall thick = wall_of(level, type);
  thick.thickness = 380_mm;
  const auto refused = document.execute(AddWall{.wall = thick});
  ASSERT_FALSE(refused.has_value());
  EXPECT_EQ(refused.error().code, ErrorCode::kTypeDecides);

  const ElementId wall = made(document.execute(AddWall{.wall = wall_of(level, type)}));
  const auto thickness = document.execute(SetWallThickness{.wall = wall, .thickness = 250_mm});
  ASSERT_FALSE(thickness.has_value());
  EXPECT_EQ(thickness.error().code, ErrorCode::kTypeDecides);
  const auto unit = document.execute(
      SetWallUnit{.wall = wall, .length = 250_mm, .width = 120_mm, .height = 65_mm});
  ASSERT_FALSE(unit.has_value());
  EXPECT_EQ(unit.error().code, ErrorCode::kTypeDecides);
}

TEST(WallType, GivingAWallATypeDropsItsOwnThicknessAndUnitInOneStep) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  const ElementId type = made(document.execute(outside_wall()));
  const ElementId wall = made(document.execute(AddWall{.wall = plain_wall(level)}));

  ASSERT_TRUE(document.execute(SetWallType{.wall = wall, .type = type}));

  EXPECT_EQ(document.log().back().kind, CommandKind::kSetWallType);
  const Wall* typed = document.model().find<Wall>(wall);
  EXPECT_EQ(typed->type, type);
  EXPECT_EQ(typed->thickness, 0_mm);
  EXPECT_FALSE(typed->has_unit());
  EXPECT_EQ(build_of(document.model(), *typed).thickness(), 470_mm);

  ASSERT_TRUE(document.execute(SetWallMirrored{.wall = wall, .mirrored = true}));
  EXPECT_TRUE(document.model().find<Wall>(wall)->mirrored);

  ASSERT_TRUE(document.undo());
  ASSERT_TRUE(document.undo());
  EXPECT_EQ(*document.model().find<Wall>(wall), plain_wall(level));
}

TEST(WallType, AWallNeedsItsTypeToExist) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  const ElementId wall = made(document.execute(AddWall{.wall = plain_wall(level)}));
  const ElementId nowhere{Uuid{7, 7}};

  const auto added = document.execute(AddWall{.wall = wall_of(level, nowhere)});
  ASSERT_FALSE(added.has_value());
  EXPECT_EQ(added.error().code, ErrorCode::kNotFound);
  const auto typed = document.execute(SetWallType{.wall = wall, .type = nowhere});
  ASSERT_FALSE(typed.has_value());
  EXPECT_EQ(typed.error().code, ErrorCode::kNotFound);
}

TEST(WallType, NeedsLayersThatCanBeBuilt) {
  Document document{test_environment()};

  const auto none = document.execute(AddWallType{.role = WallRole::kPartition, .layers = {}});
  ASSERT_FALSE(none.has_value());
  EXPECT_EQ(none.error().code, ErrorCode::kThicknessNotPositive);

  const auto thin = document.execute(AddWallType{
      .role = WallRole::kPartition, .layers = {brick(LayerFunction::kStructure, 0_mm)}});
  ASSERT_FALSE(thin.has_value());
  EXPECT_EQ(thin.error().code, ErrorCode::kThicknessNotPositive);

  // Insulation is not laid from bricks.
  const auto laid = document.execute(AddWallType{
      .role = WallRole::kExterior, .layers = {brick(LayerFunction::kInsulation, 100_mm)}});
  ASSERT_FALSE(laid.has_value());
  EXPECT_EQ(laid.error().code, ErrorCode::kUnitSizeNotPositive);
  EXPECT_EQ(document.model().count<WallType>(), 0U);
  EXPECT_TRUE(document.log().empty());
}

TEST(WallType, ASavedJobOpensWithItsTypes) {
  const ElementId level{Uuid{10, 1}};
  const ElementId type{Uuid{10, 2}};
  const ElementId wall{Uuid{10, 5}};
  WallLayer facing = brick(LayerFunction::kFacing, 120_mm);
  facing.type = type;
  WallLayer structure = brick(LayerFunction::kStructure, 250_mm);
  structure.type = type;
  structure.position = 1;
  const Snapshot saved{
      .levels = {{level, Level{.elevation = 0_mm, .height = 3000_mm}}},
      .walls = {{wall, wall_of(level, type)}},
      .openings = {},
      .conditions = {},
      .clock = {},
      .wall_types = {{type, WallType{.role = WallRole::kBearing}}},
      // Saved in any order: the positions say which is outside.
      .wall_layers = {{ElementId{Uuid{10, 3}}, structure}, {ElementId{Uuid{10, 4}}, facing}}};
  Document document{test_environment()};

  ASSERT_TRUE(document.open(saved));

  const Build build = build_of(document.model(), *document.model().find<Wall>(wall));
  EXPECT_EQ(build.role, WallRole::kBearing);
  ASSERT_EQ(build.layers.size(), 2U);
  EXPECT_EQ(build.layers[0].function, LayerFunction::kFacing);
  EXPECT_EQ(build.thickness(), 370_mm);

  // A wall whose type is not saved, or a layer of a type that is not, is a broken job.
  Snapshot untyped = saved;
  untyped.wall_types.clear();
  untyped.wall_layers.clear();
  const auto missing = document.open(untyped);
  ASSERT_FALSE(missing.has_value());
  EXPECT_EQ(missing.error().code, ErrorCode::kCorruptData);
  Snapshot orphan = saved;
  orphan.wall_types.clear();
  orphan.walls.clear();
  EXPECT_FALSE(document.open(orphan).has_value());
  // Nothing changed.
  EXPECT_EQ(build_of(document.model(), *document.model().find<Wall>(wall)).thickness(), 370_mm);
}

}  // namespace
}  // namespace otves
