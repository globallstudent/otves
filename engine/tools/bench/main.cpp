// How long the engine takes to derive a level: wall joins, the spaces the walls close and the
// drawing with its layers, rooms, openings and sizes. A touch on the plan asks for the drawing
// again, so the screen's must fit in a frame (docs/roadmap-drawing.md). Run it from a release
// build: `otves_bench` prints a table, `otves_bench 8.3` also fails when the screen's drawing
// of the largest plan takes longer than that many milliseconds.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <ratio>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/build.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/room.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall_type.h>
#include <otves/drafting/plan.h>
#include <otves/geom/openings.h>
#include <otves/geom/spaces.h>
#include <otves/geom/wall_joins.h>

namespace {

using namespace otves;
using namespace otves::literals;

// A room of the grid, between the walls' lines.
constexpr std::int64_t kRoomWidth = 4000;
constexpr std::int64_t kRoomDepth = 3000;

// Each timing is the best of this many runs.
constexpr int kRuns = 5;

// Label text 300 mm high on the plan, its digits 150 mm wide.
constexpr drafting::TextSize kText{.height = 0.3, .digit = 0.15};

// A storey to derive: a grid of rooms with a window in every outside wall and a door in every
// wall between two rooms. Outside walls are of three layers.
struct Storey {
  std::vector<geom::WallInput> walls;
  std::vector<geom::PlacedOpening> openings;
  std::vector<drafting::Marked> marked;
  std::vector<drafting::WallBuild> builds;
  std::vector<drafting::NamedRoom> rooms;
};

struct Grid {
  int columns = 0;
  int rows = 0;
};

// An outside wall: facing brick, insulation and block from its outer face in. A wall between
// two rooms is half a brick.
constexpr BuiltLayer kFacing{.function = LayerFunction::kFacing,
                             .thickness = 120_mm,
                             .unit_length = 250_mm,
                             .unit_width = 120_mm,
                             .unit_height = 65_mm};
constexpr BuiltLayer kInsulation{.function = LayerFunction::kInsulation,
                                 .thickness = 100_mm,
                                 .unit_length = {},
                                 .unit_width = {},
                                 .unit_height = {}};
constexpr BuiltLayer kBlock{.function = LayerFunction::kStructure,
                            .thickness = 190_mm,
                            .unit_length = 390_mm,
                            .unit_width = 190_mm,
                            .unit_height = 188_mm};
constexpr Mm kOutside = kFacing.thickness + kInsulation.thickness + kBlock.thickness;
constexpr Mm kPartition = 120_mm;
constexpr Mm kWallHeight = 2800_mm;

// A window in an outside wall and a door in a partition, each a metre from its wall's start.
constexpr Opening kWindow{.wall = {},
                          .offset = 1000_mm,
                          .width = 1500_mm,
                          .height = 1500_mm,
                          .sill = 900_mm,
                          .kind = OpeningKind::kWindow};
constexpr Opening kDoor{.wall = {},
                        .offset = 1000_mm,
                        .width = 900_mm,
                        .height = 2100_mm,
                        .sill = 0_mm,
                        .kind = OpeningKind::kDoor};

class Builder {
 public:
  // A wall from one crossing of the grid to the next, with its opening.
  void add(PointMm start, PointMm end, bool outside) {
    const geom::WallInput wall{.id = next(),
                               .wall = {.start = start,
                                        .end = end,
                                        .thickness = outside ? kOutside : kPartition,
                                        .height = kWallHeight}};
    Opening opening = outside ? kWindow : kDoor;
    opening.wall = wall.id;
    storey_.walls.push_back(wall);
    storey_.openings.push_back({.id = next(), .wall = wall.wall, .opening = opening});
    storey_.marked.push_back({.id = storey_.openings.back().id, .opening = opening});
    if (outside) {
      storey_.builds.push_back({.wall = wall.id, .layers = {kFacing, kInsulation, kBlock}});
    }
  }

  void name(PointMm anchor) {
    storey_.rooms.push_back(
        {.id = next(), .room = {.level = {}, .anchor = anchor, .kind = RoomKind::kBedroom}});
  }

  [[nodiscard]] Storey take() { return std::move(storey_); }

 private:
  ElementId next() { return ElementId{Uuid::v7(++made_, 0, 0)}; }

  Storey storey_;
  std::int64_t made_ = 0;
};

Storey storey_of(Grid grid) {
  Builder builder;
  const auto at = [](int column, int row) {
    return PointMm{.x = Mm{column * kRoomWidth}, .y = Mm{row * kRoomDepth}};
  };
  for (int row = 0; row <= grid.rows; ++row) {
    for (int column = 0; column < grid.columns; ++column) {
      builder.add(at(column, row), at(column + 1, row), row == 0 || row == grid.rows);
    }
  }
  for (int column = 0; column <= grid.columns; ++column) {
    for (int row = 0; row < grid.rows; ++row) {
      builder.add(at(column, row), at(column, row + 1), column == 0 || column == grid.columns);
    }
  }
  for (int row = 0; row < grid.rows; ++row) {
    for (int column = 0; column < grid.columns; ++column) {
      builder.name({.x = Mm{(column * kRoomWidth) + (kRoomWidth / 2)},
                    .y = Mm{(row * kRoomDepth) + (kRoomDepth / 2)}});
    }
  }
  return builder.take();
}

// The shortest of several runs, in milliseconds.
template <typename Run>
double best_ms(const Run& run) {
  double best = 0.0;
  for (int i = 0; i < kRuns; ++i) {
    const auto start = std::chrono::steady_clock::now();
    run();
    const std::chrono::duration<double, std::milli> took = std::chrono::steady_clock::now() - start;
    best = i == 0 ? took.count() : std::min(best, took.count());
  }
  return best;
}

// What deriving a storey took, in milliseconds.
struct Timing {
  double joins = 0.0;
  double spaces = 0.0;
  double screen = 0.0;   // nothing selected
  double focused = 0.0;  // a room in focus, drawn with its sizes
  double sheet = 0.0;    // every room with its sizes, for a sheet
};

Timing time(const Storey& storey) {
  drafting::Plan plan{.walls = storey.walls,
                      .openings = storey.openings,
                      .below = {},
                      .builds = storey.builds,
                      .rooms = storey.rooms,
                      .marked = storey.marked};
  // Results are counted so that the work is not optimised away.
  std::size_t made = 0;
  Timing timing;
  timing.joins = best_ms([&] { made += geom::join_walls(storey.walls).footprints.size(); });
  timing.spaces = best_ms([&] { made += geom::find_spaces(storey.walls).size(); });
  timing.screen = best_ms([&] { made += drafting::draft(plan, kText).texts.size(); });
  plan.focus = PointMm{.x = Mm{kRoomWidth / 2}, .y = Mm{kRoomDepth / 2}};
  timing.focused = best_ms([&] { made += drafting::draft(plan, kText).texts.size(); });
  plan.focus.reset();
  plan.sheet = true;
  timing.sheet = best_ms([&] { made += drafting::draft(plan, kText).texts.size(); });
  return made > 0 ? timing : Timing{};
}

// Prints the table. Fails when a budget is given, in milliseconds, and the screen's drawing of
// the largest plan takes longer.
int run(std::span<char*> arguments) {
  // A house, a large house, a block of flats' floor, and more than any job should hold.
  constexpr std::array grids{Grid{.columns = 3, .rows = 3}, Grid{.columns = 5, .rows = 5},
                             Grid{.columns = 8, .rows = 9}, Grid{.columns = 15, .rows = 16}};
  std::cout << std::format("{:>6}{:>10}{:>7}{:>10}{:>10}{:>10}{:>10}{:>10}\n", "walls", "openings",
                           "rooms", "joins", "spaces", "screen", "focused", "sheet");
  double largest = 0.0;
  for (const Grid grid : grids) {
    const Storey storey = storey_of(grid);
    const Timing timing = time(storey);
    largest = timing.screen;
    std::cout << std::format("{:>6}{:>10}{:>7}{:>10.2f}{:>10.2f}{:>10.2f}{:>10.2f}{:>10.2f}\n",
                             storey.walls.size(), storey.openings.size(), storey.rooms.size(),
                             timing.joins, timing.spaces, timing.screen, timing.focused,
                             timing.sheet);
  }
  std::cout << "milliseconds, the best of " << kRuns << " runs\n";
  const double budget = arguments.size() > 1 ? std::strtod(arguments[1], nullptr) : 0.0;
  if (budget > 0.0 && largest > budget) {
    std::cout << std::format("over the budget of {:.1f} ms\n", budget);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run({argv, static_cast<std::size_t>(argc)});
  } catch (...) {
    return EXIT_FAILURE;
  }
}
