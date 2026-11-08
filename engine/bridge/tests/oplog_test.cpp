#include "oplog.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include <flatbuffers/flatbuffers.h>
#include <gtest/gtest.h>

#include <otves/core/command.h>
#include <otves/core/document.h>
#include <otves/core/error.h>
#include <otves/core/op.h>
#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

#include "engine_generated.h"

namespace otves::bridge {
namespace {

using namespace otves::literals;

DocumentEnvironment test_environment() {
  const auto now = std::make_shared<std::int64_t>(1'762'387'200'000);
  const auto seed = std::make_shared<std::uint64_t>(0);
  return DocumentEnvironment{
      .device = DeviceId{Uuid{1, 1}},
      .unix_ms = [now] { return (*now)++; },
      .random_u64 = [seed] { return ++*seed * 0x9E37'79B9'7F4A'7C15ULL; },
  };
}

ElementId created(const Result<std::optional<ElementId>>& result) {
  EXPECT_TRUE(result.has_value() && result->has_value());
  return result.value_or(std::nullopt).value_or(ElementId{});
}

// Every kind of op: creates, each field of each element, deletions by a cascade, undo and redo.
Document edited_document() {
  Document document{test_environment()};
  const ElementId level =
      created(document.execute(AddLevel{.level = {.elevation = 0_mm, .height = 3000_mm}}));
  const ElementId wall = created(document.execute(AddWall{.wall = {.level = level,
                                                                   .start = {.x = 0_mm, .y = 0_mm},
                                                                   .end = {.x = 6400_mm, .y = 0_mm},
                                                                   .thickness = 380_mm,
                                                                   .height = 2800_mm}}));
  const ElementId window = created(document.execute(AddOpening{
      .opening = {
          .wall = wall, .offset = 1200_mm, .width = 1500_mm, .height = 1500_mm, .sill = 900_mm}}));
  const std::vector<Command> edits{
      SetLevelElevation{.level = level, .elevation = -450_mm},
      SetLevelHeight{.level = level, .height = 3300_mm},
      MoveWallEnd{.wall = wall, .end = WallEnd::kStart, .to = {.x = -100_mm, .y = 50_mm}},
      SetWallThickness{.wall = wall, .thickness = 510_mm},
      SetWallHeight{.wall = wall, .height = 2700_mm},
      SetWallLocationLine{.wall = wall, .location_line = LocationLine::kRightFace},
      MoveOpening{.opening = window, .offset = 1000_mm},
      ResizeOpening{.opening = window, .width = 900_mm, .height = 2100_mm},
      SetOpeningSill{.opening = window, .sill = 0_mm},
      SetOpeningKind{.opening = window, .kind = OpeningKind::kDoor},
      RemoveWall{.wall = wall},
  };
  for (const Command& edit : edits) {
    EXPECT_TRUE(document.execute(edit));
  }
  EXPECT_TRUE(document.undo());
  EXPECT_TRUE(document.redo());
  return document;
}

void expect_round_trip(const OpGroup& group) {
  const Result<OpGroup> decoded = decode_group(encode_group(group));
  ASSERT_TRUE(decoded.has_value()) << decoded.error().message;
  EXPECT_EQ(decoded->id, group.id);
  EXPECT_EQ(decoded->kind, group.kind);
  EXPECT_EQ(decoded->stamp, group.stamp);
  EXPECT_EQ(decoded->origin, group.origin);
  EXPECT_EQ(decoded->ops, group.ops);
}

TEST(OpLog, EveryGroupComesBackFromItsBuffer) {
  const Document document = edited_document();
  ASSERT_EQ(document.log().size(), 16U);

  for (const OpGroup& group : document.log()) {
    expect_round_trip(group);
  }
}

TEST(OpLog, ForeignBytesAreCorrupt) {
  std::vector<std::uint8_t> bytes = encode_group(edited_document().log().front());
  bytes[4] = 'X';  // the identifier
  const std::vector<std::uint8_t> garbage{1, 2, 3, 4, 5, 6, 7, 8};

  EXPECT_EQ(decode_group(bytes).error().code, ErrorCode::kCorruptData);
  EXPECT_EQ(decode_group(garbage).error().code, ErrorCode::kCorruptData);
}

TEST(OpLog, GroupFromANewerEngineIsUnsupported) {
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(schema::CreateOpGroup(builder, 2), kGroupIdentifier);
  const std::span<const std::uint8_t> bytes{builder.GetBufferPointer(), builder.GetSize()};

  EXPECT_EQ(decode_group(bytes).error().code, ErrorCode::kUnsupportedVersion);
}

}  // namespace
}  // namespace otves::bridge
