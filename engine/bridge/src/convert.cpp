#include "convert.h"

#include <expected>
#include <optional>
#include <string>

#include <flatbuffers/flatbuffers.h>

#include <otves/core/command.h>
#include <otves/core/error.h>
#include <otves/core/level.h>
#include <otves/core/model.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

#include "engine_generated.h"

namespace otves::bridge {
namespace {

std::unexpected<Error> corrupt(const std::string& what) {
  return make_error(ErrorCode::kCorruptData, what);
}

std::optional<LocationLine> from_schema(schema::LocationLine line) {
  switch (line) {
    case schema::LocationLine::Center:
      return LocationLine::kCenter;
    case schema::LocationLine::LeftFace:
      return LocationLine::kLeftFace;
    case schema::LocationLine::RightFace:
      return LocationLine::kRightFace;
  }
  return std::nullopt;
}

std::optional<OpeningKind> from_schema(schema::OpeningKind kind) {
  switch (kind) {
    case schema::OpeningKind::Window:
      return OpeningKind::kWindow;
    case schema::OpeningKind::Door:
      return OpeningKind::kDoor;
    case schema::OpeningKind::Empty:
      return OpeningKind::kEmpty;
  }
  return std::nullopt;
}

std::optional<WallEnd> from_schema(schema::WallEnd end) {
  switch (end) {
    case schema::WallEnd::Start:
      return WallEnd::kStart;
    case schema::WallEnd::End:
      return WallEnd::kEnd;
  }
  return std::nullopt;
}

schema::LocationLine to_schema(LocationLine line) {
  switch (line) {
    case LocationLine::kCenter:
      return schema::LocationLine::Center;
    case LocationLine::kLeftFace:
      return schema::LocationLine::LeftFace;
    case LocationLine::kRightFace:
      return schema::LocationLine::RightFace;
  }
  return schema::LocationLine::Center;
}

schema::OpeningKind to_schema(OpeningKind kind) {
  switch (kind) {
    case OpeningKind::kWindow:
      return schema::OpeningKind::Window;
    case OpeningKind::kDoor:
      return schema::OpeningKind::Door;
    case OpeningKind::kEmpty:
      return schema::OpeningKind::Empty;
  }
  return schema::OpeningKind::Window;
}

PointMm to_point(const schema::PointMm& point) { return {.x = Mm{point.x()}, .y = Mm{point.y()}}; }

schema::PointMm to_schema(PointMm point) { return {point.x.value(), point.y.value()}; }

std::expected<Level, Error> to_level(const schema::Level* level) {
  if (level == nullptr) {
    return corrupt("level is missing");
  }
  return Level{.elevation = Mm{level->elevation()}, .height = Mm{level->height()}};
}

std::expected<Wall, Error> to_wall(const schema::Wall* wall) {
  if (wall == nullptr || wall->start() == nullptr || wall->end() == nullptr) {
    return corrupt("wall or its points are missing");
  }
  const std::optional<LocationLine> line = from_schema(wall->location_line());
  if (!line) {
    return corrupt("unknown location line");
  }
  return Wall{.level = to_id(wall->level()),
              .start = to_point(*wall->start()),
              .end = to_point(*wall->end()),
              .thickness = Mm{wall->thickness()},
              .height = Mm{wall->height()},
              .location_line = *line};
}

std::expected<Opening, Error> to_opening(const schema::Opening* opening) {
  if (opening == nullptr) {
    return corrupt("opening is missing");
  }
  const std::optional<OpeningKind> kind = from_schema(opening->kind());
  if (!kind) {
    return corrupt("unknown opening kind");
  }
  return Opening{.wall = to_id(opening->wall()),
                 .offset = Mm{opening->offset()},
                 .width = Mm{opening->width()},
                 .height = Mm{opening->height()},
                 .sill = Mm{opening->sill()},
                 .kind = *kind};
}

std::expected<Command, Error> decode_level(const schema::CommandEntry& entry) {
  switch (entry.command_type()) {
    case schema::Command::AddLevel:
      return to_level(entry.command_as_AddLevel()->level()).transform([](const Level& level) {
        return Command{AddLevel{.level = level}};
      });
    case schema::Command::SetLevelElevation: {
      const auto* set = entry.command_as_SetLevelElevation();
      return SetLevelElevation{.level = to_id(set->level()), .elevation = Mm{set->elevation()}};
    }
    case schema::Command::SetLevelHeight: {
      const auto* set = entry.command_as_SetLevelHeight();
      return SetLevelHeight{.level = to_id(set->level()), .height = Mm{set->height()}};
    }
    case schema::Command::RemoveLevel:
      return RemoveLevel{.level = to_id(entry.command_as_RemoveLevel()->level())};
    default:
      return corrupt("not a level command");
  }
}

std::expected<Command, Error> decode_wall(const schema::CommandEntry& entry) {
  switch (entry.command_type()) {
    case schema::Command::AddWall:
      return to_wall(entry.command_as_AddWall()->wall()).transform([](const Wall& wall) {
        return Command{AddWall{.wall = wall}};
      });
    case schema::Command::MoveWallEnd: {
      const auto* move = entry.command_as_MoveWallEnd();
      const std::optional<WallEnd> end = from_schema(move->end());
      if (!end || move->to() == nullptr) {
        return corrupt("unknown wall end or missing point");
      }
      return MoveWallEnd{.wall = to_id(move->wall()), .end = *end, .to = to_point(*move->to())};
    }
    case schema::Command::SetWallThickness: {
      const auto* set = entry.command_as_SetWallThickness();
      return SetWallThickness{.wall = to_id(set->wall()), .thickness = Mm{set->thickness()}};
    }
    case schema::Command::SetWallHeight: {
      const auto* set = entry.command_as_SetWallHeight();
      return SetWallHeight{.wall = to_id(set->wall()), .height = Mm{set->height()}};
    }
    case schema::Command::SetWallLocationLine: {
      const auto* set = entry.command_as_SetWallLocationLine();
      const std::optional<LocationLine> line = from_schema(set->location_line());
      if (!line) {
        return corrupt("unknown location line");
      }
      return SetWallLocationLine{.wall = to_id(set->wall()), .location_line = *line};
    }
    case schema::Command::RemoveWall:
      return RemoveWall{.wall = to_id(entry.command_as_RemoveWall()->wall())};
    default:
      return corrupt("not a wall command");
  }
}

std::expected<Command, Error> decode_opening(const schema::CommandEntry& entry) {
  switch (entry.command_type()) {
    case schema::Command::AddOpening:
      return to_opening(entry.command_as_AddOpening()->opening())
          .transform(
              [](const Opening& opening) { return Command{AddOpening{.opening = opening}}; });
    case schema::Command::MoveOpening: {
      const auto* move = entry.command_as_MoveOpening();
      return MoveOpening{.opening = to_id(move->opening()), .offset = Mm{move->offset()}};
    }
    case schema::Command::ResizeOpening: {
      const auto* resize = entry.command_as_ResizeOpening();
      return ResizeOpening{.opening = to_id(resize->opening()),
                           .width = Mm{resize->width()},
                           .height = Mm{resize->height()}};
    }
    case schema::Command::SetOpeningSill: {
      const auto* set = entry.command_as_SetOpeningSill();
      return SetOpeningSill{.opening = to_id(set->opening()), .sill = Mm{set->sill()}};
    }
    case schema::Command::SetOpeningKind: {
      const auto* set = entry.command_as_SetOpeningKind();
      const std::optional<OpeningKind> kind = from_schema(set->kind());
      if (!kind) {
        return corrupt("unknown opening kind");
      }
      return SetOpeningKind{.opening = to_id(set->opening()), .kind = *kind};
    }
    case schema::Command::RemoveOpening:
      return RemoveOpening{.opening = to_id(entry.command_as_RemoveOpening()->opening())};
    default:
      return corrupt("not an opening command");
  }
}

}  // namespace

schema::Uuid to_schema(ElementId id) { return {id.uuid().high(), id.uuid().low()}; }

ElementId to_id(const schema::Uuid* uuid) {
  return uuid == nullptr ? ElementId{} : ElementId{Uuid{uuid->high(), uuid->low()}};
}

schema::ErrorCode to_schema(ErrorCode code) {
  switch (code) {
    case ErrorCode::kNotFound:
      return schema::ErrorCode::NotFound;
    case ErrorCode::kOffPlan:
      return schema::ErrorCode::OffPlan;
    case ErrorCode::kZeroLength:
      return schema::ErrorCode::ZeroLength;
    case ErrorCode::kThicknessNotPositive:
      return schema::ErrorCode::ThicknessNotPositive;
    case ErrorCode::kHeightNotPositive:
      return schema::ErrorCode::HeightNotPositive;
    case ErrorCode::kWidthNotPositive:
      return schema::ErrorCode::WidthNotPositive;
    case ErrorCode::kOffsetNegative:
      return schema::ErrorCode::OffsetNegative;
    case ErrorCode::kSillNegative:
      return schema::ErrorCode::SillNegative;
    case ErrorCode::kBearingNegative:
      return schema::ErrorCode::BearingNegative;
    case ErrorCode::kUnitSizeNotPositive:
      return schema::ErrorCode::UnitSizeNotPositive;
    case ErrorCode::kJointNegative:
      return schema::ErrorCode::JointNegative;
    case ErrorCode::kRuleSyntax:
      return schema::ErrorCode::RuleSyntax;
    case ErrorCode::kRuleFailed:
      return schema::ErrorCode::RuleFailed;
    case ErrorCode::kRuleOutput:
      return schema::ErrorCode::RuleOutput;
    case ErrorCode::kGestureOpen:
      return schema::ErrorCode::GestureOpen;
    case ErrorCode::kNothingToUndo:
      return schema::ErrorCode::NothingToUndo;
    case ErrorCode::kNothingToRedo:
      return schema::ErrorCode::NothingToRedo;
    case ErrorCode::kCorruptData:
      return schema::ErrorCode::CorruptData;
    case ErrorCode::kUnsupportedVersion:
      return schema::ErrorCode::UnsupportedVersion;
  }
  return schema::ErrorCode::Internal;
}

std::expected<Command, Error> decode(const schema::CommandEntry& entry) {
  // The verifier accepts a union whose type is set but whose table is absent.
  if (entry.command() == nullptr) {
    return corrupt("command is missing");
  }
  switch (entry.command_type()) {
    case schema::Command::AddLevel:
    case schema::Command::SetLevelElevation:
    case schema::Command::SetLevelHeight:
    case schema::Command::RemoveLevel:
      return decode_level(entry);
    case schema::Command::AddWall:
    case schema::Command::MoveWallEnd:
    case schema::Command::SetWallThickness:
    case schema::Command::SetWallHeight:
    case schema::Command::SetWallLocationLine:
    case schema::Command::RemoveWall:
      return decode_wall(entry);
    case schema::Command::AddOpening:
    case schema::Command::MoveOpening:
    case schema::Command::ResizeOpening:
    case schema::Command::SetOpeningSill:
    case schema::Command::SetOpeningKind:
    case schema::Command::RemoveOpening:
      return decode_opening(entry);
    default:
      return corrupt("not a model command");
  }
}

flatbuffers::Offset<schema::ElementChange> encode_change(flatbuffers::FlatBufferBuilder& builder,
                                                         const Model& model, ElementId id) {
  const schema::Uuid key = to_schema(id);
  if (const auto* level = model.find<Level>(id)) {
    const auto value =
        schema::CreateLevel(builder, level->elevation.value(), level->height.value());
    return schema::CreateElementChange(builder, &key, schema::Element::Level, value.Union());
  }
  if (const auto* wall = model.find<Wall>(id)) {
    const schema::Uuid level = to_schema(wall->level);
    const schema::PointMm start = to_schema(wall->start);
    const schema::PointMm end = to_schema(wall->end);
    const auto value = schema::CreateWall(builder, &level, &start, &end, wall->thickness.value(),
                                          wall->height.value(), to_schema(wall->location_line));
    return schema::CreateElementChange(builder, &key, schema::Element::Wall, value.Union());
  }
  if (const auto* opening = model.find<Opening>(id)) {
    const schema::Uuid wall = to_schema(opening->wall);
    const auto value = schema::CreateOpening(builder, &wall, opening->offset.value(),
                                             opening->width.value(), opening->height.value(),
                                             opening->sill.value(), to_schema(opening->kind));
    return schema::CreateElementChange(builder, &key, schema::Element::Opening, value.Union());
  }
  return schema::CreateElementChange(builder, &key);
}

}  // namespace otves::bridge
