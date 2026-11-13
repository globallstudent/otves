#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/drafting/plan.h>
#include <otves/geom/openings.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

namespace otves::drafting {
namespace {

using namespace otves::literals;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

PointMm at(std::int64_t x, std::int64_t y) { return {.x = Mm{x}, .y = Mm{y}}; }

// 1½ bricks of 1НФ on its centre line.
geom::WallInput wall(std::int64_t n, PointMm start, PointMm end) {
  return {.id = id(n),
          .wall = {.start = start,
                   .end = end,
                   .thickness = 380_mm,
                   .height = 2800_mm,
                   .unit_length = 250_mm,
                   .unit_width = 120_mm,
                   .unit_height = 65_mm}};
}

// Label text 300 mm high on the plan, its digits 150 mm wide.
constexpr TextSize kText{.height = 0.3, .digit = 0.15};

// From a wall's line to the middle of its length: half the wall, a quarter of a text height
// clear, half a text height.
constexpr double kBeside = 0.19 + 0.075 + 0.15;

Drawing draft_walls(const std::vector<geom::WallInput>& walls) {
  return draft({.walls = walls, .openings = {}, .below = {}}, kText);
}

void expect_near(geom::Vec2 actual, geom::Vec2 expected) {
  EXPECT_NEAR(actual.x, expected.x, 1e-9);
  EXPECT_NEAR(actual.y, expected.y, 1e-9);
}

TEST(Draft, AWallIsItsOutlineFilledByWhetherItHasAMaterial) {
  std::vector walls{wall(1, at(0, 0), at(6000, 0))};
  const Drawing built = draft_walls(walls);
  ASSERT_EQ(built.polygons.size(), 1U);
  EXPECT_EQ(built.polygons[0].element, id(1));
  EXPECT_EQ(built.polygons[0].fill, Fill::kWall);
  EXPECT_EQ(built.polygons[0].stroke, Stroke::kWallEdge);
  EXPECT_EQ(built.polygons[0].points.size(), 4U);

  walls[0].wall.unit_length = 0_mm;
  walls[0].wall.unit_width = 0_mm;
  walls[0].wall.unit_height = 0_mm;
  EXPECT_EQ(draft_walls(walls).polygons[0].fill, Fill::kUnchosen);
}

TEST(Draft, ALengthIsWrittenByTheLeftFaceAtTheMiddle) {
  const Drawing drawing = draft_walls({wall(1, at(0, 0), at(6000, 0))});

  ASSERT_EQ(drawing.texts.size(), 1U);
  const Text& length = drawing.texts[0];
  EXPECT_EQ(length.element, id(1));
  EXPECT_EQ(length.kind, TextKind::kLength);
  EXPECT_EQ(length.value, 6000_mm);
  expect_near(length.at, {.x = 3.0, .y = kBeside});
  expect_near(length.along, {.x = 1.0, .y = 0.0});
}

TEST(Draft, ALengthReadsFromTheLeftAndOnAnUprightWallFromTheBottom) {
  // Drawn to the left, the wall's left face is below it.
  const Drawing leftward = draft_walls({wall(1, at(6000, 0), at(0, 0))});
  expect_near(leftward.texts[0].at, {.x = 3.0, .y = -kBeside});
  expect_near(leftward.texts[0].along, {.x = 1.0, .y = 0.0});

  const Drawing downward = draft_walls({wall(1, at(0, 4000), at(0, 0))});
  expect_near(downward.texts[0].at, {.x = kBeside, .y = 2.0});
  expect_near(downward.texts[0].along, {.x = 0.0, .y = 1.0});

  const Drawing upward = draft_walls({wall(1, at(0, 0), at(0, 4000))});
  expect_near(upward.texts[0].at, {.x = -kBeside, .y = 2.0});
  expect_near(upward.texts[0].along, {.x = 0.0, .y = 1.0});
}

TEST(Draft, ALengthLongerThanFourFifthsOfItsWallIsLeftOut) {
  // «500» is 450 mm wide; four fifths of the wall are 400.
  EXPECT_TRUE(draft_walls({wall(1, at(0, 0), at(500, 0))}).texts.empty());
  // «600» fits in 480.
  EXPECT_EQ(draft_walls({wall(1, at(0, 0), at(600, 0))}).texts.size(), 1U);
}

TEST(Draft, ASketchedLengthIsMarkedAndTakesTheRoomOfTwoMoreDigits) {
  std::vector walls{wall(1, at(0, 0), at(6000, 0)), wall(2, at(0, 3000), at(1000, 3000))};
  walls[0].wall.exact = false;
  EXPECT_EQ(draft_walls(walls).texts[0].kind, TextKind::kAboutLength);
  // «1000» is 600 mm wide and fits in 800; «≈ 1000» is 900 and does not.
  ASSERT_EQ(draft_walls(walls).texts.size(), 2U);
  walls[1].wall.exact = false;
  EXPECT_EQ(draft_walls(walls).texts.size(), 1U);
}

TEST(Draft, ALengthKeepsOffOtherWallsAndOtherLengths) {
  // A second wall runs 400 mm above the first, where the first one's length would stand.
  const Drawing beside =
      draft_walls({wall(1, at(0, 0), at(6000, 0)), wall(2, at(0, 400), at(6000, 400))});
  ASSERT_EQ(beside.texts.size(), 2U);
  expect_near(beside.texts[0].at, {.x = 3.0, .y = -kBeside});
  expect_near(beside.texts[1].at, {.x = 3.0, .y = 0.4 + kBeside});

  // Two walls facing each other 900 mm apart would write their lengths on one spot between
  // them: the second one goes by its other face.
  const Drawing between =
      draft_walls({wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 900), at(0, 900))});
  ASSERT_EQ(between.texts.size(), 2U);
  expect_near(between.texts[0].at, {.x = 3.0, .y = kBeside});
  expect_near(between.texts[1].at, {.x = 3.0, .y = 0.9 + kBeside});
}

TEST(Draft, AWindowIsAGapWithJambsFacesAndGlazing) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};
  const std::vector<geom::PlacedOpening> openings{{.id = id(2),
                                                   .wall = walls[0].wall,
                                                   .opening = {.wall = id(1),
                                                               .offset = 2000_mm,
                                                               .width = 1500_mm,
                                                               .height = 1500_mm,
                                                               .sill = 900_mm,
                                                               .kind = OpeningKind::kWindow}}};

  const Drawing drawing = draft({.walls = walls, .openings = openings, .below = {}}, kText);

  ASSERT_EQ(drawing.polygons.size(), 2U);
  const Polygon& gap = drawing.polygons[1];
  EXPECT_EQ(gap.element, id(2));
  EXPECT_EQ(gap.fill, Fill::kGap);
  EXPECT_EQ(gap.stroke, Stroke::kNone);
  ASSERT_EQ(gap.points.size(), 4U);
  expect_near(gap.points[0], {.x = 2.0, .y = -0.19});
  expect_near(gap.points[2], {.x = 3.5, .y = 0.19});
  // Two jambs, two faces and the glazing line down the middle of the wall.
  ASSERT_EQ(drawing.lines.size(), 5U);
  EXPECT_EQ(std::ranges::count(drawing.lines, Stroke::kWallEdge, &Line::stroke), 2);
  EXPECT_EQ(std::ranges::count(drawing.lines, Stroke::kFrame, &Line::stroke), 3);
  expect_near(drawing.lines[4].from, {.x = 2.0, .y = 0.0});
  expect_near(drawing.lines[4].to, {.x = 3.5, .y = 0.0});
  EXPECT_TRUE(drawing.arcs.empty());
}

TEST(Draft, ADoorStandsOpenOnTheLeftFaceWithItsSwing) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};
  const std::vector<geom::PlacedOpening> openings{{.id = id(2),
                                                   .wall = walls[0].wall,
                                                   .opening = {.wall = id(1),
                                                               .offset = 1000_mm,
                                                               .width = 900_mm,
                                                               .height = 2100_mm,
                                                               .sill = 0_mm,
                                                               .kind = OpeningKind::kDoor}},
                                                  {.id = id(3),
                                                   .wall = walls[0].wall,
                                                   .opening = {.wall = id(1),
                                                               .offset = 4000_mm,
                                                               .width = 900_mm,
                                                               .height = 2100_mm,
                                                               .sill = 0_mm,
                                                               .kind = OpeningKind::kEmpty}}};

  const Drawing drawing = draft({.walls = walls, .openings = openings, .below = {}}, kText);

  // The door's jambs and leaf, then the empty opening's jambs alone.
  ASSERT_EQ(drawing.lines.size(), 5U);
  const Line& leaf = drawing.lines[2];
  EXPECT_EQ(leaf.stroke, Stroke::kLeaf);
  expect_near(leaf.from, {.x = 1.0, .y = 0.19});
  expect_near(leaf.to, {.x = 1.0, .y = 0.19 + 0.9});
  ASSERT_EQ(drawing.arcs.size(), 1U);
  const Arc& swing = drawing.arcs[0];
  EXPECT_EQ(swing.element, id(2));
  expect_near(swing.centre, {.x = 1.0, .y = 0.19});
  expect_near(swing.from, {.x = 1.9, .y = 0.19});
  expect_near(swing.to, {.x = 1.0, .y = 1.09});
}

TEST(Draft, TheLevelBelowIsOutlinedUnderTheWalls) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};
  const std::vector below{wall(7, at(0, 0), at(6000, 0)), wall(8, at(6000, 0), at(6000, 4000))};

  const Drawing drawing = draft({.walls = walls, .openings = {}, .below = below}, kText);

  ASSERT_EQ(drawing.polygons.size(), 3U);
  EXPECT_EQ(drawing.polygons[0].stroke, Stroke::kBelow);
  EXPECT_EQ(drawing.polygons[0].fill, Fill::kNone);
  EXPECT_TRUE(drawing.polygons[0].element.is_nil());
  EXPECT_EQ(drawing.polygons[1].stroke, Stroke::kBelow);
  EXPECT_EQ(drawing.polygons[2].element, id(1));
  // Lengths are written for this level only.
  EXPECT_EQ(drawing.texts.size(), 1U);
}

TEST(Draft, TheDrawingDoesNotDependOnTheOrderOfTheWalls) {
  std::vector walls{wall(1, at(0, 0), at(6000, 0)), wall(2, at(6000, 0), at(6000, 4000)),
                    wall(3, at(6000, 4000), at(0, 4000)), wall(4, at(0, 4000), at(0, 0)),
                    wall(5, at(3000, 0), at(3000, 4000))};
  const Drawing drawing = draft_walls(walls);
  std::ranges::reverse(walls);
  const Drawing again = draft_walls(walls);

  ASSERT_EQ(again.polygons.size(), drawing.polygons.size());
  ASSERT_EQ(again.texts.size(), drawing.texts.size());
  for (std::size_t index = 0; index < drawing.texts.size(); ++index) {
    EXPECT_EQ(again.texts[index].element, drawing.texts[index].element);
    expect_near(again.texts[index].at, drawing.texts[index].at);
  }
}

}  // namespace
}  // namespace otves::drafting
