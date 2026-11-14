#include <cstdint>
#include <memory>
#include <optional>

#include <gtest/gtest.h>

#include <otves/core/command.h>
#include <otves/core/document.h>
#include <otves/core/error.h>
#include <otves/core/op.h>
#include <otves/core/point.h>
#include <otves/core/room.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>

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

Room bedroom(ElementId level) {
  return {.level = level, .anchor = {.x = 2000_mm, .y = 1500_mm}, .kind = RoomKind::kBedroom};
}

ErrorCode refusal(const Result<std::optional<ElementId>>& result) {
  EXPECT_FALSE(result.has_value());
  return result.has_value() ? ErrorCode::kNotFound : result.error().code;
}

TEST(Room, IsAddedRenamedMovedAndRemoved) {
  Document document{test_environment()};
  const ElementId level = add_level(document);

  const ElementId room = made(document.execute(AddRoom{.room = bedroom(level)}));
  ASSERT_NE(document.model().find<Room>(room), nullptr);
  EXPECT_EQ(*document.model().find<Room>(room), bedroom(level));
  EXPECT_EQ(document.log().back().kind, CommandKind::kAddRoom);

  ASSERT_TRUE(document.execute(SetRoomKind{.room = room, .kind = RoomKind::kStudy}));
  EXPECT_EQ(document.model().find<Room>(room)->kind, RoomKind::kStudy);
  ASSERT_TRUE(document.execute(MoveRoom{.room = room, .anchor = {.x = 500_mm, .y = 700_mm}}));
  const PointMm moved{.x = 500_mm, .y = 700_mm};
  EXPECT_EQ(document.model().find<Room>(room)->anchor, moved);

  ASSERT_TRUE(document.undo());
  ASSERT_TRUE(document.undo());
  EXPECT_EQ(*document.model().find<Room>(room), bedroom(level));

  ASSERT_TRUE(document.execute(RemoveRoom{.room = room}));
  EXPECT_EQ(document.model().find<Room>(room), nullptr);
  ASSERT_TRUE(document.undo());
  EXPECT_EQ(*document.model().find<Room>(room), bedroom(level));
}

TEST(Room, NeedsALevelAndAPointOnThePlan) {
  Document document{test_environment()};
  const ElementId level = add_level(document);

  EXPECT_EQ(refusal(document.execute(AddRoom{.room = bedroom(ElementId{Uuid{7, 7}})})),
            ErrorCode::kNotFound);
  Room far = bedroom(level);
  far.anchor.x = kMaxCoordinate + 1_mm;
  EXPECT_EQ(refusal(document.execute(AddRoom{.room = far})), ErrorCode::kOffPlan);
  EXPECT_EQ(refusal(document.execute(RemoveRoom{.room = ElementId{Uuid{7, 7}}})),
            ErrorCode::kNotFound);

  const ElementId room = made(document.execute(AddRoom{.room = bedroom(level)}));
  EXPECT_EQ(refusal(document.execute(MoveRoom{.room = room, .anchor = far.anchor})),
            ErrorCode::kOffPlan);
  EXPECT_EQ(*document.model().find<Room>(room), bedroom(level));
  // The same name again changes nothing and logs nothing.
  const auto logged = document.log().size();
  ASSERT_TRUE(document.execute(SetRoomKind{.room = room, .kind = RoomKind::kBedroom}));
  EXPECT_EQ(document.log().size(), logged);
}

TEST(Room, GoesWithItsLevelAndComesBackWithIt) {
  Document document{test_environment()};
  const ElementId ground = add_level(document);
  const ElementId upper =
      made(document.execute(AddLevel{.level = {.elevation = 3000_mm, .height = 3000_mm}}));
  const ElementId below = made(document.execute(AddRoom{.room = bedroom(ground)}));
  const ElementId above = made(document.execute(AddRoom{.room = bedroom(upper)}));

  ASSERT_TRUE(document.execute(RemoveLevel{.level = upper}));
  EXPECT_EQ(document.model().find<Room>(above), nullptr);
  EXPECT_NE(document.model().find<Room>(below), nullptr);

  ASSERT_TRUE(document.undo());
  ASSERT_NE(document.model().find<Room>(above), nullptr);
  EXPECT_EQ(document.model().find<Room>(above)->level, upper);
}

TEST(Room, OpensFromASavedJobOnlyOnALevelThatExists) {
  const ElementId level{Uuid::v7(1, 0, 0)};
  const ElementId room{Uuid::v7(2, 0, 0)};
  Snapshot saved{.levels = {{level, {.elevation = 0_mm, .height = 3000_mm}}},
                 .walls = {},
                 .openings = {},
                 .clock = {},
                 .rooms = {{room, bedroom(level)}}};
  Document document{test_environment()};
  ASSERT_TRUE(document.open(saved));
  EXPECT_EQ(document.model().count<Room>(), 1U);
  EXPECT_EQ(*document.model().find<Room>(room), bedroom(level));

  saved.rooms.front().second.level = ElementId{Uuid::v7(9, 0, 0)};
  const auto refused = document.open(saved);
  ASSERT_FALSE(refused.has_value());
  EXPECT_EQ(refused.error().code, ErrorCode::kCorruptData);
  EXPECT_EQ(document.model().count<Room>(), 1U);
}

}  // namespace
}  // namespace otves
