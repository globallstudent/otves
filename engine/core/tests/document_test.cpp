#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/command.h>
#include <otves/core/document.h>
#include <otves/core/error.h>
#include <otves/core/op.h>
#include <otves/core/point.h>
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

// Wall Ст-4 of the reference house: 6 400 mm along the bottom, 1½ bricks, 2 800 mm high.
Wall wall_4() {
  return Wall{.start = {.x = 0_mm, .y = 0_mm},
              .end = {.x = 6400_mm, .y = 0_mm},
              .thickness = 380_mm,
              .height = 2800_mm,
              .location_line = LocationLine::kLeftFace};
}

ElementId only_wall(const Document& document) {
  ElementId found;
  document.model().for_each_wall([&found](ElementId id, const Wall&) { found = id; });
  return found;
}

TEST(Document, AddWallRecordsOneCreateOp) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));

  ASSERT_EQ(document.log().size(), 1U);
  const OpGroup& group = document.log().front();
  EXPECT_EQ(group.kind, CommandKind::kAddWall);
  EXPECT_EQ(group.id.uuid().version(), 7U);
  ASSERT_EQ(group.ops.size(), 1U);
  const auto* create = std::get_if<CreateWall>(&group.ops.front());
  ASSERT_NE(create, nullptr);
  EXPECT_EQ(create->wall, wall_4());
  EXPECT_EQ(*document.model().find_wall(create->id), wall_4());
}

TEST(Document, RejectsInvalidWallAndChangesNothing) {
  Document document{test_environment()};
  Wall flat = wall_4();
  flat.thickness = 0_mm;
  Wall low = wall_4();
  low.height = -1_mm;
  Wall point = wall_4();
  point.end = point.start;
  Wall far = wall_4();
  far.end.x = kMaxCoordinate + 1_mm;

  const auto code = [&document](const Wall& wall) {
    return document.execute(AddWall{.wall = wall}).error().code;
  };
  EXPECT_EQ(code(flat), ErrorCode::kThicknessNotPositive);
  EXPECT_EQ(code(low), ErrorCode::kHeightNotPositive);
  EXPECT_EQ(code(point), ErrorCode::kZeroLength);
  EXPECT_EQ(code(far), ErrorCode::kOffPlan);
  EXPECT_TRUE(document.log().empty());
  EXPECT_EQ(document.model().wall_count(), 0U);
}

TEST(Document, MoveWallEndRecordsBeforeAndAfter) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);

  ASSERT_TRUE(
      document.execute(MoveWallEnd{.wall = id, .end = WallEnd::kEnd, .to = {7000_mm, 0_mm}}));

  const auto* set = std::get_if<SetWallField>(&document.log().back().ops.front());
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(set->field, WallField::kEnd);
  EXPECT_EQ(set->before, (WallValue{PointMm{6400_mm, 0_mm}}));
  EXPECT_EQ(set->after, (WallValue{PointMm{7000_mm, 0_mm}}));
  EXPECT_EQ(document.model().find_wall(id)->end, (PointMm{7000_mm, 0_mm}));
}

TEST(Document, UndoRestoresExactStateAndAppendsToLog) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallThickness{.wall = id, .thickness = 510_mm}));
  const GroupId thickened = document.log().back().id;

  ASSERT_TRUE(document.undo());

  EXPECT_EQ(*document.model().find_wall(id), wall_4());
  ASSERT_EQ(document.log().size(), 3U);
  EXPECT_EQ(document.log().back().kind, CommandKind::kUndo);
  EXPECT_EQ(document.log().back().origin, thickened);
  EXPECT_TRUE(document.can_redo());
}

TEST(Document, RedoReappliesUndoneChange) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallHeight{.wall = id, .height = 3000_mm}));
  ASSERT_TRUE(document.undo());

  ASSERT_TRUE(document.redo());

  EXPECT_EQ(document.model().find_wall(id)->height, 3000_mm);
  EXPECT_EQ(document.log().back().kind, CommandKind::kRedo);
  EXPECT_TRUE(document.can_undo());
  EXPECT_FALSE(document.can_redo());
}

TEST(Document, NewCommandClearsRedo) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallHeight{.wall = id, .height = 3000_mm}));
  ASSERT_TRUE(document.undo());

  ASSERT_TRUE(document.execute(SetWallThickness{.wall = id, .thickness = 250_mm}));

  EXPECT_FALSE(document.can_redo());
  EXPECT_EQ(document.redo().error().code, ErrorCode::kNothingToRedo);
}

TEST(Document, UndoingRemovalRestoresTheSameWall) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(RemoveWall{.wall = id}));
  ASSERT_EQ(document.model().find_wall(id), nullptr);

  ASSERT_TRUE(document.undo());

  ASSERT_NE(document.model().find_wall(id), nullptr);
  EXPECT_EQ(*document.model().find_wall(id), wall_4());
}

TEST(Document, GestureFoldsEveryFrameIntoOneGroup) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);

  document.begin_gesture();
  for (std::int64_t frame = 1; frame <= 120; ++frame) {
    ASSERT_TRUE(document.execute(
        MoveWallEnd{.wall = id, .end = WallEnd::kEnd, .to = {Mm{6400 + (frame * 5)}, 0_mm}}));
  }
  const std::optional<GroupId> group = document.end_gesture();

  ASSERT_TRUE(group.has_value());
  ASSERT_EQ(document.log().size(), 2U);
  const OpGroup& drag = document.log().back();
  EXPECT_EQ(drag.kind, CommandKind::kMoveWallEnd);
  ASSERT_EQ(drag.ops.size(), 1U);
  const auto* set = std::get_if<SetWallField>(&drag.ops.front());
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(set->before, (WallValue{PointMm{6400_mm, 0_mm}}));
  EXPECT_EQ(set->after, (WallValue{PointMm{7000_mm, 0_mm}}));
}

TEST(Document, GestureEndingWhereItStartedRecordsNothing) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);

  document.begin_gesture();
  ASSERT_TRUE(
      document.execute(MoveWallEnd{.wall = id, .end = WallEnd::kEnd, .to = {6500_mm, 0_mm}}));
  ASSERT_TRUE(
      document.execute(MoveWallEnd{.wall = id, .end = WallEnd::kEnd, .to = {6400_mm, 0_mm}}));

  EXPECT_FALSE(document.end_gesture().has_value());
  EXPECT_EQ(document.log().size(), 1U);
}

TEST(Document, GestureLeftOpenIsLoggedWhenTheNextOneBegins) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);

  // The touch-up of the first drag never arrives.
  document.begin_gesture();
  ASSERT_TRUE(
      document.execute(MoveWallEnd{.wall = id, .end = WallEnd::kEnd, .to = {7000_mm, 0_mm}}));
  document.begin_gesture();
  ASSERT_TRUE(document.execute(SetWallHeight{.wall = id, .height = 3000_mm}));
  ASSERT_TRUE(document.end_gesture().has_value());

  ASSERT_EQ(document.log().size(), 3U);
  EXPECT_EQ(document.log()[1].kind, CommandKind::kMoveWallEnd);
  EXPECT_EQ(document.log()[2].kind, CommandKind::kSetWallHeight);
  ASSERT_TRUE(document.undo());
  ASSERT_TRUE(document.undo());
  EXPECT_EQ(*document.model().find_wall(id), wall_4());
}

TEST(Document, UndoIsRefusedDuringGesture) {
  Document document{test_environment()};
  EXPECT_EQ(document.undo().error().code, ErrorCode::kNothingToUndo);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  document.begin_gesture();
  EXPECT_EQ(document.undo().error().code, ErrorCode::kGestureOpen);
  EXPECT_EQ(document.redo().error().code, ErrorCode::kGestureOpen);
  static_cast<void>(document.end_gesture());
  EXPECT_TRUE(document.undo());
}

TEST(Document, UnknownWallIsNotFound) {
  Document document{test_environment()};
  const ElementId missing{Uuid{7, 7}};
  EXPECT_EQ(document.execute(RemoveWall{.wall = missing}).error().code, ErrorCode::kNotFound);
  EXPECT_EQ(document.execute(SetWallHeight{.wall = missing, .height = 3000_mm}).error().code,
            ErrorCode::kNotFound);
}

TEST(Document, SettingTheSameValueRecordsNothing) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallThickness{.wall = id, .thickness = 380_mm}));
  EXPECT_EQ(document.log().size(), 1U);
}

TEST(Document, ReportsDirtyElementsOnce) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallHeight{.wall = id, .height = 3000_mm}));

  EXPECT_EQ(document.take_dirty(), std::vector<ElementId>{id});
  EXPECT_TRUE(document.take_dirty().empty());
}

TEST(Document, StampsGrowStrictly) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4()}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallHeight{.wall = id, .height = 3000_mm}));
  ASSERT_TRUE(document.undo());

  const auto log = document.log();
  for (std::size_t i = 1; i < log.size(); ++i) {
    EXPECT_LT(log[i - 1].stamp.hlc, log[i].stamp.hlc);
    EXPECT_EQ(log[i].stamp.device, (DeviceId{Uuid{1, 1}}));
  }
}

}  // namespace
}  // namespace otves
