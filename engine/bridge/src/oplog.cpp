#include "oplog.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <flatbuffers/flatbuffers.h>

#include <otves/core/error.h>
#include <otves/core/hlc.h>
#include <otves/core/level.h>
#include <otves/core/op.h>
#include <otves/core/opening.h>
#include <otves/core/overloaded.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

#include "convert.h"
#include "engine_generated.h"

namespace otves::bridge {
namespace {

constexpr std::uint32_t kGroupVersion = 1;
constexpr std::size_t kGroupAlignment = 8;  // force_align of LogEntry.ops

std::unexpected<Error> corrupt(const std::string& what) {
  return make_error(ErrorCode::kCorruptData, what);
}

schema::CommandKind to_code(CommandKind kind) {
  switch (kind) {
    case CommandKind::kAddLevel:
      return schema::CommandKind::AddLevel;
    case CommandKind::kSetLevelElevation:
      return schema::CommandKind::SetLevelElevation;
    case CommandKind::kSetLevelHeight:
      return schema::CommandKind::SetLevelHeight;
    case CommandKind::kRemoveLevel:
      return schema::CommandKind::RemoveLevel;
    case CommandKind::kAddWall:
      return schema::CommandKind::AddWall;
    case CommandKind::kMoveWallEnd:
      return schema::CommandKind::MoveWallEnd;
    case CommandKind::kSetWallThickness:
      return schema::CommandKind::SetWallThickness;
    case CommandKind::kSetWallHeight:
      return schema::CommandKind::SetWallHeight;
    case CommandKind::kSetWallLocationLine:
      return schema::CommandKind::SetWallLocationLine;
    case CommandKind::kRemoveWall:
      return schema::CommandKind::RemoveWall;
    case CommandKind::kSetWallUnit:
      return schema::CommandKind::SetWallUnit;
    case CommandKind::kSetLevelRingBeam:
      return schema::CommandKind::SetLevelRingBeam;
    case CommandKind::kSetLevelJunctionMeshes:
      return schema::CommandKind::SetLevelJunctionMeshes;
    case CommandKind::kAddOpening:
      return schema::CommandKind::AddOpening;
    case CommandKind::kMoveOpening:
      return schema::CommandKind::MoveOpening;
    case CommandKind::kResizeOpening:
      return schema::CommandKind::ResizeOpening;
    case CommandKind::kSetOpeningSill:
      return schema::CommandKind::SetOpeningSill;
    case CommandKind::kSetOpeningKind:
      return schema::CommandKind::SetOpeningKind;
    case CommandKind::kRemoveOpening:
      return schema::CommandKind::RemoveOpening;
    case CommandKind::kUndo:
      return schema::CommandKind::Undo;
    case CommandKind::kRedo:
      return schema::CommandKind::Redo;
  }
  return schema::CommandKind::AddLevel;
}

std::optional<CommandKind> from_code(schema::CommandKind kind) {
  switch (kind) {
    case schema::CommandKind::AddLevel:
      return CommandKind::kAddLevel;
    case schema::CommandKind::SetLevelElevation:
      return CommandKind::kSetLevelElevation;
    case schema::CommandKind::SetLevelHeight:
      return CommandKind::kSetLevelHeight;
    case schema::CommandKind::RemoveLevel:
      return CommandKind::kRemoveLevel;
    case schema::CommandKind::AddWall:
      return CommandKind::kAddWall;
    case schema::CommandKind::MoveWallEnd:
      return CommandKind::kMoveWallEnd;
    case schema::CommandKind::SetWallThickness:
      return CommandKind::kSetWallThickness;
    case schema::CommandKind::SetWallHeight:
      return CommandKind::kSetWallHeight;
    case schema::CommandKind::SetWallLocationLine:
      return CommandKind::kSetWallLocationLine;
    case schema::CommandKind::RemoveWall:
      return CommandKind::kRemoveWall;
    case schema::CommandKind::AddOpening:
      return CommandKind::kAddOpening;
    case schema::CommandKind::MoveOpening:
      return CommandKind::kMoveOpening;
    case schema::CommandKind::ResizeOpening:
      return CommandKind::kResizeOpening;
    case schema::CommandKind::SetOpeningSill:
      return CommandKind::kSetOpeningSill;
    case schema::CommandKind::SetOpeningKind:
      return CommandKind::kSetOpeningKind;
    case schema::CommandKind::RemoveOpening:
      return CommandKind::kRemoveOpening;
    case schema::CommandKind::Undo:
      return CommandKind::kUndo;
    case schema::CommandKind::Redo:
      return CommandKind::kRedo;
    case schema::CommandKind::SetWallUnit:
      return CommandKind::kSetWallUnit;
    case schema::CommandKind::SetLevelRingBeam:
      return CommandKind::kSetLevelRingBeam;
    case schema::CommandKind::SetLevelJunctionMeshes:
      return CommandKind::kSetLevelJunctionMeshes;
  }
  return std::nullopt;
}

schema::LevelField to_code(LevelField field) {
  switch (field) {
    case LevelField::kElevation:
      return schema::LevelField::Elevation;
    case LevelField::kHeight:
      return schema::LevelField::Height;
    case LevelField::kRingBeam:
      return schema::LevelField::RingBeam;
    case LevelField::kJunctionMeshes:
      return schema::LevelField::JunctionMeshes;
  }
  return schema::LevelField::Elevation;
}

std::optional<LevelField> from_code(schema::LevelField field) {
  switch (field) {
    case schema::LevelField::Elevation:
      return LevelField::kElevation;
    case schema::LevelField::Height:
      return LevelField::kHeight;
    case schema::LevelField::RingBeam:
      return LevelField::kRingBeam;
    case schema::LevelField::JunctionMeshes:
      return LevelField::kJunctionMeshes;
  }
  return std::nullopt;
}

schema::WallField to_code(WallField field) {
  switch (field) {
    case WallField::kStart:
      return schema::WallField::Start;
    case WallField::kEnd:
      return schema::WallField::End;
    case WallField::kThickness:
      return schema::WallField::Thickness;
    case WallField::kHeight:
      return schema::WallField::Height;
    case WallField::kLocationLine:
      return schema::WallField::LocationLine;
    case WallField::kUnitLength:
      return schema::WallField::UnitLength;
    case WallField::kUnitWidth:
      return schema::WallField::UnitWidth;
    case WallField::kUnitHeight:
      return schema::WallField::UnitHeight;
  }
  return schema::WallField::Start;
}

std::optional<WallField> from_code(schema::WallField field) {
  switch (field) {
    case schema::WallField::Start:
      return WallField::kStart;
    case schema::WallField::End:
      return WallField::kEnd;
    case schema::WallField::Thickness:
      return WallField::kThickness;
    case schema::WallField::Height:
      return WallField::kHeight;
    case schema::WallField::LocationLine:
      return WallField::kLocationLine;
    case schema::WallField::UnitLength:
      return WallField::kUnitLength;
    case schema::WallField::UnitWidth:
      return WallField::kUnitWidth;
    case schema::WallField::UnitHeight:
      return WallField::kUnitHeight;
  }
  return std::nullopt;
}

schema::OpeningField to_code(OpeningField field) {
  switch (field) {
    case OpeningField::kOffset:
      return schema::OpeningField::Offset;
    case OpeningField::kWidth:
      return schema::OpeningField::Width;
    case OpeningField::kHeight:
      return schema::OpeningField::Height;
    case OpeningField::kSill:
      return schema::OpeningField::Sill;
    case OpeningField::kKind:
      return schema::OpeningField::Kind;
  }
  return schema::OpeningField::Offset;
}

std::optional<OpeningField> from_code(schema::OpeningField field) {
  switch (field) {
    case schema::OpeningField::Offset:
      return OpeningField::kOffset;
    case schema::OpeningField::Width:
      return OpeningField::kWidth;
    case schema::OpeningField::Height:
      return OpeningField::kHeight;
    case schema::OpeningField::Sill:
      return OpeningField::kSill;
    case schema::OpeningField::Kind:
      return OpeningField::kKind;
  }
  return std::nullopt;
}

// An enum stored as a number in before/after, read back only when it is one of the schema's.
template <typename Code>
std::optional<Code> code_of(std::int64_t value) {
  if (value < static_cast<std::int64_t>(Code::MIN) ||
      value > static_cast<std::int64_t>(Code::MAX)) {
    return std::nullopt;
  }
  return static_cast<Code>(value);
}

std::int64_t number(const LevelValue& value) {
  return std::visit(Overloaded{
                        [](Mm length) { return length.value(); },
                        [](bool on) { return std::int64_t{on ? 1 : 0}; },
                    },
                    value);
}

std::int64_t number(const WallValue& value) {
  return std::visit(
      Overloaded{
          [](PointMm) { return std::int64_t{0}; },
          [](Mm length) { return length.value(); },
          [](LocationLine line) { return static_cast<std::int64_t>(to_schema(line)); },
      },
      value);
}

std::optional<schema::PointMm> point(const WallValue& value) {
  if (const auto* at = std::get_if<PointMm>(&value)) {
    return to_schema(*at);
  }
  return std::nullopt;
}

std::int64_t number(const OpeningValue& value) {
  return std::visit(Overloaded{
                        [](Mm length) { return length.value(); },
                        [](OpeningKind kind) { return static_cast<std::int64_t>(to_schema(kind)); },
                    },
                    value);
}

template <typename T>
flatbuffers::Offset<schema::OpEntry> element_op(flatbuffers::FlatBufferBuilder& builder,
                                                ElementId id, const T& element, bool create) {
  const schema::Uuid key = to_schema(id);
  const EncodedElement value = encode(builder, element);
  if (create) {
    const auto op = schema::CreateCreateOp(builder, &key, value.type, value.value);
    return schema::CreateOpEntry(builder, schema::Op::CreateOp, op.Union());
  }
  const auto op = schema::CreateDeleteOp(builder, &key, value.type, value.value);
  return schema::CreateOpEntry(builder, schema::Op::DeleteOp, op.Union());
}

flatbuffers::Offset<schema::OpEntry> encode_op(flatbuffers::FlatBufferBuilder& builder,
                                               const Op& op) {
  return std::visit(
      Overloaded{
          [&builder]<typename T>(const Create<T>& create) {
            return element_op(builder, create.id, create.element, true);
          },
          [&builder]<typename T>(const Delete<T>& remove) {
            return element_op(builder, remove.id, remove.element, false);
          },
          [&builder](const SetLevelField& set) {
            const schema::Uuid key = to_schema(set.id);
            const auto value = schema::CreateSetLevelFieldOp(builder, &key, to_code(set.field),
                                                             number(set.before), number(set.after));
            return schema::CreateOpEntry(builder, schema::Op::SetLevelFieldOp, value.Union());
          },
          [&builder](const SetWallField& set) {
            const schema::Uuid key = to_schema(set.id);
            const std::optional<schema::PointMm> before = point(set.before);
            const std::optional<schema::PointMm> after = point(set.after);
            const auto value = schema::CreateSetWallFieldOp(
                builder, &key, to_code(set.field), number(set.before), number(set.after),
                before ? &*before : nullptr, after ? &*after : nullptr);
            return schema::CreateOpEntry(builder, schema::Op::SetWallFieldOp, value.Union());
          },
          [&builder](const SetOpeningField& set) {
            const schema::Uuid key = to_schema(set.id);
            const auto value = schema::CreateSetOpeningFieldOp(
                builder, &key, to_code(set.field), number(set.before), number(set.after));
            return schema::CreateOpEntry(builder, schema::Op::SetOpeningFieldOp, value.Union());
          },
      },
      op);
}

template <typename Table>
Result<Op> element_op(const Table* table, bool create) {
  if (table == nullptr) {
    return corrupt("op is missing");
  }
  auto element = decode(table->element_type(), table->element());
  if (!element) {
    return std::unexpected{std::move(element).error()};
  }
  const ElementId id = to_id(table->id());
  return std::visit(
      [id, create]<typename T>(const T& value) -> Op {
        if (create) {
          return Create<T>{.id = id, .element = value};
        }
        return Delete<T>{.id = id, .element = value};
      },
      *element);
}

Result<Op> level_op(const schema::SetLevelFieldOp* set) {
  if (set == nullptr) {
    return corrupt("op is missing");
  }
  const std::optional<LevelField> field = from_code(set->field());
  if (!field) {
    return corrupt("unknown level field");
  }
  const auto value = [&field](std::int64_t number) -> LevelValue {
    if (*field == LevelField::kJunctionMeshes) {
      return number != 0;
    }
    return Mm{number};
  };
  return SetLevelField{.id = to_id(set->id()),
                       .field = *field,
                       .before = value(set->before()),
                       .after = value(set->after())};
}

Result<WallValue> wall_value(WallField field, std::int64_t number, const schema::PointMm* at) {
  switch (field) {
    case WallField::kStart:
    case WallField::kEnd:
      if (at == nullptr) {
        return corrupt("wall point is missing");
      }
      return to_point(*at);
    case WallField::kThickness:
    case WallField::kHeight:
    case WallField::kUnitLength:
    case WallField::kUnitWidth:
    case WallField::kUnitHeight:
      return Mm{number};
    case WallField::kLocationLine: {
      const auto code = code_of<schema::LocationLine>(number);
      const std::optional<LocationLine> line = code ? from_schema(*code) : std::nullopt;
      if (!line) {
        return corrupt("unknown location line");
      }
      return *line;
    }
  }
  return corrupt("unknown wall field");
}

Result<Op> wall_op(const schema::SetWallFieldOp* set) {
  if (set == nullptr) {
    return corrupt("op is missing");
  }
  const std::optional<WallField> field = from_code(set->field());
  if (!field) {
    return corrupt("unknown wall field");
  }
  auto before = wall_value(*field, set->before(), set->before_point());
  auto after = wall_value(*field, set->after(), set->after_point());
  if (!before || !after) {
    return std::unexpected{!before ? std::move(before).error() : std::move(after).error()};
  }
  return SetWallField{.id = to_id(set->id()), .field = *field, .before = *before, .after = *after};
}

Result<OpeningValue> opening_value(OpeningField field, std::int64_t number) {
  if (field != OpeningField::kKind) {
    return Mm{number};
  }
  const auto code = code_of<schema::OpeningKind>(number);
  const std::optional<OpeningKind> kind = code ? from_schema(*code) : std::nullopt;
  if (!kind) {
    return corrupt("unknown opening kind");
  }
  return *kind;
}

Result<Op> opening_op(const schema::SetOpeningFieldOp* set) {
  if (set == nullptr) {
    return corrupt("op is missing");
  }
  const std::optional<OpeningField> field = from_code(set->field());
  if (!field) {
    return corrupt("unknown opening field");
  }
  auto before = opening_value(*field, set->before());
  auto after = opening_value(*field, set->after());
  if (!before || !after) {
    return std::unexpected{!before ? std::move(before).error() : std::move(after).error()};
  }
  return SetOpeningField{
      .id = to_id(set->id()), .field = *field, .before = *before, .after = *after};
}

Result<Op> decode_op(const schema::OpEntry& entry) {
  switch (entry.op_type()) {
    case schema::Op::CreateOp:
      return element_op(entry.op_as_CreateOp(), true);
    case schema::Op::DeleteOp:
      return element_op(entry.op_as_DeleteOp(), false);
    case schema::Op::SetLevelFieldOp:
      return level_op(entry.op_as_SetLevelFieldOp());
    case schema::Op::SetWallFieldOp:
      return wall_op(entry.op_as_SetWallFieldOp());
    case schema::Op::SetOpeningFieldOp:
      return opening_op(entry.op_as_SetOpeningFieldOp());
    case schema::Op::NONE:
      break;
  }
  return corrupt("unknown op");
}

}  // namespace

std::vector<std::uint8_t> encode_group(const OpGroup& group) {
  flatbuffers::FlatBufferBuilder builder;
  std::vector<flatbuffers::Offset<schema::OpEntry>> ops;
  ops.reserve(group.ops.size());
  for (const Op& op : group.ops) {
    ops.push_back(encode_op(builder, op));
  }
  const schema::Uuid id = to_schema(group.id.uuid());
  const schema::Uuid device = to_schema(group.stamp.device.uuid());
  const schema::Uuid origin = to_schema(group.origin.uuid());
  const auto root =
      schema::CreateOpGroup(builder, kGroupVersion, &id, to_code(group.kind), group.stamp.hlc.value,
                            &device, &origin, builder.CreateVector(ops));
  builder.Finish(root, kGroupIdentifier);
  const std::span<const std::uint8_t> bytes{builder.GetBufferPointer(), builder.GetSize()};
  return {bytes.begin(), bytes.end()};
}

Result<OpGroup> decode_group(std::span<const std::uint8_t> bytes) {
  // A buffer must start on an 8-byte boundary for its 64-bit fields; a fresh vector does.
  const std::vector<std::uint8_t> aligned{bytes.begin(), bytes.end()};
  flatbuffers::Verifier verifier(aligned.data(), aligned.size());
  if (!verifier.VerifyBuffer<schema::OpGroup>(kGroupIdentifier)) {
    return corrupt("op group failed verification");
  }
  const auto* group = flatbuffers::GetRoot<schema::OpGroup>(aligned.data());
  if (group->version() != kGroupVersion) {
    return make_error(ErrorCode::kUnsupportedVersion,
                      "op group version " + std::to_string(group->version()));
  }
  const std::optional<CommandKind> kind = from_code(group->kind());
  if (!kind) {
    return corrupt("unknown command kind");
  }
  OpGroup decoded{
      .id = GroupId{to_uuid(group->id())},
      .kind = *kind,
      .stamp = {.hlc = Hlc{.value = group->hlc()}, .device = DeviceId{to_uuid(group->device())}},
      .origin = GroupId{to_uuid(group->origin())},
      .ops = {}};
  if (group->ops() != nullptr) {
    decoded.ops.reserve(group->ops()->size());
    for (const schema::OpEntry* entry : *group->ops()) {
      auto op = decode_op(*entry);
      if (!op) {
        return std::unexpected{std::move(op).error()};
      }
      decoded.ops.push_back(*std::move(op));
    }
  }
  return decoded;
}

flatbuffers::Offset<schema::LogEntry> encode_log_entry(flatbuffers::FlatBufferBuilder& builder,
                                                       const OpGroup& group) {
  const std::vector<std::uint8_t> ops = encode_group(group);
  builder.ForceVectorAlignment(ops.size(), sizeof(std::uint8_t), kGroupAlignment);
  const auto bytes = builder.CreateVector(ops);
  const schema::Uuid id = to_schema(group.id.uuid());
  const schema::Uuid device = to_schema(group.stamp.device.uuid());
  const schema::Uuid origin = to_schema(group.origin.uuid());
  return schema::CreateLogEntry(builder, &id, to_code(group.kind), group.stamp.hlc.value, &device,
                                &origin, bytes);
}

}  // namespace otves::bridge
