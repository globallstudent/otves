#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/command.h>
#include <otves/core/conditions.h>
#include <otves/core/document.h>
#include <otves/core/error.h>
#include <otves/core/hlc.h>
#include <otves/core/level.h>
#include <otves/core/op.h>
#include <otves/core/opening.h>
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

// The ground floor, 3 000 mm floor to floor: every wall stands on a level.
ElementId add_level(Document& document) {
  EXPECT_TRUE(document.execute(AddLevel{.level = {.elevation = 0_mm, .height = 3000_mm}}));
  ElementId found;
  document.model().for_each<Level>([&found](ElementId id, const Level&) { found = id; });
  static_cast<void>(document.take_dirty());
  return found;
}

// Wall Ст-4 of the reference house: 6 400 mm along the bottom, 1½ bricks, 2 800 mm high.
Wall wall_4(ElementId level) {
  return Wall{.level = level,
              .start = {.x = 0_mm, .y = 0_mm},
              .end = {.x = 6400_mm, .y = 0_mm},
              .thickness = 380_mm,
              .height = 2800_mm,
              .location_line = LocationLine::kLeftFace};
}

ElementId only_wall(const Document& document) {
  ElementId found;
  document.model().for_each<Wall>([&found](ElementId id, const Wall&) { found = id; });
  return found;
}

TEST(Document, AddWallRecordsOneCreateOp) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));

  ASSERT_EQ(document.log().size(), 2U);
  const OpGroup& group = document.log().back();
  EXPECT_EQ(group.kind, CommandKind::kAddWall);
  EXPECT_EQ(group.id.uuid().version(), 7U);
  ASSERT_EQ(group.ops.size(), 1U);
  const auto* create = std::get_if<CreateWall>(&group.ops.front());
  ASSERT_NE(create, nullptr);
  EXPECT_EQ(create->element, wall_4(level));
  EXPECT_EQ(*document.model().find<Wall>(create->id), wall_4(level));
}

TEST(Document, ExecuteReturnsTheCreatedElement) {
  Document document{test_environment()};
  const ElementId level = add_level(document);

  const auto added = document.execute(AddWall{.wall = wall_4(level)});
  const std::optional<ElementId> created = added.value_or(std::nullopt);
  EXPECT_EQ(created, only_wall(document));
  const auto moved = document.execute(MoveWallEnd{
      .wall = created.value_or(ElementId{}), .end = WallEnd::kEnd, .to = {7000_mm, 0_mm}});
  ASSERT_TRUE(moved.has_value());
  EXPECT_FALSE(moved->has_value());
}

TEST(Document, RejectsInvalidWallAndChangesNothing) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  Wall flat = wall_4(level);
  flat.thickness = 0_mm;
  Wall low = wall_4(level);
  low.height = -1_mm;
  Wall point = wall_4(level);
  point.end = point.start;
  Wall far = wall_4(level);
  far.end.x = kMaxCoordinate + 1_mm;

  const auto code = [&document](const Wall& wall) {
    return document.execute(AddWall{.wall = wall}).error().code;
  };
  EXPECT_EQ(code(flat), ErrorCode::kThicknessNotPositive);
  EXPECT_EQ(code(low), ErrorCode::kHeightNotPositive);
  EXPECT_EQ(code(point), ErrorCode::kZeroLength);
  EXPECT_EQ(code(far), ErrorCode::kOffPlan);
  EXPECT_EQ(document.log().size(), 1U);
  EXPECT_EQ(document.model().count<Wall>(), 0U);
}

TEST(Document, MoveWallEndRecordsBeforeAndAfter) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);

  ASSERT_TRUE(
      document.execute(MoveWallEnd{.wall = id, .end = WallEnd::kEnd, .to = {7000_mm, 0_mm}}));

  const auto* set = std::get_if<SetWallField>(&document.log().back().ops.front());
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(set->field, WallField::kEnd);
  EXPECT_EQ(set->before, (WallValue{PointMm{6400_mm, 0_mm}}));
  EXPECT_EQ(set->after, (WallValue{PointMm{7000_mm, 0_mm}}));
  EXPECT_EQ(document.model().find<Wall>(id)->end, (PointMm{7000_mm, 0_mm}));
}

TEST(Document, UndoRestoresExactStateAndAppendsToLog) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallThickness{.wall = id, .thickness = 510_mm}));
  const GroupId thickened = document.log().back().id;

  ASSERT_TRUE(document.undo());

  EXPECT_EQ(*document.model().find<Wall>(id), wall_4(level));
  ASSERT_EQ(document.log().size(), 4U);
  EXPECT_EQ(document.log().back().kind, CommandKind::kUndo);
  EXPECT_EQ(document.log().back().origin, thickened);
  EXPECT_TRUE(document.can_redo());
}

TEST(Document, RedoReappliesUndoneChange) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallHeight{.wall = id, .height = 3000_mm}));
  ASSERT_TRUE(document.undo());

  ASSERT_TRUE(document.redo());

  EXPECT_EQ(document.model().find<Wall>(id)->height, 3000_mm);
  EXPECT_EQ(document.log().back().kind, CommandKind::kRedo);
  EXPECT_TRUE(document.can_undo());
  EXPECT_FALSE(document.can_redo());
}

TEST(Document, NewCommandClearsRedo) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallHeight{.wall = id, .height = 3000_mm}));
  ASSERT_TRUE(document.undo());

  ASSERT_TRUE(document.execute(SetWallThickness{.wall = id, .thickness = 250_mm}));

  EXPECT_FALSE(document.can_redo());
  EXPECT_EQ(document.redo().error().code, ErrorCode::kNothingToRedo);
}

TEST(Document, UndoingRemovalRestoresTheSameWall) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(RemoveWall{.wall = id}));
  ASSERT_EQ(document.model().find<Wall>(id), nullptr);

  ASSERT_TRUE(document.undo());

  ASSERT_NE(document.model().find<Wall>(id), nullptr);
  EXPECT_EQ(*document.model().find<Wall>(id), wall_4(level));
}

TEST(Document, GestureFoldsEveryFrameIntoOneGroup) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);

  document.begin_gesture();
  for (std::int64_t frame = 1; frame <= 120; ++frame) {
    ASSERT_TRUE(document.execute(
        MoveWallEnd{.wall = id, .end = WallEnd::kEnd, .to = {Mm{6400 + (frame * 5)}, 0_mm}}));
  }
  const std::optional<GroupId> group = document.end_gesture();

  ASSERT_TRUE(group.has_value());
  ASSERT_EQ(document.log().size(), 3U);
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
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);

  document.begin_gesture();
  ASSERT_TRUE(
      document.execute(MoveWallEnd{.wall = id, .end = WallEnd::kEnd, .to = {6500_mm, 0_mm}}));
  ASSERT_TRUE(
      document.execute(MoveWallEnd{.wall = id, .end = WallEnd::kEnd, .to = {6400_mm, 0_mm}}));

  EXPECT_FALSE(document.end_gesture().has_value());
  EXPECT_EQ(document.log().size(), 2U);
}

TEST(Document, GestureLeftOpenIsLoggedWhenTheNextOneBegins) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);

  // The touch-up of the first drag never arrives.
  document.begin_gesture();
  ASSERT_TRUE(
      document.execute(MoveWallEnd{.wall = id, .end = WallEnd::kEnd, .to = {7000_mm, 0_mm}}));
  document.begin_gesture();
  ASSERT_TRUE(document.execute(SetWallHeight{.wall = id, .height = 3000_mm}));
  ASSERT_TRUE(document.end_gesture().has_value());

  ASSERT_EQ(document.log().size(), 4U);
  EXPECT_EQ(document.log()[2].kind, CommandKind::kMoveWallEnd);
  EXPECT_EQ(document.log()[3].kind, CommandKind::kSetWallHeight);
  ASSERT_TRUE(document.undo());
  ASSERT_TRUE(document.undo());
  EXPECT_EQ(*document.model().find<Wall>(id), wall_4(level));
}

TEST(Document, UndoIsRefusedDuringGesture) {
  Document document{test_environment()};
  EXPECT_EQ(document.undo().error().code, ErrorCode::kNothingToUndo);
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  document.begin_gesture();
  EXPECT_EQ(document.undo().error().code, ErrorCode::kGestureOpen);
  EXPECT_EQ(document.redo().error().code, ErrorCode::kGestureOpen);
  static_cast<void>(document.end_gesture());
  EXPECT_TRUE(document.undo());
}

TEST(Document, WallNeedsAnExistingLevel) {
  Document document{test_environment()};
  const ElementId missing{Uuid{7, 7}};

  EXPECT_EQ(document.execute(AddWall{.wall = wall_4(missing)}).error().code, ErrorCode::kNotFound);
  EXPECT_TRUE(document.log().empty());
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
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallThickness{.wall = id, .thickness = 380_mm}));
  EXPECT_EQ(document.log().size(), 2U);
}

TEST(Document, AWallUnitIsChosenWholeAndUndoneWhole) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);
  EXPECT_FALSE(document.model().find<Wall>(id)->has_unit());

  // 1НФ brick as laid.
  ASSERT_TRUE(document.execute(
      SetWallUnit{.wall = id, .length = 250_mm, .width = 120_mm, .height = 65_mm}));
  EXPECT_TRUE(document.model().find<Wall>(id)->has_unit());
  EXPECT_EQ(document.log().back().ops.size(), 3U);

  const auto partial =
      document.execute(SetWallUnit{.wall = id, .length = 250_mm, .width = 0_mm, .height = 65_mm});
  ASSERT_FALSE(partial.has_value());
  EXPECT_EQ(partial.error().code, ErrorCode::kUnitSizeNotPositive);

  ASSERT_TRUE(document.undo());
  EXPECT_EQ(*document.model().find<Wall>(id), wall_4(level));
}

TEST(Document, IdsMadeInOneMillisecondSortAsMade) {
  DocumentEnvironment environment = test_environment();
  environment.unix_ms = [] { return std::int64_t{1'762'387'200'000}; };
  Document document{std::move(environment)};
  const ElementId level = add_level(document);

  std::vector<ElementId> made;
  for (std::int64_t metres = 1; metres <= 50; ++metres) {
    Wall wall = wall_4(level);
    wall.end.x = Mm{1000 * metres};
    const ElementId added =
        document.execute(AddWall{.wall = wall}).value_or(std::nullopt).value_or(ElementId{});
    ASSERT_FALSE(added.is_nil());
    made.push_back(added);
  }

  EXPECT_TRUE(std::ranges::is_sorted(made));
}

TEST(Document, ASketchedWallBecomesExactAndUndoes) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  Wall sketched = wall_4(level);
  sketched.exact = false;
  ASSERT_TRUE(document.execute(AddWall{.wall = sketched}));
  const ElementId id = only_wall(document);
  EXPECT_FALSE(document.model().find<Wall>(id)->exact);

  ASSERT_TRUE(document.execute(SetWallExact{.wall = id, .exact = true}));
  EXPECT_TRUE(document.model().find<Wall>(id)->exact);
  EXPECT_EQ(document.log().back().kind, CommandKind::kSetWallExact);
  const std::size_t groups = document.log().size();
  ASSERT_TRUE(document.execute(SetWallExact{.wall = id, .exact = true}));
  EXPECT_EQ(document.log().size(), groups);

  ASSERT_TRUE(document.undo());
  EXPECT_FALSE(document.model().find<Wall>(id)->exact);
}

TEST(Document, ALevelCarriesItsRingBeamAndMeshes) {
  Document document{test_environment()};
  const ElementId level = add_level(document);

  ASSERT_TRUE(document.execute(SetLevelRingBeam{.level = level, .height = 150_mm}));
  ASSERT_TRUE(document.execute(SetLevelJunctionMeshes{.level = level, .meshes = true}));
  EXPECT_EQ(document.model().find<Level>(level)->ring_beam, 150_mm);
  EXPECT_TRUE(document.model().find<Level>(level)->junction_meshes);
  EXPECT_EQ(document.log().back().kind, CommandKind::kSetLevelJunctionMeshes);

  const auto negative = document.execute(SetLevelRingBeam{.level = level, .height = -1_mm});
  EXPECT_FALSE(negative.has_value());

  ASSERT_TRUE(document.undo());
  EXPECT_FALSE(document.model().find<Level>(level)->junction_meshes);
}

TEST(Document, TheFirstConditionCreatesTheJobsConditionsAndLaterOnesSetFields) {
  Document document{test_environment()};

  // Unsetting what was never set records nothing.
  ASSERT_TRUE(document.execute(SetCondition{.field = ConditionsField::kCementGrade}));
  EXPECT_EQ(document.log().size(), 0U);

  const auto created =
      document.execute(SetCondition{.field = ConditionsField::kCementGrade, .value = 500});
  ASSERT_TRUE(created.has_value());
  const ElementId id = created->value_or(ElementId{});
  ASSERT_TRUE(document.execute(SetCondition{.field = ConditionsField::kUnitWaste, .value = 50}));
  EXPECT_EQ(document.model().count<Conditions>(), 1U);
  const auto* conditions = document.model().find<Conditions>(id);
  ASSERT_NE(conditions, nullptr);
  EXPECT_EQ(conditions->cement_grade, 500);
  EXPECT_EQ(conditions->unit_waste, 50);
  EXPECT_EQ(conditions->mortar_grade, Conditions::kUnset);
  EXPECT_TRUE(std::holds_alternative<SetConditionsField>(document.log().back().ops.front()));

  const auto too_much =
      document.execute(SetCondition{.field = ConditionsField::kMortarWaste, .value = 1000});
  ASSERT_FALSE(too_much.has_value());
  EXPECT_EQ(too_much.error().code, ErrorCode::kConditionOutOfRange);

  ASSERT_TRUE(document.undo());
  ASSERT_TRUE(document.undo());
  EXPECT_EQ(document.model().count<Conditions>(), 0U);
}

TEST(Document, AJobOpensWithItsConditionsAndRefusesTwoSets) {
  Document document{test_environment()};
  Snapshot snapshot;
  snapshot.conditions.emplace_back(ElementId{Uuid{5, 1}}, Conditions{.mortar_grade = 150});
  ASSERT_TRUE(document.open(snapshot));
  EXPECT_EQ(document.model().find<Conditions>(ElementId{Uuid{5, 1}})->mortar_grade, 150);

  snapshot.conditions.emplace_back(ElementId{Uuid{5, 2}}, Conditions{});
  const auto twice = document.open(snapshot);
  ASSERT_FALSE(twice.has_value());
  EXPECT_EQ(twice.error().code, ErrorCode::kCorruptData);
}

TEST(Document, ReportsDirtyElementsOnce) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallHeight{.wall = id, .height = 3000_mm}));

  EXPECT_EQ(document.take_dirty(), std::vector<ElementId>{id});
  EXPECT_TRUE(document.take_dirty().empty());
}

TEST(Document, StampsGrowStrictly) {
  Document document{test_environment()};
  const ElementId level = add_level(document);
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(level)}));
  const ElementId id = only_wall(document);
  ASSERT_TRUE(document.execute(SetWallHeight{.wall = id, .height = 3000_mm}));
  ASSERT_TRUE(document.undo());

  const auto log = document.log();
  for (std::size_t i = 1; i < log.size(); ++i) {
    EXPECT_LT(log[i - 1].stamp.hlc, log[i].stamp.hlc);
    EXPECT_EQ(log[i].stamp.device, (DeviceId{Uuid{1, 1}}));
  }
}

// A saved ground floor with wall Ст-4 and a window in it.
Snapshot saved_job() {
  const ElementId level{Uuid{10, 1}};
  const ElementId wall{Uuid{10, 2}};
  return Snapshot{.levels = {{level, Level{.elevation = 0_mm, .height = 3000_mm}}},
                  .walls = {{wall, wall_4(level)}},
                  .openings = {{ElementId{Uuid{10, 3}}, Opening{.wall = wall,
                                                                .offset = 1200_mm,
                                                                .width = 1500_mm,
                                                                .height = 1500_mm,
                                                                .sill = 900_mm}}},
                  .clock = Hlc{.value = 0xFFFF'0000'0000ULL}};
}

TEST(Document, OpenReplacesTheModelAndStartsAnEmptyLog) {
  Document document{test_environment()};
  ASSERT_TRUE(document.execute(AddWall{.wall = wall_4(add_level(document))}));
  const ElementId old_wall = only_wall(document);
  static_cast<void>(document.take_dirty());

  ASSERT_TRUE(document.open(saved_job()));

  EXPECT_EQ(document.model().count<Level>(), 1U);
  EXPECT_EQ(document.model().find<Wall>(ElementId{Uuid{10, 2}})->end.x, 6400_mm);
  EXPECT_EQ(document.model().find<Opening>(ElementId{Uuid{10, 3}})->sill, 900_mm);
  EXPECT_EQ(document.model().find<Wall>(old_wall), nullptr);
  EXPECT_TRUE(document.log().empty());
  EXPECT_FALSE(document.can_undo());
  const std::vector<ElementId> dirty = document.take_dirty();
  EXPECT_EQ(dirty.size(), 5U);  // the old level and wall, and the three saved elements
}

TEST(Document, OpenRefusesABrokenSnapshotAndKeepsTheModel) {
  Document document{test_environment()};
  const ElementId level = add_level(document);

  Snapshot orphan = saved_job();
  orphan.levels.clear();
  Snapshot repeated = saved_job();
  repeated.walls.front().first = repeated.levels.front().first;
  Snapshot invalid = saved_job();
  invalid.walls.front().second.thickness = 0_mm;

  for (const Snapshot& snapshot : {orphan, repeated, invalid}) {
    const auto opened = document.open(snapshot);
    ASSERT_FALSE(opened.has_value());
    EXPECT_EQ(opened.error().code, ErrorCode::kCorruptData);
  }
  EXPECT_NE(document.model().find<Level>(level), nullptr);
  EXPECT_TRUE(document.can_undo());
}

TEST(Document, GroupsAfterOpenStampAfterTheSavedClock) {
  Document document{test_environment()};
  const Snapshot snapshot = saved_job();
  ASSERT_TRUE(document.open(snapshot));

  ASSERT_TRUE(document.execute(AddLevel{.level = {.elevation = 3000_mm, .height = 3000_mm}}));

  ASSERT_EQ(document.log().size(), 1U);
  EXPECT_GT(document.log().front().stamp.hlc, snapshot.clock);
}

}  // namespace
}  // namespace otves
