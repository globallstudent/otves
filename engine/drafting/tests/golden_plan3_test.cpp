#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/room.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/drafting/plan.h>
#include <otves/geom/openings.h>
#include <otves/geom/wall_joins.h>

// «План3», one of the architects' drawings the editor is to match (docs/roadmap-drawing.md): a
// one-storey house 8 300 by 12 800 mm, outer walls 400, a bearing wall 300 down its length and
// partitions 150. The sizes are read off the sheet; the areas are the ones written in its rooms.
namespace otves::drafting {
namespace {

using namespace otves::literals;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

PointMm at(std::int64_t x, std::int64_t y) { return {.x = Mm{x}, .y = Mm{y}}; }

geom::WallInput wall(std::int64_t n, PointMm start, PointMm end, Mm thickness) {
  return {.id = id(n),
          .wall = {.start = start, .end = end, .thickness = thickness, .height = 2800_mm}};
}

// The walls on their centre lines, from the outer bottom left corner of the house.
std::vector<geom::WallInput> walls() {
  return {
      // Outside, counter-clockwise from the bottom.
      wall(1, at(200, 200), at(8100, 200), 400_mm),
      wall(2, at(8100, 200), at(8100, 12600), 400_mm),
      wall(3, at(8100, 12600), at(200, 12600), 400_mm),
      wall(4, at(200, 12600), at(200, 200), 400_mm),
      // The bearing wall: 3 100 clear to its left.
      wall(5, at(3650, 12600), at(3650, 200), 300_mm),
      // Under the kitchen-living room, 8 100 deep, and under the boiler room, 2 000 deep.
      wall(6, at(200, 4225), at(3650, 4225), 150_mm),
      wall(7, at(200, 2075), at(3650, 2075), 150_mm),
      // Under the upper bedroom, 3 300 deep.
      wall(8, at(3650, 9025), at(8100, 9025), 150_mm),
      // Between the corridor and the bedrooms, 2 800 wide, and round the corner cut out of the
      // lower one: 600 by 1 400.
      wall(9, at(5025, 9025), at(5025, 1725), 150_mm),
      wall(10, at(5025, 4875), at(8100, 4875), 150_mm),
      wall(11, at(5025, 1725), at(5625, 1725), 150_mm),
      wall(12, at(5625, 1725), at(5625, 200), 150_mm),
  };
}

// An opening of the sheet: which wall of walls() it is in, by its place there, how far it
// starts from that wall's start and how wide it is.
struct Placed {
  std::int64_t n = 0;
  std::size_t wall = 0;
  Mm offset;
  Mm width;
  OpeningKind kind = OpeningKind::kWindow;
};

// The windows, the doors and the open way from the kitchen-living room to the corridor. The
// sheet gives no heights: windows are taken 1 500 high over a 900 sill, doors 2 100.
std::vector<geom::PlacedOpening> openings() {
  const std::vector<geom::WallInput> in = walls();
  const std::vector<Placed> placed{
      // The bottom wall: the bathroom's window, the way in and the lower bedroom's window.
      {.n = 31, .wall = 0, .offset = 675_mm, .width = 1250_mm},
      {.n = 32, .wall = 0, .offset = 3750_mm, .width = 1040_mm, .kind = OpeningKind::kDoor},
      {.n = 33, .wall = 0, .offset = 5975_mm, .width = 1250_mm},
      // The right wall, the top wall and the left one, each from its start.
      {.n = 34, .wall = 1, .offset = 5950_mm, .width = 1600_mm},
      {.n = 35, .wall = 2, .offset = 950_mm, .width = 1600_mm},
      {.n = 36, .wall = 2, .offset = 5350_mm, .width = 1600_mm},
      {.n = 37, .wall = 3, .offset = 1700_mm, .width = 1800_mm},
      {.n = 38, .wall = 3, .offset = 5200_mm, .width = 1600_mm},
      {.n = 39, .wall = 3, .offset = 9150_mm, .width = 600_mm},
      // The bearing wall: the open way, then the doors of the boiler room and the bathroom.
      {.n = 40, .wall = 4, .offset = 3600_mm, .width = 3200_mm, .kind = OpeningKind::kEmpty},
      {.n = 41, .wall = 4, .offset = 9400_mm, .width = 900_mm, .kind = OpeningKind::kDoor},
      {.n = 42, .wall = 4, .offset = 10750_mm, .width = 800_mm, .kind = OpeningKind::kDoor},
      // The doors of the three bedrooms.
      {.n = 43, .wall = 7, .offset = 300_mm, .width = 900_mm, .kind = OpeningKind::kDoor},
      {.n = 44, .wall = 8, .offset = 3025_mm, .width = 900_mm, .kind = OpeningKind::kDoor},
      {.n = 45, .wall = 8, .offset = 4375_mm, .width = 900_mm, .kind = OpeningKind::kDoor},
  };
  std::vector<geom::PlacedOpening> all;
  all.reserve(placed.size());
  for (const Placed& opening : placed) {
    const bool window = opening.kind == OpeningKind::kWindow;
    all.push_back({.id = id(opening.n),
                   .wall = in[opening.wall].wall,
                   .opening = {.wall = in[opening.wall].id,
                               .offset = opening.offset,
                               .width = opening.width,
                               .height = window ? 1500_mm : 2100_mm,
                               .sill = window ? 900_mm : 0_mm,
                               .kind = opening.kind}});
  }
  return all;
}

NamedRoom room(std::int64_t n, PointMm anchor, RoomKind kind) {
  return {.id = id(n), .room = {.level = id(100), .anchor = anchor, .kind = kind}};
}

std::vector<NamedRoom> rooms() {
  return {room(21, at(2000, 8000), RoomKind::kKitchenLiving),
          room(22, at(6000, 11000), RoomKind::kBedroom),
          room(23, at(6500, 7000), RoomKind::kBedroom),
          room(24, at(6500, 3000), RoomKind::kBedroom),
          room(25, at(4400, 5000), RoomKind::kCorridor),
          room(26, at(2000, 3000), RoomKind::kBoiler),
          room(27, at(2000, 1000), RoomKind::kBathroom)};
}

// Label text as an A4 sheet has it on this plan: 2.5 mm high at about 1:90.
constexpr TextSize kText{.height = 0.223, .digit = 0.108};

// A sheet of the plan.
Drawing sheet() {
  const std::vector<geom::WallInput> built = walls();
  const std::vector<geom::PlacedOpening> cut = openings();
  const std::vector<NamedRoom> named = rooms();
  std::vector<Marked> marked;
  marked.reserve(cut.size());
  for (const geom::PlacedOpening& opening : cut) {
    marked.push_back({.id = opening.id, .opening = opening.opening});
  }
  return draft({.walls = built,
                .openings = cut,
                .below = {},
                .builds = {},
                .rooms = named,
                .marked = marked,
                .sheet = true},
               kText);
}

// The floor area the schedule gives the room, or nothing when it does not list it.
std::optional<double> area_of(const Drawing& drawing, std::int64_t n) {
  const auto listed = std::ranges::find(drawing.rooms, id(n), &ListedRoom::room);
  return listed == drawing.rooms.end() ? std::nullopt : std::optional{listed->area};
}

std::vector<std::int64_t> sizes(const Drawing& drawing, TextKind kind) {
  std::vector<std::int64_t> found;
  for (const Text& text : drawing.texts) {
    if (text.kind == kind) {
      found.push_back(text.value);
    }
  }
  std::ranges::sort(found);
  return found;
}

// The room a text takes on the sheet, along the plan's axes: every text of this plan runs
// across it or up it. A name is taken sixteen digits wide, or as wide as the room it is given
// if that is less, an area seven digits, a mark five, and a number its digits.
struct Taken {
  double left = 0.0;
  double right = 0.0;
  double bottom = 0.0;
  double top = 0.0;
};

Taken taken_by(const Text& text) {
  double width = static_cast<double>(std::to_string(text.value).size()) * kText.digit;
  if (text.kind == TextKind::kRoomName) {
    width = std::min(text.fit, 16 * kText.digit);
  } else if (text.kind == TextKind::kRoomArea) {
    width = 7 * kText.digit;
  } else if (text.kind == TextKind::kWindowMark || text.kind == TextKind::kDoorMark ||
             text.kind == TextKind::kEmptyMark) {
    width = 5 * kText.digit;
  }
  const bool upright = text.along.y != 0.0;
  const double across = (upright ? kText.height : width) / 2;
  const double up = (upright ? width : kText.height) / 2;
  return {.left = text.at.x - across,
          .right = text.at.x + across,
          .bottom = text.at.y - up,
          .top = text.at.y + up};
}

// How many pairs of texts lie on each other by more than `slack` metres.
std::ptrdiff_t overlaps(const Drawing& drawing, double slack) {
  std::ptrdiff_t found = 0;
  for (std::size_t i = 0; i < drawing.texts.size(); ++i) {
    const Taken a = taken_by(drawing.texts[i]);
    found += std::count_if(drawing.texts.begin() + static_cast<std::ptrdiff_t>(i) + 1,
                           drawing.texts.end(), [&a, slack](const Text& other) {
                             const Taken b = taken_by(other);
                             return std::min(a.right, b.right) - std::max(a.left, b.left) > slack &&
                                    std::min(a.top, b.top) - std::max(a.bottom, b.bottom) > slack;
                           });
  }
  return found;
}

TEST(GoldenPlan3, TheWallsJoinWithNothingToFix) {
  EXPECT_TRUE(geom::join_walls(walls()).issues.empty());
}

TEST(GoldenPlan3, TheRoomsHaveTheAreasWrittenOnTheSheet) {
  const Drawing drawing = sheet();

  ASSERT_EQ(drawing.rooms.size(), 7U);
  // Кухня-гостиная 25,1; спальни 13,5, 11,2 and 11,5.
  EXPECT_NEAR(area_of(drawing, 21).value_or(0.0), 3.1 * 8.1, 1e-9);
  EXPECT_NEAR(area_of(drawing, 22).value_or(0.0), 4.1 * 3.3, 1e-9);
  EXPECT_NEAR(area_of(drawing, 23).value_or(0.0), 2.8 * 4.0, 1e-9);
  EXPECT_NEAR(area_of(drawing, 24).value_or(0.0), (2.8 * 4.4) - (0.6 * 1.4), 1e-9);
  // Котельная 6,2 and санузел 5.
  EXPECT_NEAR(area_of(drawing, 26).value_or(0.0), 3.1 * 2.0, 1e-9);
  EXPECT_NEAR(area_of(drawing, 27).value_or(0.0), 3.1 * 1.6, 1e-9);
  // The corridor, 1 150 wide with these partitions, and wider by 600 beside the cut corner.
  // The sheet says 11,6: its partitions there are thinner than 150, which it does not show.
  EXPECT_NEAR(area_of(drawing, 25).value_or(0.0), (1.15 * 8.55) + (0.6 * 1.25), 1e-9);

  // The schedule reads from the top of the plan down: it starts with the upper bedroom and
  // ends with the bathroom.
  EXPECT_EQ(drawing.rooms.front().room, id(22));
  EXPECT_EQ(drawing.rooms.back().room, id(27));
}

TEST(GoldenPlan3, TheSheetHasTheOverallSizeAndTheRoomsClearSizes) {
  const Drawing drawing = sheet();

  EXPECT_EQ(sizes(drawing, TextKind::kOverall), (std::vector<std::int64_t>{8300, 12800}));
  // Each room's size across and up the plan, as the sheet has them inside its rooms.
  const std::vector<std::int64_t> clear = sizes(drawing, TextKind::kClear);
  // The lower bedroom's is taken beside its cut corner: 3 000, not 4 400.
  for (const std::int64_t size : {3100, 8100, 4100, 3300, 2800, 4000, 3000, 2000, 1600}) {
    EXPECT_TRUE(std::ranges::binary_search(clear, size)) << size;
  }
  // Every room keeps its name on a sheet.
  EXPECT_EQ(std::ranges::count(drawing.texts, TextKind::kRoomName, &Text::kind), 7);
}

TEST(GoldenPlan3, AWallBetweenTwoRoomsIsSizedOnceAndNoTextLiesOnAnother) {
  const Drawing drawing = sheet();

  // Fifteen openings, each with one width on the sheet: the corridor does not repeat the
  // chains of the rooms beside it.
  EXPECT_EQ(std::ranges::count(drawing.texts, TextKind::kOpeningWidth, &Text::kind), 15);
  // The chains read as the sheet's do: along the kitchen-living room's side of the bearing
  // wall, 3 400, the open way of 3 200, 1 500.
  const std::vector<std::int64_t> piers = sizes(drawing, TextKind::kPier);
  for (const std::int64_t pier : {3400, 1500, 750, 1750, 3050, 2950, 1950, 475, 1375}) {
    EXPECT_TRUE(std::ranges::binary_search(piers, pier)) << pier;
  }
  // Numbers, names, areas and marks keep off each other. Two chains that meet in a corner
  // may bring their last numbers within a quarter of a text height of each other.
  EXPECT_EQ(overlaps(drawing, kText.height / 4), 0);
}

// A line of the opening schedule, to compare.
struct Scheduled {
  TextKind kind = TextKind::kWindowMark;
  std::int64_t number = 0;
  std::int64_t width = 0;
  std::int64_t count = 0;

  constexpr auto operator<=>(const Scheduled&) const = default;
};

TEST(GoldenPlan3, TheOpeningScheduleCountsEveryMarkOfTheSheet) {
  const Drawing drawing = sheet();
  std::vector<Scheduled> listed;
  listed.reserve(drawing.openings.size());
  for (const ListedOpening& opening : drawing.openings) {
    listed.push_back({.kind = opening.kind,
                      .number = opening.number,
                      .width = opening.width.value(),
                      .count = opening.count});
  }

  // Windows, doors, then the open way, each kind by its numbers.
  EXPECT_EQ(listed, (std::vector<Scheduled>{
                        {.kind = TextKind::kWindowMark, .number = 1, .width = 1250, .count = 2},
                        {.kind = TextKind::kWindowMark, .number = 2, .width = 1600, .count = 4},
                        {.kind = TextKind::kWindowMark, .number = 3, .width = 1800, .count = 1},
                        {.kind = TextKind::kWindowMark, .number = 4, .width = 600, .count = 1},
                        {.kind = TextKind::kDoorMark, .number = 1, .width = 1040, .count = 1},
                        {.kind = TextKind::kDoorMark, .number = 2, .width = 900, .count = 4},
                        {.kind = TextKind::kDoorMark, .number = 3, .width = 800, .count = 1},
                        {.kind = TextKind::kEmptyMark, .number = 1, .width = 3200, .count = 1},
                    }));
}

}  // namespace
}  // namespace otves::drafting
