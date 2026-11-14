#include <expected>

#include <otves/core/assert.h>
#include <otves/core/error.h>
#include <otves/core/opening.h>
#include <otves/core/units.h>

#include "field.h"

namespace otves {

std::expected<void, Error> validate(const Opening& opening) {
  if (opening.offset < Mm{0}) {
    return make_error(ErrorCode::kOffsetNegative, "opening offset must not be negative");
  }
  if (opening.width <= Mm{0}) {
    return make_error(ErrorCode::kWidthNotPositive, "opening width must be positive");
  }
  if (opening.height <= Mm{0}) {
    return make_error(ErrorCode::kHeightNotPositive, "opening height must be positive");
  }
  if (opening.sill < Mm{0}) {
    return make_error(ErrorCode::kSillNegative, "opening sill must not be negative");
  }
  return {};
}

OpeningValue get_field(const Opening& opening, OpeningField field) {
  switch (field) {
    case OpeningField::kOffset:
      return opening.offset;
    case OpeningField::kWidth:
      return opening.width;
    case OpeningField::kHeight:
      return opening.height;
    case OpeningField::kSill:
      return opening.sill;
    case OpeningField::kKind:
      return opening.kind;
    case OpeningField::kHinge:
      return opening.hinge;
    case OpeningField::kSwing:
      return opening.swing;
  }
  OTVES_ASSERT(false);
  return opening.offset;
}

void set_field(Opening& opening, OpeningField field, const OpeningValue& value) {
  switch (field) {
    case OpeningField::kOffset:
      assign(opening.offset, value);
      return;
    case OpeningField::kWidth:
      assign(opening.width, value);
      return;
    case OpeningField::kHeight:
      assign(opening.height, value);
      return;
    case OpeningField::kSill:
      assign(opening.sill, value);
      return;
    case OpeningField::kKind:
      assign(opening.kind, value);
      return;
    case OpeningField::kHinge:
      assign(opening.hinge, value);
      return;
    case OpeningField::kSwing:
      assign(opening.swing, value);
      return;
  }
  OTVES_ASSERT(false);
}

}  // namespace otves
