#include <vector>

#include <gtest/gtest.h>

#include <otves/geom/polygon.h>
#include <otves/geom/vec2.h>

namespace otves::geom {
namespace {

TEST(Polygon, AreaIsPositiveCounterClockwise) {
  const std::vector<Vec2> square{{0, 0}, {2, 0}, {2, 3}, {0, 3}};
  EXPECT_DOUBLE_EQ(signed_area(square), 6.0);
}

TEST(Polygon, AreaIsNegativeClockwise) {
  const std::vector<Vec2> square{{0, 0}, {0, 3}, {2, 3}, {2, 0}};
  EXPECT_DOUBLE_EQ(signed_area(square), -6.0);
}

TEST(Polygon, EmptyOutlineHasNoArea) { EXPECT_DOUBLE_EQ(signed_area({}), 0.0); }

}  // namespace
}  // namespace otves::geom
