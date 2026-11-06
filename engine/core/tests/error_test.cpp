#include <cstdint>

#include <gtest/gtest.h>

#include <otves/core/error.h>
#include <otves/core/units.h>

namespace otves {
namespace {

Result<Mm> positive_length(std::int64_t value) {
  if (value <= 0) {
    return make_error(ErrorCode::kInvalidArgument, "length must be positive");
  }
  return Mm{value};
}

TEST(Result, CarriesValueOnSuccess) {
  const auto length = positive_length(6400);
  ASSERT_TRUE(length.has_value());
  EXPECT_EQ(length->value(), 6400);
}

TEST(Result, CarriesCodeAndMessageOnFailure) {
  const auto length = positive_length(0);
  ASSERT_FALSE(length.has_value());
  EXPECT_EQ(length.error().code, ErrorCode::kInvalidArgument);
  EXPECT_EQ(length.error().message, "length must be positive");
}

}  // namespace
}  // namespace otves
