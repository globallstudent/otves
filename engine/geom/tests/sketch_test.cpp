#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/sketch.h>
#include <otves/geom/wall_joins.h>

namespace otves::geom {
namespace {

using namespace otves::literals;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

WallInput wall(std::int64_t n, PointMm start, PointMm end) {
  return {.id = id(n),
          .wall = {.start = start, .end = end, .thickness = 380_mm, .height = 2800_mm}};
}

PointMm at(std::int64_t x, std::int64_t y) { return {.x = Mm{x}, .y = Mm{y}}; }

Leg leg(PointMm start, PointMm end) { return {.start = start, .end = end}; }

// The wall a stroke is read as: 1½ bricks, 2 800 mm high.
Wall like() { return {.start = {}, .end = {}, .thickness = 380_mm, .height = 2800_mm}; }

// How a hand draws: each point up to `shake` millimetres off the line on both axes. The same seed
// gives the same path on every platform.
struct Hand {
  std::int64_t shake = 40;
  std::uint64_t seed = 1;
};

// A finger's path through `corners`: a point every 120 mm or so.
std::vector<PointMm> drawn(const std::vector<PointMm>& corners, Hand hand = {}) {
  std::uint64_t state = hand.seed * 0x9E37'79B9'7F4A'7C15ULL;
  const auto off = [&state, shake = hand.shake] {
    state = (state * 6'364'136'223'846'793'005ULL) + 1'442'695'040'888'963'407ULL;
    const auto span = static_cast<std::uint64_t>((2 * shake) + 1);
    return static_cast<std::int64_t>((state >> 33U) % span) - shake;
  };
  const auto shaken = [&off](std::int64_t x, std::int64_t y) { return at(x + off(), y + off()); };
  std::vector<PointMm> stroke;
  for (std::size_t index = 0; index + 1 < corners.size(); ++index) {
    const std::int64_t run_x = (corners[index + 1].x - corners[index].x).value();
    const std::int64_t run_y = (corners[index + 1].y - corners[index].y).value();
    const std::int64_t steps = std::max<std::int64_t>(
        1, std::max(run_x < 0 ? -run_x : run_x, run_y < 0 ? -run_y : run_y) / 120);
    for (std::int64_t part = 0; part < steps; ++part) {
      stroke.push_back(shaken(corners[index].x.value() + (run_x * part / steps),
                              corners[index].y.value() + (run_y * part / steps)));
    }
  }
  stroke.push_back(shaken(corners.back().x.value(), corners.back().y.value()));
  return stroke;
}

// A stroke read with a 300 mm reach on a 100 mm grid.
Sketch sketch(const std::vector<PointMm>& points, bool square = true) {
  return {.points = points, .reach = 300_mm, .grid = 100_mm, .square = square};
}

// A room 6 000 × 4 000 between centre lines, drawn counter-clockwise from the origin:
// walls 1 bottom, 2 right, 3 top, 4 left.
std::vector<WallInput> room() {
  return {wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(6000, 4000)),
          wall(3, at(6000, 4000), at(0, 4000)), wall(4, at(0, 4000), at(0, 0))};
}

std::vector<Leg> rectangle(std::int64_t x, std::int64_t y, std::int64_t width,
                           std::int64_t height) {
  return {leg(at(x, y), at(x + width, y)), leg(at(x + width, y), at(x + width, y + height)),
          leg(at(x + width, y + height), at(x, y + height)), leg(at(x, y + height), at(x, y))};
}

TEST(Straighten, ARoughLoopBecomesARoom) {
  const std::vector stroke = drawn({at(0, 0), at(6000, 0), at(6000, 4000), at(0, 4000), at(0, 0)});

  const auto legs = straighten({}, {}, sketch(stroke), like());

  ASSERT_TRUE(legs.has_value());
  EXPECT_EQ(*legs, rectangle(0, 0, 6000, 4000));
}

TEST(Straighten, RoundedCornersDoNotMoveTheWalls) {
  // Every corner is cut 200 mm short on both sides, as a finger rounds it.
  const std::vector stroke =
      drawn({at(0, 0), at(5800, 0), at(6000, 200), at(6000, 3800), at(5800, 4000), at(200, 4000),
             at(0, 3800), at(0, 200), at(0, 0)},
            {.shake = 0});

  const auto legs = straighten({}, {}, sketch(stroke), like());

  ASSERT_TRUE(legs.has_value());
  EXPECT_EQ(*legs, rectangle(0, 0, 6000, 4000));
}

TEST(Straighten, ALoopBegunInTheMiddleOfAWallHasNoJointThere) {
  const std::vector stroke =
      drawn({at(3000, 0), at(6000, 0), at(6000, 4000), at(0, 4000), at(0, 0), at(3000, 0)});

  const auto legs = straighten({}, {}, sketch(stroke), like());

  ASSERT_TRUE(legs.has_value());
  EXPECT_EQ(*legs, rectangle(0, 0, 6000, 4000));
}

TEST(Straighten, AStrokeThatRunsPastItsStartStillCloses) {
  const std::vector stroke =
      drawn({at(0, 0), at(6000, 0), at(6000, 4000), at(0, 4000), at(0, 0), at(500, 0)});

  const auto legs = straighten({}, {}, sketch(stroke), like());

  ASSERT_TRUE(legs.has_value());
  EXPECT_EQ(*legs, rectangle(0, 0, 6000, 4000));
}

TEST(Straighten, ABowedLineIsOneWall) {
  // Sags 400 mm in the middle of 6 000: more than the reach, so the stroke turns there.
  std::vector<PointMm> stroke;
  const std::int64_t half = 3000;
  for (std::int64_t x = 0; x <= 2 * half; x += 120) {
    const std::int64_t from_middle = x - half;
    stroke.push_back(at(x, 400 - (400 * from_middle * from_middle / (half * half))));
  }

  const auto legs = straighten({}, {}, sketch(stroke), like());

  ASSERT_TRUE(legs.has_value());
  EXPECT_EQ(*legs, std::vector{leg(at(0, 300), at(6000, 300))});
}

TEST(Straighten, WithoutTheLockALegKeepsItsAngleUnlessItIsNearlyAlongAnAxis) {
  const std::vector slanted = drawn({at(0, 0), at(4000, 3000)}, {.shake = 0});
  const auto free = straighten({}, {}, sketch(slanted, false), like());
  ASSERT_TRUE(free.has_value());
  EXPECT_EQ(*free, std::vector{leg(at(0, 0), at(4000, 3000))});

  // 280 across 4 020 along is under a tenth: about 4°.
  const std::vector nearly = drawn({at(0, 0), at(4020, 280)}, {.shake = 0});
  const auto turned = straighten({}, {}, sketch(nearly, false), like());
  ASSERT_TRUE(turned.has_value());
  EXPECT_EQ(turned->front().start.y, turned->front().end.y);
  EXPECT_EQ(turned->front().end.x, 4000_mm);
}

TEST(Straighten, APartitionMeetsTheWallsItRunsBetween) {
  const std::vector walls = room();
  const std::vector stroke = drawn({at(150, 2050), at(5900, 1950)});

  const auto legs = straighten(walls, {}, sketch(stroke), like());

  ASSERT_TRUE(legs.has_value());
  EXPECT_EQ(*legs, std::vector{leg(at(0, 2000), at(6000, 2000))});
}

TEST(Straighten, ACornerNearAWallEndIsThatEndAndTheLegsIntoItKeepTheirAxes) {
  // Two walls stand apart; the stroke runs from the top of one around to the top of the other
  // and comes down 400 mm to the side of it, further than the reach.
  const std::vector walls{wall(1, at(0, 0), at(0, 3000)), wall(2, at(5000, 3600), at(5000, 0))};
  const std::vector stroke =
      drawn({at(40, 3050), at(-30, 5020), at(5400, 4980), at(5050, 3650)}, {.shake = 20});

  const auto legs = straighten(walls, {}, sketch(stroke), like());

  ASSERT_TRUE(legs.has_value());
  EXPECT_EQ(*legs, (std::vector{leg(at(0, 3000), at(0, 5000)), leg(at(0, 5000), at(5000, 5000)),
                                leg(at(5000, 5000), at(5000, 3600))}));
}

TEST(Straighten, AWallEndOnTheLevelBelowCatchesACorner) {
  const std::vector below{at(0, 0), at(6000, 0)};
  const std::vector stroke = drawn({at(120, -90), at(5870, 110)});

  const auto legs = straighten({}, below, sketch(stroke), like());

  ASSERT_TRUE(legs.has_value());
  EXPECT_EQ(*legs, std::vector{leg(at(0, 0), at(6000, 0))});
}

TEST(Straighten, TheAnswerDoesNotDependOnTheOrderOfTheWalls) {
  std::vector walls = room();
  const std::vector stroke = drawn({at(3050, 3900), at(2950, 2000), at(5900, 2050)});
  const auto legs = straighten(walls, {}, sketch(stroke), like());
  ASSERT_TRUE(legs.has_value());
  EXPECT_EQ(*legs, (std::vector{leg(at(3000, 4000), at(3000, 2000)),
                                leg(at(3000, 2000), at(6000, 2000))}));

  std::ranges::reverse(walls);
  const auto again = straighten(walls, {}, sketch(stroke), like());
  ASSERT_TRUE(again.has_value());
  EXPECT_EQ(*again, *legs);
}

TEST(Straighten, ATapGivesNoWall) {
  const std::vector<PointMm> nothing;
  const std::vector one{at(100, 100)};
  const std::vector tap{at(100, 100), at(180, 60), at(240, 130)};

  for (const std::vector<PointMm>& stroke : {nothing, one, tap}) {
    const auto legs = straighten({}, {}, sketch(stroke), like());
    ASSERT_FALSE(legs.has_value());
    EXPECT_EQ(legs.error().code, ErrorCode::kZeroLength);
  }
}

TEST(Straighten, RefusesAPointOffThePlanAndAWallThatCannotBeBuilt) {
  const std::vector far{at(0, 0), at(100'000'001, 0)};
  const auto off = straighten({}, {}, sketch(far), like());
  ASSERT_FALSE(off.has_value());
  EXPECT_EQ(off.error().code, ErrorCode::kOffPlan);

  Wall thin = like();
  thin.thickness = 0_mm;
  const std::vector stroke = drawn({at(0, 0), at(6000, 0)});
  const auto unbuilt = straighten({}, {}, sketch(stroke), thin);
  ASSERT_FALSE(unbuilt.has_value());
  EXPECT_EQ(unbuilt.error().code, ErrorCode::kThicknessNotPositive);
}

TEST(Straighten, ALongStrokeIsReadThroughFewerPoints) {
  // 60 000 points along 6 000 mm: thinned before the turns are looked for.
  std::vector<PointMm> stroke;
  for (std::int64_t step = 0; step <= 60'000; ++step) {
    stroke.push_back(at(step / 10, step % 7));
  }

  const auto legs = straighten({}, {}, sketch(stroke), like());

  ASSERT_TRUE(legs.has_value());
  EXPECT_EQ(*legs, std::vector{leg(at(0, 0), at(6000, 0))});
}

// Rooms of every size on the grid, drawn with a shaking finger from any corner, come back as
// drawn: four walls on the grid that close.
TEST(Straighten, ShakyRoomsComeBackAsDrawn) {
  std::uint64_t state = 7;
  const auto pick = [&state](std::int64_t low, std::int64_t high) {
    state = (state * 6'364'136'223'846'793'005ULL) + 1'442'695'040'888'963'407ULL;
    const auto span = static_cast<std::uint64_t>(high - low + 1);
    return low + static_cast<std::int64_t>((state >> 33U) % span);
  };
  for (std::uint64_t seed = 1; seed <= 200; ++seed) {
    const std::int64_t x = 100 * pick(-50, 50);
    const std::int64_t y = 100 * pick(-50, 50);
    const std::int64_t width = 100 * pick(15, 90);
    const std::int64_t height = 100 * pick(15, 90);
    const std::vector stroke =
        drawn({at(x, y), at(x + width, y), at(x + width, y + height), at(x, y + height), at(x, y)},
              {.seed = seed});

    const auto legs = straighten({}, {}, sketch(stroke), like());

    ASSERT_TRUE(legs.has_value()) << "seed " << seed;
    EXPECT_EQ(*legs, rectangle(x, y, width, height)) << "seed " << seed;
  }
}

// An L-shaped house: six walls, every corner square.
TEST(Straighten, ShakyLShapesComeBackAsDrawn) {
  std::uint64_t state = 11;
  const auto pick = [&state](std::int64_t low, std::int64_t high) {
    state = (state * 6'364'136'223'846'793'005ULL) + 1'442'695'040'888'963'407ULL;
    const auto span = static_cast<std::uint64_t>(high - low + 1);
    return low + static_cast<std::int64_t>((state >> 33U) % span);
  };
  for (std::uint64_t seed = 1; seed <= 200; ++seed) {
    const std::int64_t width = 100 * pick(40, 120);
    const std::int64_t height = 100 * pick(40, 120);
    const std::int64_t notch_x = 100 * pick(15, (width / 100) - 15);
    const std::int64_t notch_y = 100 * pick(15, (height / 100) - 15);
    const std::vector corners{
        at(0, 0),      at(width, 0), at(width, notch_y), at(notch_x, notch_y), at(notch_x, height),
        at(0, height), at(0, 0)};

    const auto legs = straighten({}, {}, sketch(drawn(corners, {.seed = seed})), like());

    ASSERT_TRUE(legs.has_value()) << "seed " << seed;
    ASSERT_EQ(legs->size(), 6U) << "seed " << seed;
    for (std::size_t index = 0; index < 6; ++index) {
      EXPECT_EQ((*legs)[index], leg(corners[index], corners[index + 1])) << "seed " << seed;
    }
  }
}

}  // namespace
}  // namespace otves::geom
