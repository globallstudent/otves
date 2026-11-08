#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include <otves/core/element.h>
#include <otves/core/hlc.h>
#include <otves/core/opening.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

namespace otves {

// Property-level changes. Every model change is a list of these; undo, history and sync read
// nothing else. Create and delete carry the whole value so either can be inverted.
template <typename T>
struct Create {
  ElementId id;
  T element;

  bool operator==(const Create&) const = default;
};

template <typename T>
struct Delete {
  ElementId id;
  T element;

  bool operator==(const Delete&) const = default;
};

template <typename T>
struct SetField {
  ElementId id;
  ElementTraits<T>::Field field{};
  ElementTraits<T>::Value before;
  ElementTraits<T>::Value after;

  bool operator==(const SetField&) const = default;
};

using CreateWall = Create<Wall>;
using DeleteWall = Delete<Wall>;
using SetWallField = SetField<Wall>;
using CreateOpening = Create<Opening>;
using DeleteOpening = Delete<Opening>;
using SetOpeningField = SetField<Opening>;

using Op = std::variant<CreateWall, DeleteWall, SetWallField, CreateOpening, DeleteOpening,
                        SetOpeningField>;

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
  kAddOpening,
  kMoveOpening,
  kResizeOpening,
  kSetOpeningSill,
  kSetOpeningKind,
  kRemoveOpening,
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
