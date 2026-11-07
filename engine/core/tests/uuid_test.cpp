#include <regex>

#include <gtest/gtest.h>

#include <otves/core/uuid.h>

namespace otves {
namespace {

TEST(Uuid, V7SetsVersionAndVariant) {
  const Uuid uuid = Uuid::v7(1'762'387'200'000, ~0ULL, ~0ULL);
  EXPECT_EQ(uuid.version(), 7U);
  EXPECT_EQ(uuid.low() >> 62U, 0b10U);
}

TEST(Uuid, V7SortsByTimestamp) {
  const Uuid earlier = Uuid::v7(1'762'387'200'000, ~0ULL, ~0ULL);
  const Uuid later = Uuid::v7(1'762'387'200'001, 0, 0);
  EXPECT_LT(earlier, later);
}

TEST(Uuid, FormatsCanonically) {
  const Uuid uuid{0x0123'4567'89ab'cdefULL, 0xfedc'ba98'7654'3210ULL};
  EXPECT_EQ(uuid.to_string(), "01234567-89ab-cdef-fedc-ba9876543210");
  EXPECT_TRUE(std::regex_match(
      Uuid::v7(1, 2, 3).to_string(),
      std::regex{"[0-9a-f]{8}-[0-9a-f]{4}-7[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}"}));
}

TEST(Uuid, DefaultIsNil) {
  EXPECT_TRUE(Uuid{}.is_nil());
  EXPECT_TRUE(ElementId{}.is_nil());
}

}  // namespace
}  // namespace otves
