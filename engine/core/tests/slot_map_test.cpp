#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/slot_map.h>

namespace otves {
namespace {

TEST(SlotMap, StoresAndFindsValues) {
  SlotMap<std::string> map;
  const Handle wall = map.insert("Ст-4");
  ASSERT_NE(map.get(wall), nullptr);
  EXPECT_EQ(*map.get(wall), "Ст-4");
  EXPECT_EQ(map.size(), 1U);
}

TEST(SlotMap, StaleHandleNeverResolvesAfterReuse) {
  SlotMap<int> map;
  const Handle first = map.insert(1);
  EXPECT_TRUE(map.erase(first));
  const Handle second = map.insert(2);
  EXPECT_EQ(second.index, first.index);
  EXPECT_EQ(map.get(first), nullptr);
  EXPECT_FALSE(map.erase(first));
  EXPECT_EQ(*map.get(second), 2);
}

TEST(SlotMap, IteratesInSlotOrder) {
  SlotMap<int> map;
  static_cast<void>(map.insert(10));
  const Handle middle = map.insert(20);
  static_cast<void>(map.insert(30));
  map.erase(middle);
  std::vector<int> seen;
  map.for_each([&seen](int value) { seen.push_back(value); });
  EXPECT_EQ(seen, (std::vector<int>{10, 30}));
}

}  // namespace
}  // namespace otves
