#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include <otves/core/hlc.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

namespace otves {

// Property-level changes. Every model change is a list of these; undo, history and sync read
// nothing else. Create and delete carry the whole value so either can be inverted.
struct CreateWall {
  ElementId id;
  Wall wall;

  bool operator==(const CreateWall&) const = default;
};

struct DeleteWall {
  ElementId id;
  Wall wall;

  bool operator==(const DeleteWall&) const = default;
};

struct SetWallField {
  ElementId id;
  WallField field = WallField::kStart;
  WallValue before;
  WallValue after;

  bool operator==(const SetWallField&) const = default;
};

using Op = std::variant<CreateWall, DeleteWall, SetWallField>;

[[nodiscard]] Op inverse(const Op& op);
[[nodiscard]] ElementId target(const Op& op);

// What produced a group; the app turns it into a label such as «Стена добавлена».
enum class CommandKind : std::uint8_t {
  kAddWall,
  kMoveWallEnd,
  kSetWallThickness,
  kSetWallHeight,
  kSetWallLocationLine,
  kRemoveWall,
  kUndo,
  kRedo,
};

// Orders groups across devices: last writer wins per property.
struct Stamp {
  Hlc hlc;
  DeviceId device;

  bool operator==(const Stamp&) const = default;
};

// One entry of the append-only operation log. For kUndo and kRedo, `origin` names the group
// that was undone or redone; otherwise it is nil.
struct OpGroup {
  GroupId id;
  CommandKind kind = CommandKind::kAddWall;
  Stamp stamp;
  GroupId origin;
  std::vector<Op> ops;
};

}  // namespace otves
