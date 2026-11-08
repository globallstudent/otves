#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/command.h>
#include <otves/core/document.h>
#include <otves/core/error.h>
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

Wall wall_4() {
  return Wall{.start = {.x = 0_mm, .y = 0_mm},
              .end = {.x = 6400_mm, .y = 0_mm},
              .thickness = 380_mm,
              .height = 2800_mm,
              .location_line = LocationLine::kLeftFace};
}

// Window ОК-1 of the reference house: 1 500 × 1 500 on an 900 mm sill, 1 200 mm from the corner.
Opening window_1(ElementId wall) {
  return Opening{.wall = wall,
                 .offset = 1200_mm,
                 .width = 1500_mm,
                 .height = 1500_mm,
                 .sill = 900_mm,
                 .kind = OpeningKind::kWindow};
}

ElementId last_created(const Document& document) {
  const Op op = document.log().back().ops.back();
  return std::visit([](const auto& typed) { return typed.id; }, op);
}

struct House {
  Document document{test_environment()};
  ElementId wall;
  ElementId window;
};

std::unique_ptr<House> house() {
  auto h = std::make_unique<House>();
  EXPECT_TRUE(h->document.execute(AddWall{.wall = wall_4()}));
  h->wall = last_created(h->document);
  EXPECT_TRUE(h->document.execute(AddOpening{.opening = window_1(h->wall)}));
  h->window = last_created(h->document);
  return h;
}

TEST(Opening, AddRecordsOneCreateOp) {
  const auto h = house();

  const OpGroup& group = h->document.log().back();
  EXPECT_EQ(group.kind, CommandKind::kAddOpening);
  ASSERT_EQ(group.ops.size(), 1U);
  const auto* create = std::get_if<CreateOpening>(&group.ops.front());
  ASSERT_NE(create, nullptr);
  EXPECT_EQ(create->element, window_1(h->wall));
  EXPECT_EQ(*h->document.model().find<Opening>(h->window), window_1(h->wall));
}

TEST(Opening, NeedsAnExistingWall) {
  Document document{test_environment()};
  const ElementId missing{Uuid::v7(1, 2, 3)};

  EXPECT_EQ(document.execute(AddOpening{.opening = window_1(missing)}).error().code,
            ErrorCode::kNotFound);
  EXPECT_TRUE(document.log().empty());
}

TEST(Opening, RejectsInvalidSizesAndChangesNothing) {
  const auto h = house();
  const auto code = [&h](const Opening& opening) {
    return h->document.execute(AddOpening{.opening = opening}).error().code;
  };
  Opening narrow = window_1(h->wall);
  narrow.width = 0_mm;
  Opening flat = window_1(h->wall);
  flat.height = 0_mm;
  Opening buried = window_1(h->wall);
  buried.sill = -1_mm;
  Opening before_start = window_1(h->wall);
  before_start.offset = -1_mm;

  EXPECT_EQ(code(narrow), ErrorCode::kWidthNotPositive);
  EXPECT_EQ(code(flat), ErrorCode::kHeightNotPositive);
  EXPECT_EQ(code(buried), ErrorCode::kSillNegative);
  EXPECT_EQ(code(before_start), ErrorCode::kOffsetNegative);
  EXPECT_EQ(h->document.model().count<Opening>(), 1U);
}

// One command per frame, 10 mm further each time, as a finger drags the opening.
bool drag(Document& document, ElementId opening, int frames) {
  document.begin_gesture();
  for (int frame = 1; frame <= frames; ++frame) {
    if (!document.execute(MoveOpening{.opening = opening, .offset = Mm{1200 + (frame * 10)}})) {
      return false;
    }
  }
  return document.end_gesture().has_value();
}

TEST(Opening, DragAlongTheWallFoldsIntoOneChange) {
  const auto h = house();

  ASSERT_TRUE(drag(h->document, h->window, 30));

  const OpGroup& drag = h->document.log().back();
  EXPECT_EQ(drag.kind, CommandKind::kMoveOpening);
  ASSERT_EQ(drag.ops.size(), 1U);
  const auto* set = std::get_if<SetOpeningField>(&drag.ops.front());
  ASSERT_NE(set, nullptr);
  EXPECT_EQ(set->before, OpeningValue{1200_mm});
  EXPECT_EQ(set->after, OpeningValue{1500_mm});
}

TEST(Opening, ResizeRecordsOnlyFieldsThatChange) {
  const auto h = house();

  ASSERT_TRUE(h->document.execute(
      ResizeOpening{.opening = h->window, .width = 1800_mm, .height = 1500_mm}));

  const OpGroup& group = h->document.log().back();
  ASSERT_EQ(group.ops.size(), 1U);
  EXPECT_EQ(
      group.ops.front(),
      (Op{SetOpeningField{
          .id = h->window, .field = OpeningField::kWidth, .before = 1500_mm, .after = 1800_mm}}));
  EXPECT_EQ(h->document.execute(ResizeOpening{.opening = h->window, .width = 0_mm, .height = 1_mm})
                .error()
                .code,
            ErrorCode::kWidthNotPositive);
}

TEST(Opening, SillAndKindUndoAndRedo) {
  const auto h = house();

  ASSERT_TRUE(h->document.execute(SetOpeningSill{.opening = h->window, .sill = 0_mm}));
  ASSERT_TRUE(
      h->document.execute(SetOpeningKind{.opening = h->window, .kind = OpeningKind::kDoor}));
  const auto* opening = h->document.model().find<Opening>(h->window);
  EXPECT_EQ(opening->sill, 0_mm);
  EXPECT_EQ(opening->kind, OpeningKind::kDoor);

  ASSERT_TRUE(h->document.undo());
  ASSERT_TRUE(h->document.undo());
  EXPECT_EQ(*h->document.model().find<Opening>(h->window), window_1(h->wall));
  ASSERT_TRUE(h->document.redo());
  EXPECT_EQ(h->document.model().find<Opening>(h->window)->sill, 0_mm);
}

TEST(Opening, RemovingAWallRemovesItsOpeningsAndUndoBringsThemBack) {
  const auto h = house();
  ASSERT_TRUE(h->document.execute(AddWall{.wall = wall_4()}));
  const ElementId other = last_created(h->document);
  ASSERT_TRUE(h->document.execute(AddOpening{.opening = window_1(other)}));
  Opening door = window_1(h->wall);
  door.offset = 4000_mm;
  door.sill = 0_mm;
  door.kind = OpeningKind::kDoor;
  ASSERT_TRUE(h->document.execute(AddOpening{.opening = door}));

  ASSERT_TRUE(h->document.execute(RemoveWall{.wall = h->wall}));

  const OpGroup& removal = h->document.log().back();
  ASSERT_EQ(removal.ops.size(), 3U);
  EXPECT_TRUE(std::holds_alternative<DeleteOpening>(removal.ops[0]));
  EXPECT_TRUE(std::holds_alternative<DeleteOpening>(removal.ops[1]));
  EXPECT_TRUE(std::holds_alternative<DeleteWall>(removal.ops[2]));
  EXPECT_EQ(h->document.model().count<Opening>(), 1U);

  ASSERT_TRUE(h->document.undo());
  EXPECT_EQ(h->document.model().count<Wall>(), 2U);
  EXPECT_EQ(h->document.model().count<Opening>(), 3U);
  EXPECT_EQ(*h->document.model().find<Opening>(h->window), window_1(h->wall));
}

TEST(Opening, RemoveAndMissingOpening) {
  const auto h = house();
  const ElementId missing{Uuid::v7(1, 2, 3)};

  EXPECT_EQ(h->document.execute(RemoveOpening{.opening = missing}).error().code,
            ErrorCode::kNotFound);
  EXPECT_EQ(h->document.execute(MoveOpening{.opening = missing, .offset = 0_mm}).error().code,
            ErrorCode::kNotFound);
  ASSERT_TRUE(h->document.execute(RemoveOpening{.opening = h->window}));
  EXPECT_EQ(h->document.model().count<Opening>(), 0U);
  EXPECT_EQ(h->document.model().count<Wall>(), 1U);
}

TEST(Opening, FieldsRoundTrip) {
  Opening opening = window_1(ElementId{Uuid::v7(1, 2, 3)});
  set_field(opening, OpeningField::kOffset, 10_mm);
  set_field(opening, OpeningField::kWidth, 20_mm);
  set_field(opening, OpeningField::kHeight, 30_mm);
  set_field(opening, OpeningField::kSill, 40_mm);
  set_field(opening, OpeningField::kKind, OpeningKind::kEmpty);

  EXPECT_EQ(get_field(opening, OpeningField::kOffset), OpeningValue{10_mm});
  EXPECT_EQ(get_field(opening, OpeningField::kWidth), OpeningValue{20_mm});
  EXPECT_EQ(get_field(opening, OpeningField::kHeight), OpeningValue{30_mm});
  EXPECT_EQ(get_field(opening, OpeningField::kSill), OpeningValue{40_mm});
  EXPECT_EQ(get_field(opening, OpeningField::kKind), OpeningValue{OpeningKind::kEmpty});
}

}  // namespace
}  // namespace otves
