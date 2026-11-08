#include <gtest/gtest.h>

#include <otves/core/hlc.h>

namespace otves {
namespace {

TEST(HybridClock, GrowsWithinOneMillisecond) {
  HybridClock clock;
  const Hlc first = clock.tick(1'000);
  const Hlc second = clock.tick(1'000);
  EXPECT_LT(first, second);
}

TEST(HybridClock, KeepsGrowingWhenWallClockStepsBack) {
  HybridClock clock;
  const Hlc before = clock.tick(5'000);
  const Hlc after = clock.tick(4'000);
  EXPECT_LT(before, after);
}

TEST(HybridClock, FollowsWallClockForward) {
  HybridClock clock;
  static_cast<void>(clock.tick(1'000));
  EXPECT_EQ(clock.tick(2'000).value >> 16U, 2'000U);
}

TEST(HybridClock, TicksAfterAnObservedStamp) {
  HybridClock clock;
  const Hlc saved = clock.tick(9'000);
  HybridClock reopened;
  reopened.observe(saved);

  EXPECT_GT(reopened.tick(1'000), saved);
}

}  // namespace
}  // namespace otves
