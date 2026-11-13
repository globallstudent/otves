#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/editing.h>
#include <otves/geom/wall_joins.h>

namespace otves::geom {
namespace {

using namespace otves::literals;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

WallInput wall(std::int64_t n, PointMm start, PointMm end, Mm thickness = 380_mm) {
  return {.id = id(n),
          .wall = {.start = start, .end = end, .thickness = thickness, .height = 2800_mm}};
}

PointMm at(std::int64_t x, std::int64_t y) { return {.x = Mm{x}, .y = Mm{y}}; }

// A touch with a 300 mm reach on a 100 mm grid.
Aim touch(std::optional<PointMm> from, PointMm point, bool square = true) {
  return {.from = from, .at = point, .reach = 300_mm, .grid = 100_mm, .square = square};
}

// A room 6 000 × 4 000 between centre lines, drawn counter-clockwise from the origin:
// walls 1 bottom, 2 right, 3 top, 4 left.
std::vector<WallInput> room() {
  return {wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(6000, 4000)),
          wall(3, at(6000, 4000), at(0, 4000)), wall(4, at(0, 4000), at(0, 0))};
}

EndMove to(std::int64_t n, WallEnd end, PointMm point) {
  return {.wall = id(n), .end = end, .to = point};
}

TEST(Aim, AFirstPointFallsOnTheGrid) {
  const Aimed aimed = aim({}, {}, touch(std::nullopt, at(30, -40)));

  EXPECT_EQ(aimed.point, at(0, 0));
  EXPECT_EQ(aimed.length, 0_mm);
  EXPECT_FALSE(aimed.meets);
  EXPECT_TRUE(aimed.guides.empty());
}

TEST(Aim, HalfAGridCellGoesUp) {
  EXPECT_EQ(aim({}, {}, touch(std::nullopt, at(50, -50))).point, at(100, 0));
  EXPECT_EQ(aim({}, {}, touch(std::nullopt, at(-150, 149))).point, at(-100, 100));
}

TEST(Aim, TheLockHoldsTheWallToTheNearerAxis) {
  const Aimed along_x = aim({}, {}, touch(at(0, 0), at(6420, 260)));
  EXPECT_EQ(along_x.point, at(6400, 0));
  EXPECT_EQ(along_x.length, 6400_mm);

  EXPECT_EQ(aim({}, {}, touch(at(0, 0), at(900, -3040))).point, at(0, -3000));
}

TEST(Aim, WithoutTheLockAWallGoesAtAnyAngleUnlessItIsNearlyAlongAnAxis) {
  const Aimed free = aim({}, {}, touch(at(0, 0), at(4020, 2980), false));
  EXPECT_EQ(free.point, at(4000, 3000));
  EXPECT_EQ(free.length, 5000_mm);

  // 280 across 4 020 along is under a tenth: about 4°.
  EXPECT_EQ(aim({}, {}, touch(at(0, 0), at(4020, 280), false)).point, at(4000, 0));
}

TEST(Aim, AWallEndWithinReachTakesThePoint) {
  const std::vector walls{wall(1, at(0, 0), at(6400, 0))};

  const Aimed aimed = aim(walls, {}, touch(std::nullopt, at(6250, 180)));

  EXPECT_EQ(aimed.point, at(6400, 0));
  EXPECT_TRUE(aimed.meets);
}

TEST(Aim, TheWallsOwnStartDoesNotCatchItsEnd) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};

  // Drawing on from the end of wall 1: 200 mm up is a short wall, not its start again.
  EXPECT_EQ(aim(walls, {}, touch(at(6000, 0), at(6040, 200))).point, at(6000, 200));
}

TEST(Aim, AnEndOnTheLevelBelowCatchesThePointButIsNotMet) {
  const std::vector below{at(6400, 0)};

  const Aimed aimed = aim({}, below, touch(std::nullopt, at(6250, 180)));

  EXPECT_EQ(aimed.point, at(6400, 0));
  EXPECT_FALSE(aimed.meets);
}

TEST(Aim, AFreeCoordinateLinesUpWithAWallEnd) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(6000, 4000))};

  // Back along the top from (6 000, 4 000): 130 mm from the line through the first corner.
  const Aimed aimed = aim(walls, {}, touch(at(6000, 4000), at(130, 4150)));

  EXPECT_EQ(aimed.point, at(0, 4000));
  const std::vector expected{Guide{.from = at(0, 0), .to = at(0, 4000)}};
  EXPECT_EQ(aimed.guides, expected);
  EXPECT_FALSE(aimed.meets);
}

TEST(Aim, AWallMeetsAnotherAlongItsLength) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};

  // A first point 180 mm off the wall's line starts on it, where the grid puts it along.
  const Aimed start = aim(walls, {}, touch(std::nullopt, at(3020, 180)));
  EXPECT_EQ(start.point, at(3000, 0));
  EXPECT_TRUE(start.meets);

  // Coming down onto it from above: the lock keeps x, the wall gives y.
  const Aimed end = aim(walls, {}, touch(at(5000, 2000), at(5060, 150)));
  EXPECT_EQ(end.point, at(5000, 0));
  EXPECT_EQ(end.length, 2000_mm);
  EXPECT_TRUE(end.meets);
}

TEST(Aim, TheReachOfAWallCountsFromItsFaces) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};

  // 450 mm from the line is 260 mm from the face of a 380 mm wall.
  EXPECT_EQ(aim(walls, {}, touch(at(5000, 2000), at(5060, 450))).point, at(5000, 0));
  // 500 mm from the line is past the reach: the wall ends on the grid, short of it.
  const Aimed short_of = aim(walls, {}, touch(at(5000, 2000), at(5060, 500)));
  EXPECT_EQ(short_of.point, at(5000, 500));
  EXPECT_FALSE(short_of.meets);
}

TEST(Aim, AFreeWallMeetsASlantedWallAtTheNearestPointOfItsLine) {
  const std::vector walls{wall(1, at(0, 0), at(4000, 4000))};

  const Aimed aimed = aim(walls, {}, touch(at(4000, 0), at(2100, 1900), false));

  EXPECT_EQ(aimed.point, at(2000, 2000));
  EXPECT_TRUE(aimed.meets);
}

TEST(Aim, TheAnswerDoesNotDependOnTheOrderOfTheWalls) {
  std::vector<WallInput> walls = room();
  const Aimed forward = aim(walls, {}, touch(at(3000, 0), at(3100, 3900)));
  std::ranges::reverse(walls);

  EXPECT_EQ(aim(walls, {}, touch(at(3000, 0), at(3100, 3900))), forward);
  EXPECT_EQ(forward.point, at(3000, 4000));
}

TEST(AimMeasured, ARunStartsToTheRightAndTurnsFromTheWallBefore) {
  const Aimed first = aim({}, Measured{.from = at(0, 0), .before = {}, .length = 6400_mm});
  EXPECT_EQ(first.point, at(6400, 0));
  EXPECT_EQ(first.length, 6400_mm);

  // The plan's y runs up: left of a wall running right is up.
  const Measured left{
      .from = at(6400, 0), .before = at(0, 0), .turn = Turn::kLeft, .length = 4000_mm};
  EXPECT_EQ(aim({}, left).point, at(6400, 4000));
  const Measured right{
      .from = at(6400, 0), .before = at(0, 0), .turn = Turn::kRight, .length = 4000_mm};
  EXPECT_EQ(aim({}, right).point, at(6400, -4000));
  const Measured straight{.from = at(6400, 0), .before = at(0, 0), .length = 1000_mm};
  EXPECT_EQ(aim({}, straight).point, at(7400, 0));
}

TEST(AimMeasured, ASlantedWallBeforeGivesASlantedHeading) {
  // A 3-4-5 wall: left of it is (-4, 3) / 5.
  const Measured left{
      .from = at(3000, 4000), .before = at(0, 0), .turn = Turn::kLeft, .length = 5000_mm};

  EXPECT_EQ(aim({}, left).point, at(-1000, 7000));
}

TEST(AimMeasured, ClosingOnTheFirstPointMeetsThePlan) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(6000, 4000)),
                          wall(3, at(6000, 4000), at(0, 4000))};
  const Measured last{
      .from = at(0, 4000), .before = at(6000, 4000), .turn = Turn::kLeft, .length = 4000_mm};

  const Aimed aimed = aim(walls, last);

  EXPECT_EQ(aimed.point, at(0, 0));
  EXPECT_TRUE(aimed.meets);
}

TEST(AimMeasured, NoLengthYetStaysAtTheStart) {
  const Aimed aimed = aim(room(), Measured{.from = at(0, 0), .before = {}, .length = 0_mm});

  EXPECT_EQ(aimed.point, at(0, 0));
  EXPECT_EQ(aimed.length, 0_mm);
  EXPECT_FALSE(aimed.meets);
}

TEST(ResizeWall, AFreeWallMovesItsFarEnd) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};

  const auto moves = resize_wall(walls, id(1), 6437_mm, WallEnd::kStart);

  ASSERT_TRUE(moves.has_value()) << moves.error().message;
  EXPECT_EQ(*moves, std::vector{to(1, WallEnd::kEnd, at(6437, 0))});
}

TEST(ResizeWall, TheFixedEndMayBeEither) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};

  const auto moves = resize_wall(walls, id(1), 6400_mm, WallEnd::kEnd);

  ASSERT_TRUE(moves.has_value()) << moves.error().message;
  EXPECT_EQ(*moves, std::vector{to(1, WallEnd::kStart, at(-400, 0))});
}

TEST(ResizeWall, TheSameLengthMovesNothing) {
  const auto moves = resize_wall(room(), id(1), 6000_mm, WallEnd::kStart);

  ASSERT_TRUE(moves.has_value()) << moves.error().message;
  EXPECT_TRUE(moves->empty());
}

TEST(ResizeWall, ARoomStaysSquare) {
  const auto moves = resize_wall(room(), id(1), 6400_mm, WallEnd::kStart);

  ASSERT_TRUE(moves.has_value()) << moves.error().message;
  // The right wall goes with the corner whole; the top wall stretches to it; the left stays.
  const std::vector expected{
      to(1, WallEnd::kEnd, at(6400, 0)),
      to(2, WallEnd::kStart, at(6400, 0)),
      to(2, WallEnd::kEnd, at(6400, 4000)),
      to(3, WallEnd::kStart, at(6400, 4000)),
  };
  EXPECT_EQ(*moves, expected);
}

TEST(ResizeWall, APartitionAgainstAStretchedWallStaysWhereItIs) {
  std::vector<WallInput> walls = room();
  walls.push_back(wall(5, at(3000, 0), at(3000, 4000), 120_mm));

  const auto moves = resize_wall(walls, id(1), 6400_mm, WallEnd::kStart);

  ASSERT_TRUE(moves.has_value()) << moves.error().message;
  EXPECT_TRUE(std::ranges::none_of(*moves, [](const EndMove& move) { return move.wall == id(5); }));
}

TEST(ResizeWall, ALongerPartitionPushesTheWallItStandsAgainst) {
  std::vector<WallInput> walls = room();
  walls.push_back(wall(5, at(3000, 0), at(3000, 4000), 120_mm));

  const auto moves = resize_wall(walls, id(5), 4500_mm, WallEnd::kStart);

  ASSERT_TRUE(moves.has_value()) << moves.error().message;
  // The top wall rises with the partition's end; the side walls stretch to it.
  const std::vector expected{
      to(2, WallEnd::kEnd, at(6000, 4500)), to(3, WallEnd::kStart, at(6000, 4500)),
      to(3, WallEnd::kEnd, at(0, 4500)),    to(4, WallEnd::kStart, at(0, 4500)),
      to(5, WallEnd::kEnd, at(3000, 4500)),
  };
  EXPECT_EQ(*moves, expected);
}

TEST(ResizeWall, AWallThatMovesWholeTakesThePartitionAgainstIt) {
  std::vector<WallInput> walls = room();
  walls.push_back(wall(5, at(3000, 0), at(3000, 4000), 120_mm));

  // The left wall, from (0, 4 000) down to the origin, grows downwards: the bottom wall drops.
  const auto moves = resize_wall(walls, id(4), 4500_mm, WallEnd::kStart);

  ASSERT_TRUE(moves.has_value()) << moves.error().message;
  const std::vector expected{
      to(1, WallEnd::kStart, at(0, -500)),    to(1, WallEnd::kEnd, at(6000, -500)),
      to(2, WallEnd::kStart, at(6000, -500)), to(4, WallEnd::kEnd, at(0, -500)),
      to(5, WallEnd::kStart, at(3000, -500)),
  };
  EXPECT_EQ(*moves, expected);
}

TEST(ResizeWall, AWallThatReachesTheFixedEndTurnsAboutIt) {
  // A triangle cannot keep every direction: the side back to the fixed corner turns.
  const std::vector walls{wall(1, at(0, 0), at(4000, 0)), wall(2, at(4000, 0), at(4000, 3000)),
                          wall(3, at(4000, 3000), at(0, 0))};

  const auto moves = resize_wall(walls, id(1), 5000_mm, WallEnd::kStart);

  ASSERT_TRUE(moves.has_value()) << moves.error().message;
  const std::vector expected{
      to(1, WallEnd::kEnd, at(5000, 0)),
      to(2, WallEnd::kStart, at(5000, 0)),
      to(2, WallEnd::kEnd, at(5000, 3000)),
      to(3, WallEnd::kStart, at(5000, 3000)),
  };
  EXPECT_EQ(*moves, expected);
}

TEST(ResizeWall, ASlantedWallKeepsItsDirectionToTheMillimetre) {
  const std::vector walls{wall(1, at(0, 0), at(3000, 4000))};

  const auto moves = resize_wall(walls, id(1), 10000_mm, WallEnd::kStart);

  ASSERT_TRUE(moves.has_value()) << moves.error().message;
  EXPECT_EQ(*moves, std::vector{to(1, WallEnd::kEnd, at(6000, 8000))});
}

TEST(ResizeWall, WithNoEndNamedTheJoinedEndStays) {
  // A partition drawn from inside the room down onto its bottom wall.
  std::vector walls = room();
  walls.push_back(wall(5, at(3000, 3000), at(3000, 0)));

  // It grows from the wall it stands against, which stays where it is.
  const auto partition = resize_wall(walls, id(5), 3500_mm);
  ASSERT_TRUE(partition.has_value()) << partition.error().message;
  EXPECT_EQ(*partition, std::vector{to(5, WallEnd::kStart, at(3000, 3500))});

  // A wall joined at both ends, or at neither, keeps its start.
  const auto side = resize_wall(walls, id(1), 6400_mm);
  ASSERT_TRUE(side.has_value()) << side.error().message;
  EXPECT_EQ(side->front(), to(1, WallEnd::kEnd, at(6400, 0)));
  const std::vector alone{wall(1, at(0, 0), at(6000, 0))};
  const auto free = resize_wall(alone, id(1), 6400_mm);
  ASSERT_TRUE(free.has_value()) << free.error().message;
  EXPECT_EQ(*free, std::vector{to(1, WallEnd::kEnd, at(6400, 0))});
}

TEST(ResizeWall, RefusesWhatWouldBreakAWall) {
  const auto unknown = resize_wall(room(), id(9), 6000_mm, WallEnd::kStart);
  ASSERT_FALSE(unknown.has_value());
  EXPECT_EQ(unknown.error().code, ErrorCode::kNotFound);

  const auto none = resize_wall(room(), id(1), 0_mm, WallEnd::kStart);
  ASSERT_FALSE(none.has_value());
  EXPECT_EQ(none.error().code, ErrorCode::kZeroLength);

  // An L: shortening the first leg by its neighbour's whole run would fold that neighbour away.
  const std::vector walls{wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(8000, 0)),
                          wall(3, at(8000, 0), at(8000, 3000))};
  const auto vanished = resize_wall(walls, id(1), 8000_mm, WallEnd::kStart);
  ASSERT_FALSE(vanished.has_value());
  EXPECT_EQ(vanished.error().code, ErrorCode::kBreaksJoin);
  const auto folded = resize_wall(walls, id(1), 9000_mm, WallEnd::kStart);
  ASSERT_FALSE(folded.has_value());
  EXPECT_EQ(folded.error().code, ErrorCode::kBreaksJoin);
}

TEST(ResizeWall, RefusesToLeaveAPartitionOffItsWall) {
  // A partition stands 5 m along a 6 m wall.
  const std::vector walls{wall(1, at(0, 0), at(6000, 0)), wall(2, at(5000, 0), at(5000, 3000))};

  // The wall may end where the partition stands: they make a corner.
  const auto corner = resize_wall(walls, id(1), 5000_mm, WallEnd::kStart);
  ASSERT_TRUE(corner.has_value()) << corner.error().message;
  EXPECT_EQ(*corner, std::vector{to(1, WallEnd::kEnd, at(5000, 0))});

  const auto shorter = resize_wall(walls, id(1), 4000_mm, WallEnd::kStart);
  ASSERT_FALSE(shorter.has_value());
  EXPECT_EQ(shorter.error().code, ErrorCode::kBreaksJoin);
}

}  // namespace
}  // namespace otves::geom
