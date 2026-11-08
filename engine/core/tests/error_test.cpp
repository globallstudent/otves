#include <cstdint>

#include <gtest/gtest.h>

#include <otves/core/error.h>
#include <otves/core/units.h>

namespace otves {
namespace {

Result<Mm> positive_thickness(std::int64_t value) {
  if (value <= 0) {
    return make_error(ErrorCode::kThicknessNotPositive, "thickness must be positive");
  }
  return Mm{value};
}

TEST(Result, CarriesValueOnSuccess) {
  const auto thickness = positive_thickness(6400);
  ASSERT_TRUE(thickness.has_value());
  EXPECT_EQ(thickness->value(), 6400);
}

TEST(Result, CarriesCodeAndMessageOnFailure) {
  const auto thickness = positive_thickness(0);
  ASSERT_FALSE(thickness.has_value());
  EXPECT_EQ(thickness.error().code, ErrorCode::kThicknessNotPositive);
  EXPECT_EQ(thickness.error().message, "thickness must be positive");
}

}  // namespace
}  // namespace otves
