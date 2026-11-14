#include <expected>

#include <otves/core/assert.h>
#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/room.h>

#include "field.h"

namespace otves {

std::expected<void, Error> validate(const Room& room) {
  if (!on_plan(room.anchor)) {
    return make_error(ErrorCode::kOffPlan, "room anchor lies more than 100 km from the origin");
  }
  return {};
}

RoomValue get_field(const Room& room, RoomField field) {
  switch (field) {
    case RoomField::kAnchor:
      return room.anchor;
    case RoomField::kKind:
      return room.kind;
  }
  OTVES_ASSERT(false);
  return room.anchor;
}

void set_field(Room& room, RoomField field, const RoomValue& value) {
  switch (field) {
    case RoomField::kAnchor:
      assign(room.anchor, value);
      return;
    case RoomField::kKind:
      assign(room.kind, value);
      return;
  }
  OTVES_ASSERT(false);
}

}  // namespace otves
