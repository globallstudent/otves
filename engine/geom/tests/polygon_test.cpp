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

TEST(Polygon, ContainsWhatLiesInsideAnLShape) {
  const std::vector<Vec2> shape{{0, 0}, {4, 0}, {4, 2}, {2, 2}, {2, 4}, {0, 4}};
  EXPECT_TRUE(contains(shape, {.x = 1, .y = 1}));
  EXPECT_TRUE(contains(shape, {.x = 1, .y = 3}));
  EXPECT_TRUE(contains(shape, {.x = 3, .y = 1}));
  // The notch, and beside the shape.
  EXPECT_FALSE(contains(shape, {.x = 3, .y = 3}));
  EXPECT_FALSE(contains(shape, {.x = -1, .y = 1}));
  EXPECT_FALSE(contains(shape, {.x = 5, .y = 1}));
  EXPECT_FALSE(contains({}, {.x = 0, .y = 0}));
}

}  // namespace
}  // namespace otves::geom
