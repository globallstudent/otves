#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/openings.h>
#include <otves/geom/polygon.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

namespace otves::geom {
namespace {

using namespace otves::literals;

constexpr double kTolerance = 1e-9;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

WallInput wall(std::int64_t n, PointMm start, PointMm end,
               LocationLine location_line = LocationLine::kCenter) {
  return {.id = id(n),
          .wall = {.start = start,
                   .end = end,
                   .thickness = 380_mm,
                   .height = 2800_mm,
                   .location_line = location_line}};
}

// The wall and the distance along it, or a nil id for a miss.
struct Where {
  ElementId wall;
  double along = -1.0;
};

Where where(const std::optional<WallHit>& hit) {
  return hit ? Where{.wall = hit->wall, .along = hit->along} : Where{};
}

TEST(HitWall, FindsTheWallUnderThePointAndHowFarAlongItLies) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {6000_mm, 0_mm})};

  const Where hit = where(hit_wall(walls, {.x = 2.5, .y = 0.1}, 0.0));

  EXPECT_EQ(hit.wall, id(1));
  EXPECT_NEAR(hit.along, 2.5, kTolerance);
}

TEST(HitWall, ReachesPastTheFaceButNoFurther) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {6000_mm, 0_mm})};

  EXPECT_TRUE(hit_wall(walls, {.x = 1.0, .y = 0.49}, 0.3).has_value());
  EXPECT_FALSE(hit_wall(walls, {.x = 1.0, .y = 0.51}, 0.3).has_value());
}

TEST(HitWall, PastAnEndLandsOnThatEnd) {
  const std::vector walls{wall(1, {0_mm, 0_mm}, {6000_mm, 0_mm})};

  EXPECT_NEAR(where(hit_wall(walls, {.x = 6.2, .y = 0.0}, 0.3)).along, 6.0, kTolerance);
}

TEST(HitWall, TakesTheNearestWallAndBreaksTiesById) {
  const std::vector walls{wall(2, {0_mm, 1000_mm}, {6000_mm, 1000_mm}),
                          wall(1, {0_mm, 0_mm}, {6000_mm, 0_mm})};

  EXPECT_EQ(where(hit_wall(walls, {.x = 3.0, .y = 0.7}, 1.0)).wall, id(2));
  EXPECT_EQ(where(hit_wall(walls, {.x = 3.0, .y = 0.5}, 1.0)).wall, id(1));
}

TEST(HitWall, FollowsTheLocationLineToTheFaces) {
  // Drawn along its left face, the wall lies to the right of the line: below it here.
  const std::vector walls{wall(1, {0_mm, 0_mm}, {6000_mm, 0_mm}, LocationLine::kLeftFace)};

  EXPECT_TRUE(hit_wall(walls, {.x = 1.0, .y = -0.3}, 0.0).has_value());
  EXPECT_FALSE(hit_wall(walls, {.x = 1.0, .y = 0.1}, 0.0).has_value());
}

TEST(HitOpening, FindsTheOpeningUnderThePointWithinReach) {
  const Wall host = wall(1, {0_mm, 0_mm}, {6000_mm, 0_mm}).wall;
  const auto opening = [&host](std::int64_t n, Mm offset) {
    return PlacedOpening{
        .id = id(n),
        .wall = host,
        .opening = {
            .wall = id(1), .offset = offset, .width = 1500_mm, .height = 1500_mm, .sill = 900_mm}};
  };
  const std::vector openings{opening(3, 3000_mm), opening(2, 500_mm)};

  EXPECT_EQ(hit_opening(openings, {.x = 1.0, .y = 0.1}, 0.0), id(2));
  EXPECT_EQ(hit_opening(openings, {.x = 4.6, .y = -0.4}, 0.3), id(3));
  EXPECT_EQ(hit_opening(openings, {.x = 2.5, .y = 0.0}, 0.3), std::nullopt);
}

TEST(OpeningOutline, CutsFaceToFaceAlongTheLocationLine) {
  const Wall host = wall(1, {0_mm, 0_mm}, {0_mm, 4000_mm}).wall;
  const Opening window{
      .wall = id(1), .offset = 1000_mm, .width = 1500_mm, .height = 1500_mm, .sill = 800_mm};

  const std::array<Vec2, 4> outline = opening_outline(host, window);

  // The wall runs up the y axis, so its right face is at x = +0.19.
  const std::array<Vec2, 4> expected{Vec2{.x = 0.19, .y = 1.0},
                                     {.x = 0.19, .y = 2.5},
                                     {.x = -0.19, .y = 2.5},
                                     {.x = -0.19, .y = 1.0}};
  for (std::size_t i = 0; i < outline.size(); ++i) {
    EXPECT_NEAR(outline.at(i).x, expected.at(i).x, kTolerance) << "point " << i;
    EXPECT_NEAR(outline.at(i).y, expected.at(i).y, kTolerance) << "point " << i;
  }
  EXPECT_NEAR(signed_area(outline), 1.5 * 0.38, kTolerance);
}

}  // namespace
}  // namespace otves::geom
