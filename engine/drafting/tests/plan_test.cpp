#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/build.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/core/wall_type.h>
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

// Facing brick, insulation and block from the outer face in: 410 mm.
std::vector<BuiltLayer> outside_layers() {
  const auto laid = [](LayerFunction function, Mm thickness, Mm length, Mm width, Mm height) {
    return BuiltLayer{.function = function,
                      .thickness = thickness,
                      .unit_length = length,
                      .unit_width = width,
                      .unit_height = height};
  };
  return {laid(LayerFunction::kFacing, 120_mm, 250_mm, 120_mm, 65_mm),
          laid(LayerFunction::kInsulation, 100_mm, 0_mm, 0_mm, 0_mm),
          laid(LayerFunction::kStructure, 190_mm, 390_mm, 190_mm, 188_mm)};
}

TEST(Draft, AWallOfSeveralLayersIsItsLayersUnderItsOutline) {
  std::vector walls{wall(1, at(0, 0), at(6000, 0))};
  walls[0].wall.thickness = 410_mm;
  const std::vector<WallBuild> builds{{.wall = id(1), .layers = outside_layers()}};

  const Drawing drawing =
      draft({.walls = walls, .openings = {}, .below = {}, .builds = builds}, kText);

  ASSERT_EQ(drawing.polygons.size(), 4U);
  EXPECT_EQ(drawing.polygons[0].fill, Fill::kFacing);
  EXPECT_EQ(drawing.polygons[1].fill, Fill::kInsulation);
  EXPECT_EQ(drawing.polygons[2].fill, Fill::kWall);
  // Every layer is the wall's, edged as a layer.
  EXPECT_EQ(std::ranges::count(drawing.polygons, id(1), &Polygon::element), 4);
  EXPECT_EQ(std::ranges::count(drawing.polygons, Stroke::kLayer, &Polygon::stroke), 3);
  // The facing is the wall's right 120 mm.
  expect_near(drawing.polygons[0].points[0], {.x = 0.0, .y = -0.205});
  expect_near(drawing.polygons[0].points[2], {.x = 6.0, .y = -0.085});
  // The edge of the whole wall, with nothing in it, over the layers.
  EXPECT_EQ(drawing.polygons[3].fill, Fill::kNone);
  EXPECT_EQ(drawing.polygons[3].stroke, Stroke::kWallEdge);
  EXPECT_EQ(drawing.polygons[3].points.size(), 4U);
  // The length stands clear of the wall's 410 mm.
  expect_near(drawing.texts[0].at, {.x = 3.0, .y = 0.205 + 0.075 + 0.15});
}

TEST(Draft, AWallOfOneLayerIsFilledByWhatThatLayerIs) {
  std::vector walls{wall(1, at(0, 0), at(6000, 0)), wall(2, at(0, 3000), at(6000, 3000)),
                    wall(3, at(0, 6000), at(6000, 6000))};
  const std::vector<BuiltLayer> block{outside_layers()[2]};
  std::vector<BuiltLayer> unchosen = block;
  unchosen[0].unit_length = 0_mm;
  unchosen[0].unit_width = 0_mm;
  unchosen[0].unit_height = 0_mm;
  // The first wall is of a type whose layer has a unit, though the wall itself has none.
  walls[0].wall.unit_length = 0_mm;
  walls[0].wall.unit_width = 0_mm;
  walls[0].wall.unit_height = 0_mm;
  const std::vector<WallBuild> builds{{.wall = id(1), .layers = block},
                                      {.wall = id(2), .layers = unchosen},
                                      {.wall = id(3), .layers = {}}};

  const Drawing drawing =
      draft({.walls = walls, .openings = {}, .below = {}, .builds = builds}, kText);

  ASSERT_EQ(drawing.polygons.size(), 3U);
  EXPECT_EQ(drawing.polygons[0].fill, Fill::kWall);
  EXPECT_EQ(drawing.polygons[0].stroke, Stroke::kWallEdge);
  EXPECT_EQ(drawing.polygons[1].fill, Fill::kUnchosen);
  EXPECT_EQ(drawing.polygons[2].fill, Fill::kUnchosen);
}

TEST(Draft, ALengthIsWrittenByTheLeftFaceAtTheMiddle) {
  const Drawing drawing = draft_walls({wall(1, at(0, 0), at(6000, 0))});

  ASSERT_EQ(drawing.texts.size(), 1U);
  const Text& length = drawing.texts[0];
  EXPECT_EQ(length.element, id(1));
  EXPECT_EQ(length.kind, TextKind::kLength);
  EXPECT_EQ(length.value, 6000);
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

// A door 900 mm wide, 1 m along a 6 m wall of 380 mm on its centre line.
Drawing draft_door(Hinge hinge, Swing swing) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};
  const std::vector<geom::PlacedOpening> openings{{.id = id(2),
                                                   .wall = walls[0].wall,
                                                   .opening = {.wall = id(1),
                                                               .offset = 1000_mm,
                                                               .width = 900_mm,
                                                               .height = 2100_mm,
                                                               .sill = 0_mm,
                                                               .kind = OpeningKind::kDoor,
                                                               .hinge = hinge,
                                                               .swing = swing}}};
  return draft({.walls = walls, .openings = openings, .below = {}}, kText);
}

// The leaf of the drawing's one door: from its hinge to its tip.
Line leaf_of(const Drawing& drawing) {
  const auto leaf = std::ranges::find(drawing.lines, Stroke::kLeaf, &Line::stroke);
  return leaf != drawing.lines.end() ? *leaf : Line{};
}

TEST(Draft, ADoorHangsOnEitherEdgeAndOpensToEitherSide) {
  // On the far edge, open to the left: the leaf stands up from the left face at 1.9 m.
  const Drawing far_left = draft_door(Hinge::kFar, Swing::kLeft);
  expect_near(leaf_of(far_left).from, {.x = 1.9, .y = 0.19});
  expect_near(leaf_of(far_left).to, {.x = 1.9, .y = 1.09});
  // Its swing runs counter-clockwise from the tip round to the near jamb.
  ASSERT_EQ(far_left.arcs.size(), 1U);
  expect_near(far_left.arcs[0].centre, {.x = 1.9, .y = 0.19});
  expect_near(far_left.arcs[0].from, {.x = 1.9, .y = 1.09});
  expect_near(far_left.arcs[0].to, {.x = 1.0, .y = 0.19});

  // On the near edge, open to the right: down from the right face at 1 m.
  const Drawing near_right = draft_door(Hinge::kNear, Swing::kRight);
  expect_near(leaf_of(near_right).from, {.x = 1.0, .y = -0.19});
  expect_near(leaf_of(near_right).to, {.x = 1.0, .y = -1.09});
  expect_near(near_right.arcs[0].from, {.x = 1.0, .y = -1.09});
  expect_near(near_right.arcs[0].to, {.x = 1.9, .y = -0.19});

  const Drawing far_right = draft_door(Hinge::kFar, Swing::kRight);
  expect_near(leaf_of(far_right).from, {.x = 1.9, .y = -0.19});
  expect_near(leaf_of(far_right).to, {.x = 1.9, .y = -1.09});
  expect_near(far_right.arcs[0].from, {.x = 1.0, .y = -0.19});
  expect_near(far_right.arcs[0].to, {.x = 1.9, .y = -1.09});
}

Marked marked(std::int64_t n, OpeningKind kind, Mm width, Mm height) {
  return {.id = id(n),
          .opening = {.wall = id(100),
                      .offset = 0_mm,
                      .width = width,
                      .height = height,
                      .sill = 0_mm,
                      .kind = kind}};
}

std::vector<std::int64_t> numbers(const std::vector<Mark>& marks) {
  std::vector<std::int64_t> all;
  all.reserve(marks.size());
  for (const Mark& mark : marks) {
    all.push_back(mark.number);
  }
  return all;
}

TEST(MarkOpenings, OpeningsOfOneSizeShareAMarkNumberedByTheOldestOfEachSize) {
  // Given youngest first: the order of the ids decides, not the order given.
  const std::vector<Marked> openings{
      marked(6, OpeningKind::kWindow, 1200_mm, 1500_mm),
      marked(5, OpeningKind::kDoor, 900_mm, 2100_mm),
      marked(4, OpeningKind::kWindow, 1500_mm, 1500_mm),
      marked(3, OpeningKind::kWindow, 1200_mm, 1200_mm),
      marked(2, OpeningKind::kDoor, 800_mm, 2100_mm),
      marked(1, OpeningKind::kWindow, 1500_mm, 1500_mm),
  };
  const std::vector<Mark> marks = mark_openings(openings);

  ASSERT_EQ(marks.size(), 6U);
  EXPECT_EQ(marks[0].opening, id(1));
  // Windows: 1 500 × 1 500 first, 1 200 × 1 200 second, 1 200 × 1 500 third. Doors count apart.
  EXPECT_EQ(numbers(marks), (std::vector<std::int64_t>{1, 1, 2, 1, 2, 3}));
}

TEST(Draft, AMarkReadsAlongItsWallFromTheBottomUp) {
  // One wall drawn up the plan and one drawn down it, each with a door.
  const std::vector walls{wall(1, at(0, 0), at(0, 6000)), wall(2, at(5000, 6000), at(5000, 0))};
  const auto door = [&walls](std::int64_t n, std::size_t in) {
    return geom::PlacedOpening{.id = id(n),
                               .wall = walls[in].wall,
                               .opening = {.wall = walls[in].id,
                                           .offset = 1000_mm,
                                           .width = 900_mm,
                                           .height = 2100_mm,
                                           .sill = 0_mm,
                                           .kind = OpeningKind::kDoor}};
  };
  const std::vector<geom::PlacedOpening> openings{door(3, 0), door(4, 1)};
  const std::vector<Marked> job{{.id = id(3), .opening = openings[0].opening},
                                {.id = id(4), .opening = openings[1].opening}};

  const Drawing drawing =
      draft({.walls = walls, .openings = openings, .below = {}, .marked = job}, kText);

  std::vector<Text> written;
  std::ranges::copy_if(drawing.texts, std::back_inserter(written),
                       [](const Text& text) { return text.kind == TextKind::kDoorMark; });
  ASSERT_EQ(written.size(), 2U);
  // Each stands off its wall's right face: to the right of the first, the left of the second.
  expect_near(written[0].at, {.x = 0.19 + 0.3, .y = 1.45});
  expect_near(written[0].along, {.x = 0.0, .y = 1.0});
  expect_near(written[1].at, {.x = 5.0 - 0.19 - 0.3, .y = 6.0 - 1.45});
  expect_near(written[1].along, {.x = 0.0, .y = 1.0});
}

// The marks a drawing has, in order.
std::vector<Text> marks_in(const Drawing& drawing) {
  std::vector<Text> written;
  std::ranges::copy_if(drawing.texts, std::back_inserter(written), [](const Text& text) {
    return text.kind == TextKind::kDoorMark || text.kind == TextKind::kWindowMark ||
           text.kind == TextKind::kEmptyMark;
  });
  return written;
}

TEST(Draft, AMarkStandsBesideItsOpeningOnTheSideItDoesNotOpenTo) {
  const std::vector walls{wall(1, at(0, 0), at(6000, 0))};
  const std::vector<geom::PlacedOpening> openings{
      {.id = id(2),
       .wall = walls[0].wall,
       .opening = {.wall = id(1),
                   .offset = 1000_mm,
                   .width = 900_mm,
                   .height = 2100_mm,
                   .sill = 0_mm,
                   .kind = OpeningKind::kDoor}},
      {.id = id(3),
       .wall = walls[0].wall,
       .opening = {
           .wall = id(1), .offset = 4000_mm, .width = 1500_mm, .height = 1500_mm, .sill = 900_mm}}};
  // An older door of another size stands on another storey: this one is the second of its kind.
  const std::vector<Marked> job{marked(1, OpeningKind::kDoor, 800_mm, 2100_mm),
                                {.id = id(2), .opening = openings[0].opening},
                                {.id = id(3), .opening = openings[1].opening}};

  const Drawing drawing =
      draft({.walls = walls, .openings = openings, .below = {}, .marked = job}, kText);
  const std::vector<Text> written = marks_in(drawing);

  ASSERT_EQ(written.size(), 2U);
  // The door opens to the wall's left, up the plan: its mark stands a text height off the
  // right face, by the door's middle.
  EXPECT_EQ(written[0].element, id(2));
  EXPECT_EQ(written[0].kind, TextKind::kDoorMark);
  EXPECT_EQ(written[0].value, 2);
  expect_near(written[0].at, {.x = 1.45, .y = -0.19 - 0.3});
  // So does the window's, which has no leaf to turn.
  EXPECT_EQ(written[1].kind, TextKind::kWindowMark);
  EXPECT_EQ(written[1].value, 1);
  expect_near(written[1].at, {.x = 4.75, .y = -0.19 - 0.3});
  // The schedule lists every mark of the job once: windows, then doors by their numbers.
  ASSERT_EQ(drawing.openings.size(), 3U);
  EXPECT_EQ(drawing.openings[0].kind, TextKind::kWindowMark);
  EXPECT_EQ(drawing.openings[1].kind, TextKind::kDoorMark);
  EXPECT_EQ(drawing.openings[1].number, 1);
  EXPECT_EQ(drawing.openings[1].width, 800_mm);
  EXPECT_EQ(drawing.openings[2].number, 2);
  EXPECT_EQ(drawing.openings[2].count, 1);
  // An opening the job does not list has no mark.
  EXPECT_TRUE(marks_in(draft({.walls = walls, .openings = openings, .below = {}}, kText)).empty());
}

}  // namespace
}  // namespace otves::drafting
