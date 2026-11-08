#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/command.h>
#include <otves/core/document.h>
#include <otves/core/error.h>
#include <otves/core/level.h>
#include <otves/core/op.h>
#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

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

constexpr Level kGround{.elevation = 0_mm, .height = 3000_mm};
constexpr Level kFirst{.elevation = 3000_mm, .height = 2800_mm};

ElementId last_created(const Document& document) {
  const Op op = document.log().back().ops.back();
  return std::visit([](const auto& typed) { return typed.id; }, op);
}

Wall wall_on(ElementId level, Mm y) {
  return Wall{.level = level,
              .start = {.x = 0_mm, .y = y},
              .end = {.x = 6400_mm, .y = y},
              .thickness = 380_mm,
              .height = 2800_mm,
              .location_line = LocationLine::kCenter};
}

TEST(Level, AddRecordsOneCreateOp) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddLevel{.level = kGround}));

  const OpGroup& group = document.log().back();
  EXPECT_EQ(group.kind, CommandKind::kAddLevel);
  ASSERT_EQ(group.ops.size(), 1U);
  const auto* create = std::get_if<CreateLevel>(&group.ops.front());
  ASSERT_NE(create, nullptr);
  EXPECT_EQ(create->element, kGround);
  EXPECT_EQ(*document.model().find<Level>(create->id), kGround);
}

TEST(Level, RejectsHeightThatIsNotPositive) {
  Document document{test_environment()};
  const Level flat{.elevation = 0_mm, .height = 0_mm};

  EXPECT_EQ(document.execute(AddLevel{.level = flat}).error().code, ErrorCode::kHeightNotPositive);
  EXPECT_TRUE(document.log().empty());
}

TEST(Level, ElevationAndHeightUndo) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddLevel{.level = kGround}));
  const ElementId level = last_created(document);

  ASSERT_TRUE(document.execute(SetLevelElevation{.level = level, .elevation = -450_mm}));
  ASSERT_TRUE(document.execute(SetLevelHeight{.level = level, .height = 3300_mm}));
  EXPECT_EQ(*document.model().find<Level>(level), (Level{.elevation = -450_mm, .height = 3300_mm}));
  EXPECT_EQ(document.execute(SetLevelHeight{.level = level, .height = -1_mm}).error().code,
            ErrorCode::kHeightNotPositive);

  ASSERT_TRUE(document.undo());
  ASSERT_TRUE(document.undo());
  EXPECT_EQ(*document.model().find<Level>(level), kGround);
}

TEST(Level, RemovingALevelRemovesItsWallsAndOpeningsAndUndoBringsThemBack) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddLevel{.level = kGround}));
  const ElementId ground = last_created(document);
  ASSERT_TRUE(document.execute(AddLevel{.level = kFirst}));
  const ElementId first = last_created(document);
  for (const Mm y : {0_mm, 4000_mm}) {
    ASSERT_TRUE(document.execute(AddWall{.wall = wall_on(ground, y)}));
  }
  const ElementId wall = last_created(document);
  ASSERT_TRUE(document.execute(AddOpening{.opening = {.wall = wall,
                                                      .offset = 1200_mm,
                                                      .width = 1500_mm,
                                                      .height = 1500_mm,
                                                      .sill = 900_mm,
                                                      .kind = OpeningKind::kWindow}}));
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_on(first, 0_mm)}));

  ASSERT_TRUE(document.execute(RemoveLevel{.level = ground}));

  const OpGroup& removal = document.log().back();
  EXPECT_EQ(removal.kind, CommandKind::kRemoveLevel);
  ASSERT_EQ(removal.ops.size(), 4U);
  EXPECT_TRUE(std::holds_alternative<DeleteLevel>(removal.ops.back()));
  EXPECT_EQ(document.model().count<Level>(), 1U);
  EXPECT_EQ(document.model().count<Wall>(), 1U);
  EXPECT_EQ(document.model().count<Opening>(), 0U);

  ASSERT_TRUE(document.undo());
  EXPECT_EQ(document.model().count<Level>(), 2U);
  EXPECT_EQ(document.model().count<Wall>(), 3U);
  EXPECT_EQ(document.model().count<Opening>(), 1U);
  EXPECT_EQ(document.model().find<Wall>(wall)->level, ground);
}

TEST(Level, FieldsRoundTrip) {
  Level level = kGround;
  set_field(level, LevelField::kElevation, -900_mm);
  set_field(level, LevelField::kHeight, 2500_mm);

  EXPECT_EQ(get_field(level, LevelField::kElevation), LevelValue{-900_mm});
  EXPECT_EQ(get_field(level, LevelField::kHeight), LevelValue{2500_mm});
}

}  // namespace
}  // namespace otves
