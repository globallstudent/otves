#include "convert.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include <flatbuffers/flatbuffers.h>

#include <otves/core/command.h>
#include <otves/core/conditions.h>
#include <otves/core/document.h>
#include <otves/core/error.h>
#include <otves/core/hlc.h>
#include <otves/core/level.h>
#include <otves/core/model.h>
#include <otves/core/opening.h>
#include <otves/core/overloaded.h>
#include <otves/core/point.h>
#include <otves/core/room.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/core/wall_type.h>
#include <otves/drafting/plan.h>
#include <otves/geom/editing.h>

#include "engine_generated.h"

namespace otves::bridge {
namespace {

std::unexpected<Error> corrupt(const std::string& what) {
  return make_error(ErrorCode::kCorruptData, what);
}

// Every room kind with its code in the schema.
constexpr std::array<std::pair<RoomKind, schema::RoomKind>, 19> kRoomKinds{{
    {RoomKind::kRoom, schema::RoomKind::Room},
    {RoomKind::kLivingRoom, schema::RoomKind::LivingRoom},
    {RoomKind::kKitchenLiving, schema::RoomKind::KitchenLiving},
    {RoomKind::kKitchen, schema::RoomKind::Kitchen},
    {RoomKind::kDining, schema::RoomKind::Dining},
    {RoomKind::kBedroom, schema::RoomKind::Bedroom},
    {RoomKind::kChildren, schema::RoomKind::Children},
    {RoomKind::kStudy, schema::RoomKind::Study},
    {RoomKind::kHall, schema::RoomKind::Hall},
    {RoomKind::kCorridor, schema::RoomKind::Corridor},
    {RoomKind::kBathroom, schema::RoomKind::Bathroom},
    {RoomKind::kBath, schema::RoomKind::Bath},
    {RoomKind::kToilet, schema::RoomKind::Toilet},
    {RoomKind::kBoiler, schema::RoomKind::Boiler},
    {RoomKind::kStorage, schema::RoomKind::Storage},
    {RoomKind::kWardrobe, schema::RoomKind::Wardrobe},
    {RoomKind::kVeranda, schema::RoomKind::Veranda},
    {RoomKind::kGarage, schema::RoomKind::Garage},
    {RoomKind::kTambour, schema::RoomKind::Tambour},
}};

std::optional<WallEnd> to_wall_end(schema::WallEnd end) {
  switch (end) {
    case schema::WallEnd::Start:
      return WallEnd::kStart;
    case schema::WallEnd::End:
      return WallEnd::kEnd;
  }
  return std::nullopt;
}

std::expected<Level, Error> to_level(const schema::Level* level) {
  if (level == nullptr) {
    return corrupt("level is missing");
  }
  return Level{.elevation = Mm{level->elevation()},
               .height = Mm{level->height()},
               .ring_beam = Mm{level->ring_beam()},
               .junction_meshes = level->junction_meshes(),
               .footing_width = Mm{level->footing_width()},
               .footing_depth = Mm{level->footing_depth()}};
}

// A wall without its points: what a sketch builds its walls like.
std::expected<Wall, Error> to_wall_like(const schema::Wall* wall) {
  if (wall == nullptr) {
    return corrupt("wall is missing");
  }
  const std::optional<LocationLine> line = from_schema(wall->location_line());
  if (!line) {
    return corrupt("unknown location line");
  }
  return Wall{.level = to_id(wall->level()),
              .start = {},
              .end = {},
              .thickness = Mm{wall->thickness()},
              .height = Mm{wall->height()},
              .location_line = *line,
              .unit_length = Mm{wall->unit_length()},
              .unit_width = Mm{wall->unit_width()},
              .unit_height = Mm{wall->unit_height()},
              .exact = wall->exact(),
              .type = to_id(wall->type()),
              .mirrored = wall->mirrored()};
}

std::expected<Wall, Error> to_wall(const schema::Wall* wall) {
  if (wall == nullptr || wall->start() == nullptr || wall->end() == nullptr) {
    return corrupt("wall or its points are missing");
  }
  return to_wall_like(wall).transform([wall](Wall built) {
    built.start = to_point(*wall->start());
    built.end = to_point(*wall->end());
    return built;
  });
}

std::expected<WallType, Error> to_wall_type(const schema::WallType* type) {
  if (type == nullptr) {
    return corrupt("wall type is missing");
  }
  const std::optional<WallRole> role = from_schema(type->role());
  if (!role) {
    return corrupt("unknown wall role");
  }
  return WallType{.role = *role};
}

std::expected<WallLayer, Error> to_wall_layer(const schema::WallLayer* layer) {
  if (layer == nullptr) {
    return corrupt("wall layer is missing");
  }
  const std::optional<LayerFunction> function = from_schema(layer->function());
  if (!function) {
    return corrupt("unknown layer function");
  }
  return WallLayer{.type = to_id(layer->type()),
                   .position = layer->position(),
                   .function = *function,
                   .thickness = Mm{layer->thickness()},
                   .unit_length = Mm{layer->unit_length()},
                   .unit_width = Mm{layer->unit_width()},
                   .unit_height = Mm{layer->unit_height()}};
}

std::expected<Room, Error> to_room(const schema::Room* room) {
  if (room == nullptr || room->anchor() == nullptr) {
    return corrupt("room or its anchor is missing");
  }
  const std::optional<RoomKind> kind = from_schema(room->kind());
  if (!kind) {
    return corrupt("unknown room kind");
  }
  return Room{.level = to_id(room->level()), .anchor = to_point(*room->anchor()), .kind = *kind};
}

std::expected<Command, Error> decode_room(const schema::CommandEntry& entry) {
  switch (entry.command_type()) {
    case schema::Command::AddRoom:
      return to_room(entry.command_as_AddRoom()->room()).transform([](const Room& room) {
        return Command{AddRoom{.room = room}};
      });
    case schema::Command::MoveRoom: {
      const auto* move = entry.command_as_MoveRoom();
      if (move->anchor() == nullptr) {
        return corrupt("anchor is missing");
      }
      return MoveRoom{.room = to_id(move->room()), .anchor = to_point(*move->anchor())};
    }
    case schema::Command::SetRoomKind: {
      const auto* set = entry.command_as_SetRoomKind();
      const std::optional<RoomKind> kind = from_schema(set->kind());
      if (!kind) {
        return corrupt("unknown room kind");
      }
      return SetRoomKind{.room = to_id(set->room()), .kind = *kind};
    }
    case schema::Command::RemoveRoom:
      return RemoveRoom{.room = to_id(entry.command_as_RemoveRoom()->room())};
    default:
      return corrupt("not a room command");
  }
}

std::expected<Opening, Error> to_opening(const schema::Opening* opening) {
  if (opening == nullptr) {
    return corrupt("opening is missing");
  }
  const std::optional<OpeningKind> kind = from_schema(opening->kind());
  const std::optional<Hinge> hinge = from_schema(opening->hinge());
  const std::optional<Swing> swing = from_schema(opening->swing());
  if (!kind || !hinge || !swing) {
    return corrupt("unknown opening kind, hinge or swing");
  }
  return Opening{.wall = to_id(opening->wall()),
                 .offset = Mm{opening->offset()},
                 .width = Mm{opening->width()},
                 .height = Mm{opening->height()},
                 .sill = Mm{opening->sill()},
                 .kind = *kind,
                 .hinge = *hinge,
                 .swing = *swing};
}

std::expected<Conditions, Error> to_conditions(const schema::Conditions* conditions) {
  if (conditions == nullptr) {
    return corrupt("conditions are missing");
  }
  return Conditions{.seismic_intensity = conditions->seismic_intensity(),
                    .cement_grade = conditions->cement_grade(),
                    .mortar_grade = conditions->mortar_grade(),
                    .unit_waste = conditions->unit_waste(),
                    .mortar_waste = conditions->mortar_waste()};
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
    case schema::Command::SetLevelRingBeam: {
      const auto* set = entry.command_as_SetLevelRingBeam();
      return SetLevelRingBeam{.level = to_id(set->level()), .height = Mm{set->height()}};
    }
    case schema::Command::SetLevelJunctionMeshes: {
      const auto* set = entry.command_as_SetLevelJunctionMeshes();
      return SetLevelJunctionMeshes{.level = to_id(set->level()), .meshes = set->meshes()};
    }
    case schema::Command::SetLevelFooting: {
      const auto* set = entry.command_as_SetLevelFooting();
      return SetLevelFooting{
          .level = to_id(set->level()), .width = Mm{set->width()}, .depth = Mm{set->depth()}};
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
      const std::optional<WallEnd> end = to_wall_end(move->end());
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
    case schema::Command::SetWallUnit: {
      const auto* set = entry.command_as_SetWallUnit();
      return SetWallUnit{.wall = to_id(set->wall()),
                         .length = Mm{set->length()},
                         .width = Mm{set->width()},
                         .height = Mm{set->height()}};
    }
    case schema::Command::RemoveWall:
      return RemoveWall{.wall = to_id(entry.command_as_RemoveWall()->wall())};
    default:
      return corrupt("not a wall command");
  }
}

std::expected<Command, Error> decode_add_wall_type(const schema::AddWallType* add) {
  const std::optional<WallRole> role = from_schema(add->role());
  if (!role) {
    return corrupt("unknown wall role");
  }
  AddWallType command{.role = *role, .layers = {}};
  if (add->layers() != nullptr) {
    command.layers.reserve(add->layers()->size());
    for (const schema::WallLayer* layer : *add->layers()) {
      auto decoded = to_wall_layer(layer);
      if (!decoded) {
        return std::unexpected{std::move(decoded).error()};
      }
      command.layers.push_back(*decoded);
    }
  }
  return command;
}

std::expected<Command, Error> decode_wall_type(const schema::CommandEntry& entry) {
  switch (entry.command_type()) {
    case schema::Command::AddWallType:
      return decode_add_wall_type(entry.command_as_AddWallType());
    case schema::Command::SetWallTypeRole: {
      const auto* set = entry.command_as_SetWallTypeRole();
      const std::optional<WallRole> role = from_schema(set->role());
      if (!role) {
        return corrupt("unknown wall role");
      }
      return SetWallTypeRole{.type = to_id(set->type()), .role = *role};
    }
    case schema::Command::SetWallLayer: {
      const auto* set = entry.command_as_SetWallLayer();
      return SetWallLayer{.layer = to_id(set->layer()),
                          .thickness = Mm{set->thickness()},
                          .unit_length = Mm{set->unit_length()},
                          .unit_width = Mm{set->unit_width()},
                          .unit_height = Mm{set->unit_height()}};
    }
    case schema::Command::SetWallType: {
      const auto* set = entry.command_as_SetWallType();
      return SetWallType{.wall = to_id(set->wall()), .type = to_id(set->type())};
    }
    case schema::Command::SetWallMirrored: {
      const auto* set = entry.command_as_SetWallMirrored();
      return SetWallMirrored{.wall = to_id(set->wall()), .mirrored = set->mirrored()};
    }
    default:
      return corrupt("not a wall type command");
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
    case schema::Command::SetOpeningHinge: {
      const auto* set = entry.command_as_SetOpeningHinge();
      const std::optional<Hinge> hinge = from_schema(set->hinge());
      if (!hinge) {
        return corrupt("unknown hinge");
      }
      return SetOpeningHinge{.opening = to_id(set->opening()), .hinge = *hinge};
    }
    case schema::Command::SetOpeningSwing: {
      const auto* set = entry.command_as_SetOpeningSwing();
      const std::optional<Swing> swing = from_schema(set->swing());
      if (!swing) {
        return corrupt("unknown swing");
      }
      return SetOpeningSwing{.opening = to_id(set->opening()), .swing = *swing};
    }
    default:
      return corrupt("not an opening command");
  }
}

}  // namespace

schema::Uuid to_schema(Uuid uuid) { return {uuid.high(), uuid.low()}; }

schema::Uuid to_schema(ElementId id) { return to_schema(id.uuid()); }

Uuid to_uuid(const schema::Uuid* uuid) {
  return uuid == nullptr ? Uuid{} : Uuid{uuid->high(), uuid->low()};
}

ElementId to_id(const schema::Uuid* uuid) { return ElementId{to_uuid(uuid)}; }

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

schema::Hinge to_schema(Hinge hinge) {
  return hinge == Hinge::kNear ? schema::Hinge::Near : schema::Hinge::Far;
}

std::optional<Hinge> from_schema(schema::Hinge hinge) {
  switch (hinge) {
    case schema::Hinge::Near:
      return Hinge::kNear;
    case schema::Hinge::Far:
      return Hinge::kFar;
  }
  return std::nullopt;
}

schema::Swing to_schema(Swing swing) {
  return swing == Swing::kLeft ? schema::Swing::Left : schema::Swing::Right;
}

std::optional<Swing> from_schema(schema::Swing swing) {
  switch (swing) {
    case schema::Swing::Left:
      return Swing::kLeft;
    case schema::Swing::Right:
      return Swing::kRight;
  }
  return std::nullopt;
}

schema::RoomKind to_schema(RoomKind kind) {
  const auto* const found =
      std::ranges::find(kRoomKinds, kind, &std::pair<RoomKind, schema::RoomKind>::first);
  return found != kRoomKinds.end() ? found->second : schema::RoomKind::Room;
}

std::optional<RoomKind> from_schema(schema::RoomKind kind) {
  const auto* const found =
      std::ranges::find(kRoomKinds, kind, &std::pair<RoomKind, schema::RoomKind>::second);
  return found != kRoomKinds.end() ? std::optional{found->first} : std::nullopt;
}

schema::WallRole to_schema(WallRole role) {
  switch (role) {
    case WallRole::kExterior:
      return schema::WallRole::Exterior;
    case WallRole::kBearing:
      return schema::WallRole::Bearing;
    case WallRole::kPartition:
      return schema::WallRole::Partition;
  }
  return schema::WallRole::Exterior;
}

std::optional<WallRole> from_schema(schema::WallRole role) {
  switch (role) {
    case schema::WallRole::Exterior:
      return WallRole::kExterior;
    case schema::WallRole::Bearing:
      return WallRole::kBearing;
    case schema::WallRole::Partition:
      return WallRole::kPartition;
  }
  return std::nullopt;
}

schema::LayerFunction to_schema(LayerFunction function) {
  switch (function) {
    case LayerFunction::kStructure:
      return schema::LayerFunction::Structure;
    case LayerFunction::kInsulation:
      return schema::LayerFunction::Insulation;
    case LayerFunction::kFacing:
      return schema::LayerFunction::Facing;
    case LayerFunction::kGap:
      return schema::LayerFunction::Gap;
  }
  return schema::LayerFunction::Structure;
}

std::optional<LayerFunction> from_schema(schema::LayerFunction function) {
  switch (function) {
    case schema::LayerFunction::Structure:
      return LayerFunction::kStructure;
    case schema::LayerFunction::Insulation:
      return LayerFunction::kInsulation;
    case schema::LayerFunction::Facing:
      return LayerFunction::kFacing;
    case schema::LayerFunction::Gap:
      return LayerFunction::kGap;
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

PointMm to_point(const schema::PointMm& point) { return {.x = Mm{point.x()}, .y = Mm{point.y()}}; }

schema::PointMm to_schema(PointMm point) { return {point.x.value(), point.y.value()}; }

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
    case ErrorCode::kNoWall:
      return schema::ErrorCode::NoWall;
    case ErrorCode::kDoesNotFit:
      return schema::ErrorCode::DoesNotFit;
    case ErrorCode::kConditionOutOfRange:
      return schema::ErrorCode::ConditionOutOfRange;
    case ErrorCode::kBreaksJoin:
      return schema::ErrorCode::BreaksJoin;
    case ErrorCode::kTypeDecides:
      return schema::ErrorCode::TypeDecides;
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
    case schema::Command::SetLevelRingBeam:
    case schema::Command::SetLevelJunctionMeshes:
    case schema::Command::SetLevelFooting:
      return decode_level(entry);
    case schema::Command::AddWall:
    case schema::Command::MoveWallEnd:
    case schema::Command::SetWallThickness:
    case schema::Command::SetWallHeight:
    case schema::Command::SetWallLocationLine:
    case schema::Command::RemoveWall:
    case schema::Command::SetWallUnit:
      return decode_wall(entry);
    case schema::Command::AddWallType:
    case schema::Command::SetWallTypeRole:
    case schema::Command::SetWallLayer:
    case schema::Command::SetWallType:
    case schema::Command::SetWallMirrored:
      return decode_wall_type(entry);
    case schema::Command::AddOpening:
    case schema::Command::MoveOpening:
    case schema::Command::ResizeOpening:
    case schema::Command::SetOpeningSill:
    case schema::Command::SetOpeningKind:
    case schema::Command::RemoveOpening:
    case schema::Command::SetOpeningHinge:
    case schema::Command::SetOpeningSwing:
      return decode_opening(entry);
    case schema::Command::AddRoom:
    case schema::Command::MoveRoom:
    case schema::Command::SetRoomKind:
    case schema::Command::RemoveRoom:
      return decode_room(entry);
    case schema::Command::SetCondition: {
      const auto* set = entry.command_as_SetCondition();
      const std::optional<ConditionsField> field = from_schema(set->field());
      if (!field) {
        return corrupt("unknown condition");
      }
      return SetCondition{.field = *field, .value = set->value()};
    }
    default:
      return corrupt("not a model command");
  }
}

std::expected<Tap, Error> decode(const schema::Pick* pick) {
  if (pick == nullptr || pick->at() == nullptr) {
    return corrupt("pick or its point is missing");
  }
  return Tap{
      .level = to_id(pick->level()), .at = to_point(*pick->at()), .reach = Mm{pick->reach()}};
}

std::expected<OpeningPlacement, Error> decode(const schema::PlaceOpening* place) {
  if (place == nullptr || place->at() == nullptr) {
    return corrupt("placement or its point is missing");
  }
  const std::optional<OpeningKind> kind = from_schema(place->kind());
  if (!kind) {
    return corrupt("unknown opening kind");
  }
  return OpeningPlacement{.level = to_id(place->level()),
                          .at = to_point(*place->at()),
                          .reach = Mm{place->reach()},
                          .opening = {.wall = {},
                                      .offset = {},
                                      .width = Mm{place->width()},
                                      .height = Mm{place->height()},
                                      .sill = Mm{place->sill()},
                                      .kind = *kind}};
}

std::expected<Sight, Error> decode(const schema::Aim* aim) {
  if (aim == nullptr || aim->at() == nullptr) {
    return corrupt("aim or its point is missing");
  }
  return Sight{
      .level = to_id(aim->level()),
      .aim = {.from = aim->from() != nullptr ? std::optional{to_point(*aim->from())} : std::nullopt,
              .at = to_point(*aim->at()),
              .reach = Mm{aim->reach()},
              .grid = Mm{aim->grid()},
              .square = aim->square()}};
}

std::expected<MeasuredSight, Error> decode(const schema::AimMeasured* aim) {
  if (aim == nullptr || aim->from() == nullptr) {
    return corrupt("measured wall or its start is missing");
  }
  geom::Turn turn = geom::Turn::kStraight;
  switch (aim->turn()) {
    case schema::Turn::Left:
      turn = geom::Turn::kLeft;
      break;
    case schema::Turn::Straight:
      break;
    case schema::Turn::Right:
      turn = geom::Turn::kRight;
      break;
    default:
      return corrupt("unknown turn");
  }
  return MeasuredSight{
      .level = to_id(aim->level()),
      .measured = {.from = to_point(*aim->from()),
                   .before = aim->before() != nullptr ? std::optional{to_point(*aim->before())}
                                                      : std::nullopt,
                   .turn = turn,
                   .length = Mm{aim->length()}}};
}

std::expected<Resize, Error> decode(const schema::SetWallLength* set) {
  if (set == nullptr) {
    return corrupt("wall length is missing");
  }
  Resize resize{.wall = to_id(set->wall()), .length = Mm{set->length()}, .fixed = std::nullopt};
  switch (set->fixed()) {
    case schema::FixedEnd::Joined:
      break;
    case schema::FixedEnd::Start:
      resize.fixed = WallEnd::kStart;
      break;
    case schema::FixedEnd::End:
      resize.fixed = WallEnd::kEnd;
      break;
    default:
      return corrupt("unknown fixed end");
  }
  return resize;
}

std::expected<Partition, Error> decode(const schema::AddWallAcross* add) {
  if (add == nullptr || add->at() == nullptr) {
    return corrupt("wall across or its point is missing");
  }
  geom::Across across{
      .in = to_point(*add->at()), .from = geom::Hand::kTop, .distance = Mm{add->distance()}};
  switch (add->from()) {
    case schema::Hand::Top:
      break;
    case schema::Hand::Bottom:
      across.from = geom::Hand::kBottom;
      break;
    case schema::Hand::Left:
      across.from = geom::Hand::kLeft;
      break;
    case schema::Hand::Right:
      across.from = geom::Hand::kRight;
      break;
    default:
      return corrupt("unknown hand");
  }
  return to_wall_like(add->wall()).transform([&across](const Wall& wall) {
    return Partition{.wall = wall, .across = across};
  });
}

std::expected<Stroke, Error> decode(const schema::AddSketch* sketch) {
  if (sketch == nullptr) {
    return corrupt("sketch is missing");
  }
  return to_wall_like(sketch->wall()).transform([sketch](const Wall& wall) {
    Stroke stroke{.wall = wall,
                  .points = {},
                  .reach = Mm{sketch->reach()},
                  .grid = Mm{sketch->grid()},
                  .square = sketch->square()};
    if (sketch->points() != nullptr) {
      stroke.points.reserve(sketch->points()->size());
      for (const schema::PointMm* point : *sketch->points()) {
        stroke.points.push_back(to_point(*point));
      }
    }
    return stroke;
  });
}

std::expected<View, Error> decode(const schema::Draw* draw) {
  if (draw == nullptr) {
    return corrupt("draw is missing");
  }
  // A size is never negative and never larger than the plan.
  const auto size = [](std::int64_t millimetres) {
    return to_metres(std::clamp(Mm{millimetres}, Mm{0}, kMaxCoordinate));
  };
  return View{
      .level = to_id(draw->level()),
      .text = {.height = size(draw->text_height()), .digit = size(draw->digit_width())},
      .focus = draw->focus() != nullptr ? std::optional{to_point(*draw->focus())} : std::nullopt,
      .selected = to_id(draw->selected()),
      .sheet = draw->sheet()};
}

std::expected<Sized, Error> decode(const schema::SetDimension* set) {
  if (set == nullptr) {
    return corrupt("size is missing");
  }
  Sized sized{.element = to_id(set->element()),
              .from = Mm{set->from()},
              .forward = set->forward(),
              .value = Mm{set->value()},
              .other = to_id(set->other())};
  switch (set->kind()) {
    case schema::TextKind::Length:
    case schema::TextKind::AboutLength:
      sized.kind = drafting::TextKind::kLength;
      return sized;
    case schema::TextKind::Pier:
      sized.kind = drafting::TextKind::kPier;
      return sized;
    case schema::TextKind::OpeningWidth:
      sized.kind = drafting::TextKind::kOpeningWidth;
      return sized;
    case schema::TextKind::Clear:
      sized.kind = drafting::TextKind::kClear;
      return sized;
    default:
      return corrupt("not a size a typed value changes");
  }
}

EncodedElement encode(flatbuffers::FlatBufferBuilder& builder, const Level& level) {
  return {.type = schema::Element::Level,
          .value = schema::CreateLevel(builder, level.elevation.value(), level.height.value(),
                                       level.ring_beam.value(), level.junction_meshes,
                                       level.footing_width.value(), level.footing_depth.value())
                       .Union()};
}

EncodedElement encode(flatbuffers::FlatBufferBuilder& builder, const Wall& wall) {
  const schema::Uuid level = to_schema(wall.level);
  const schema::PointMm start = to_schema(wall.start);
  const schema::PointMm end = to_schema(wall.end);
  const schema::Uuid type = to_schema(wall.type);
  return {.type = schema::Element::Wall,
          .value = schema::CreateWall(builder, &level, &start, &end, wall.thickness.value(),
                                      wall.height.value(), to_schema(wall.location_line),
                                      wall.unit_length.value(), wall.unit_width.value(),
                                      wall.unit_height.value(), wall.exact,
                                      wall.type.is_nil() ? nullptr : &type, wall.mirrored)
                       .Union()};
}

EncodedElement encode(flatbuffers::FlatBufferBuilder& builder, const WallType& type) {
  return {.type = schema::Element::WallType,
          .value = schema::CreateWallType(builder, to_schema(type.role)).Union()};
}

EncodedElement encode(flatbuffers::FlatBufferBuilder& builder, const WallLayer& layer) {
  const schema::Uuid type = to_schema(layer.type);
  return {
      .type = schema::Element::WallLayer,
      .value = schema::CreateWallLayer(builder, &type, layer.position, to_schema(layer.function),
                                       layer.thickness.value(), layer.unit_length.value(),
                                       layer.unit_width.value(), layer.unit_height.value())
                   .Union()};
}

EncodedElement encode(flatbuffers::FlatBufferBuilder& builder, const Opening& opening) {
  const schema::Uuid wall = to_schema(opening.wall);
  return {.type = schema::Element::Opening,
          .value = schema::CreateOpening(builder, &wall, opening.offset.value(),
                                         opening.width.value(), opening.height.value(),
                                         opening.sill.value(), to_schema(opening.kind),
                                         to_schema(opening.hinge), to_schema(opening.swing))
                       .Union()};
}

EncodedElement encode(flatbuffers::FlatBufferBuilder& builder, const Room& room) {
  const schema::Uuid level = to_schema(room.level);
  const schema::PointMm anchor = to_schema(room.anchor);
  return {.type = schema::Element::Room,
          .value = schema::CreateRoom(builder, &level, &anchor, to_schema(room.kind)).Union()};
}

EncodedElement encode(flatbuffers::FlatBufferBuilder& builder, const Conditions& conditions) {
  return {.type = schema::Element::Conditions,
          .value = schema::CreateConditions(builder, conditions.seismic_intensity,
                                            conditions.cement_grade, conditions.mortar_grade,
                                            conditions.unit_waste, conditions.mortar_waste)
                       .Union()};
}

schema::ConditionsField to_schema(ConditionsField field) {
  switch (field) {
    case ConditionsField::kSeismicIntensity:
      return schema::ConditionsField::SeismicIntensity;
    case ConditionsField::kCementGrade:
      return schema::ConditionsField::CementGrade;
    case ConditionsField::kMortarGrade:
      return schema::ConditionsField::MortarGrade;
    case ConditionsField::kUnitWaste:
      return schema::ConditionsField::UnitWaste;
    case ConditionsField::kMortarWaste:
      return schema::ConditionsField::MortarWaste;
  }
  return schema::ConditionsField::SeismicIntensity;
}

std::optional<ConditionsField> from_schema(schema::ConditionsField field) {
  switch (field) {
    case schema::ConditionsField::SeismicIntensity:
      return ConditionsField::kSeismicIntensity;
    case schema::ConditionsField::CementGrade:
      return ConditionsField::kCementGrade;
    case schema::ConditionsField::MortarGrade:
      return ConditionsField::kMortarGrade;
    case schema::ConditionsField::UnitWaste:
      return ConditionsField::kUnitWaste;
    case schema::ConditionsField::MortarWaste:
      return ConditionsField::kMortarWaste;
  }
  return std::nullopt;
}

std::expected<Element, Error> decode(schema::Element type, const void* value) {
  if (value == nullptr) {
    return corrupt("element is missing");
  }
  switch (type) {
    case schema::Element::Level:
      return to_level(static_cast<const schema::Level*>(value));
    case schema::Element::Wall:
      return to_wall(static_cast<const schema::Wall*>(value));
    case schema::Element::Opening:
      return to_opening(static_cast<const schema::Opening*>(value));
    case schema::Element::Conditions:
      return to_conditions(static_cast<const schema::Conditions*>(value));
    case schema::Element::WallType:
      return to_wall_type(static_cast<const schema::WallType*>(value));
    case schema::Element::WallLayer:
      return to_wall_layer(static_cast<const schema::WallLayer*>(value));
    case schema::Element::Room:
      return to_room(static_cast<const schema::Room*>(value));
    case schema::Element::NONE:
      break;
  }
  return corrupt("unknown element type");
}

std::expected<Snapshot, Error> decode(const schema::OpenModel* open) {
  Snapshot snapshot;
  if (open == nullptr) {
    return corrupt("open command is missing");
  }
  snapshot.clock = Hlc{.value = open->clock()};
  if (open->elements() == nullptr) {
    return snapshot;
  }
  for (const schema::ElementChange* saved : *open->elements()) {
    auto element = decode(saved->element_type(), saved->element());
    if (!element) {
      return std::unexpected{std::move(element).error()};
    }
    const ElementId id = to_id(saved->id());
    std::visit(
        Overloaded{
            [&](const Level& level) { snapshot.levels.emplace_back(id, level); },
            [&](const Wall& wall) { snapshot.walls.emplace_back(id, wall); },
            [&](const Opening& opening) { snapshot.openings.emplace_back(id, opening); },
            [&](const Conditions& conditions) { snapshot.conditions.emplace_back(id, conditions); },
            [&](const WallType& type) { snapshot.wall_types.emplace_back(id, type); },
            [&](const WallLayer& layer) { snapshot.wall_layers.emplace_back(id, layer); },
            [&](const Room& room) { snapshot.rooms.emplace_back(id, room); },
        },
        *element);
  }
  return snapshot;
}

flatbuffers::Offset<schema::ElementChange> encode_change(flatbuffers::FlatBufferBuilder& builder,
                                                         const Model& model, ElementId id) {
  const schema::Uuid key = to_schema(id);
  EncodedElement value;
  if (const auto* level = model.find<Level>(id)) {
    value = encode(builder, *level);
  } else if (const auto* wall = model.find<Wall>(id)) {
    value = encode(builder, *wall);
  } else if (const auto* opening = model.find<Opening>(id)) {
    value = encode(builder, *opening);
  } else if (const auto* conditions = model.find<Conditions>(id)) {
    value = encode(builder, *conditions);
  } else if (const auto* type = model.find<WallType>(id)) {
    value = encode(builder, *type);
  } else if (const auto* layer = model.find<WallLayer>(id)) {
    value = encode(builder, *layer);
  } else if (const auto* room = model.find<Room>(id)) {
    value = encode(builder, *room);
  }
  return schema::CreateElementChange(builder, &key, value.type, value.value);
}

}  // namespace otves::bridge
