#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <variant>

#include <otves/core/element.h>
#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/uuid.h>

namespace otves {

// What a room is used as. The app writes the name in the builder's language.
enum class RoomKind : std::uint8_t {
  kRoom,
  kLivingRoom,
  kKitchenLiving,
  kKitchen,
  kDining,
  kBedroom,
  kChildren,
  kStudy,
  kHall,
  kCorridor,
  kBathroom,  // a bath and a toilet in one room (санузел)
  kBath,
  kToilet,
  kBoiler,
  kStorage,
  kWardrobe,
  kVeranda,
  kGarage,
  kTambour,
};

// A name for a space the walls of a level close (docs/adr/0015). The space that holds `anchor`
// is this room. The model keeps the point the user set: whether it still lies in a space is
// checked on derived geometry, because the walls may change later.
struct Room {
  ElementId level;
  PointMm anchor;
  RoomKind kind = RoomKind::kRoom;

  constexpr bool operator==(const Room&) const = default;
};

// Checks values that come from the user; never asserts. Document checks that the level exists.
[[nodiscard]] std::expected<void, Error> validate(const Room& room);

enum class RoomField : std::uint8_t {
  kAnchor,
  kKind,
};

using RoomValue = std::variant<PointMm, RoomKind>;

[[nodiscard]] RoomValue get_field(const Room& room, RoomField field);

// The value's alternative must match the field (PointMm for kAnchor, RoomKind for kKind); a
// mismatch is a programming error.
void set_field(Room& room, RoomField field, const RoomValue& value);

template <>
struct ElementTraits<Room> {
  using Field = RoomField;
  using Value = RoomValue;
  static constexpr std::string_view kName = "room";
};

}  // namespace otves
