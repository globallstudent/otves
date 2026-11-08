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

// Plan coordinates stay within ±100 km, so geometry can multiply two coordinate differences in
// int64 without overflow.
inline constexpr Mm kMaxCoordinate{100'000'000};

}  // namespace otves
