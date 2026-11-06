#include <ostream>

#include <gtest/gtest.h>

#include <otves/core/units.h>

namespace otves {

// GoogleTest finds this printer by name through ADL, which an anonymous namespace would hide.
// NOLINTNEXTLINE(readability-identifier-naming,misc-use-anonymous-namespace)
static void PrintTo(Mm length, std::ostream* out) { *out << length.value() << " mm"; }

namespace {

using namespace literals;

TEST(Mm, KeepsMeasuredValuesExact) {
  constexpr Mm wall = 6400_mm;
  static_assert(wall.value() == 6400);
  EXPECT_EQ(wall + 600_mm, 7000_mm);
  EXPECT_EQ(wall - 6400_mm, 0_mm);
}

TEST(Mm, ScalesByWholeFactors) {
  EXPECT_EQ(380_mm * 2, 760_mm);
  EXPECT_EQ(3 * 250_mm, 750_mm);
  EXPECT_EQ(-(120_mm), Mm{-120});
}

TEST(Mm, OrdersLikeIntegers) {
  EXPECT_LT(380_mm, 510_mm);
  EXPECT_GE(510_mm, 510_mm);
}

TEST(Mm, ConvertsToMetres) { EXPECT_DOUBLE_EQ(to_metres(6400_mm), 6.4); }

TEST(Mm, RoundsMetresToNearestMillimetre) {
  EXPECT_EQ(round_to_mm(6.3996), 6400_mm);
  EXPECT_EQ(round_to_mm(6.4004), 6400_mm);
  EXPECT_EQ(round_to_mm(-0.0004), 0_mm);
}

}  // namespace
}  // namespace otves
