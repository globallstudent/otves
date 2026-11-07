#pragma once

#include <compare>

#include <otves/core/units.h>

namespace otves {

// A point on a part's plan, in millimetres: x to the right, y up, from the part's origin.
struct PointMm {
  Mm x;
  Mm y;

  constexpr auto operator<=>(const PointMm&) const = default;
};

}  // namespace otves
