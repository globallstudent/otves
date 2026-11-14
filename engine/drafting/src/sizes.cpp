#include "sizes.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/drafting/plan.h>
#include <otves/geom/openings.h>
#include <otves/geom/spaces.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

#include "rooms.h"

namespace otves::drafting {
namespace {

// Layout, in heights of the label text, so sizes read the same at every zoom.
constexpr double kChainInset = 1.2;     // from a wall's face to the chain along it
constexpr double kClearInset = 2.6;     // from a space's top and left to its clear sizes
constexpr double kClearStep = 1.3;      // how much further in a clear size goes to miss a number
constexpr double kOverallOffset = 2.0;  // from the plan's outer face to its overall size
constexpr double kTick = 0.35;          // half the length of a tick
constexpr double kTextGap = 0.15;       // between a dimension line and its number

// A number takes at most this share of the piece of line it is written by.
constexpr double kTextFill = 0.9;

// Thousands are set apart by a space about half a digit wide.
constexpr double kGroupSpace = 0.5;
constexpr std::size_t kGroup = 3;

// Lengths below this are rounding noise: inputs sit on a 1 mm grid.
constexpr double kEpsilon = 1e-6;

// One size on a dimension line: the piece between two points of it, with its number.
struct Piece {
  double from = 0.0;  // along the line, from its first point
  double to = 0.0;
  TextKind kind = TextKind::kOverall;
  std::int64_t value = 0;
  ElementId element;
  Mm fixed;  // see Text::from
  bool forward = false;
  ElementId other{};  // NOLINT(readability-redundant-member-init)
};

// A dimension line from `start` along the unit direction `dir`, with its pieces in order.
struct Chain {
  geom::Vec2 start;
  geom::Vec2 dir;
  Stroke stroke = Stroke::kDimension;
  std::vector<Piece> pieces;
};

double text_width(std::int64_t value, TextSize text) {
  const std::size_t digits = std::to_string(value).size();
  // One space before each whole group of three digits after the first.
  const std::size_t spaces = (digits - 1) / kGroup;
  return (static_cast<double>(digits) + (static_cast<double>(spaces) * kGroupSpace)) * text.digit;
}

// The line, a tick at each end of each piece, and each number that fits beside its piece: over
// the line, as the text reads.
void write(const Chain& chain, TextSize text, Drawing& drawing) {
  if (chain.pieces.empty()) {
    return;
  }
  const auto at = [&chain](double along) { return chain.start + (chain.dir * along); };
  drawing.lines.push_back({.element = {},
                           .stroke = chain.stroke,
                           .from = at(chain.pieces.front().from),
                           .to = at(chain.pieces.back().to)});
  // Text never reads towards the left or straight down.
  const bool turned =
      chain.dir.x < -kEpsilon || (std::abs(chain.dir.x) <= kEpsilon && chain.dir.y < 0.0);
  const geom::Vec2 reading = turned ? -chain.dir : chain.dir;
  const geom::Vec2 over = geom::perp(reading);
  const geom::Vec2 slant = (reading + over) * (kTick * text.height / std::numbers::sqrt2);
  const auto tick = [&](double along) {
    drawing.lines.push_back({.element = {},
                             .stroke = chain.stroke,
                             .from = at(along) - slant,
                             .to = at(along) + slant});
  };
  tick(chain.pieces.front().from);
  for (const Piece& piece : chain.pieces) {
    tick(piece.to);
    if (text_width(piece.value, text) > (piece.to - piece.from) * kTextFill) {
      continue;
    }
    drawing.texts.push_back(
        {.element = piece.element,
         .kind = piece.kind,
         .value = piece.value,
         .at = at((piece.from + piece.to) / 2) + (over * ((kTextGap + (1.0 / 2)) * text.height)),
         .along = reading,
         .from = piece.fixed,
         .forward = piece.forward,
         .other = piece.other});
  }
}

// An opening as a stretch of one side of a space, from the side's first corner.
struct Gap {
  double from = 0.0;
  double to = 0.0;
  ElementId opening;
};

// The openings of the side's wall that lie on the side, in order along it.
std::vector<Gap> gaps_of(const geom::Side& side, std::span<const geom::PlacedOpening> openings) {
  std::vector<Gap> gaps;
  for (const geom::PlacedOpening& placed : openings) {
    if (placed.opening.wall != side.wall) {
      continue;
    }
    if (const auto span = geom::span_on(side, placed.opening)) {
      gaps.push_back({.from = span->from, .to = span->to, .opening = placed.id});
    }
  }
  std::ranges::sort(gaps, {}, &Gap::from);
  return gaps;
}

// Wall, opening, wall along a side: each piece of wall leads to the opening after it, and the
// last one back to the opening before it.
Chain chain_along(const geom::Side& side, std::span<const Gap> gaps, TextSize text) {
  Chain chain{.start = side.corner + (geom::perp(side.dir) * (kChainInset * text.height)),
              .dir = side.dir,
              .stroke = Stroke::kChain,
              .pieces = {}};
  double reached = 0.0;
  for (const Gap& gap : gaps) {
    if (gap.from - reached > kEpsilon) {
      chain.pieces.push_back({.from = reached,
                              .to = gap.from,
                              .kind = TextKind::kPier,
                              .value = round_to_mm(gap.from - reached).value(),
                              .element = gap.opening,
                              .fixed = round_to_mm(side.on_wall(reached)),
                              .forward = side.same});
    }
    chain.pieces.push_back({.from = gap.from,
                            .to = gap.to,
                            .kind = TextKind::kOpeningWidth,
                            .value = round_to_mm(gap.to - gap.from).value(),
                            .element = gap.opening,
                            .fixed = {}});
    reached = gap.to;
  }
  if (side.length - reached > kEpsilon) {
    chain.pieces.push_back({.from = reached,
                            .to = side.length,
                            .kind = TextKind::kPier,
                            .value = round_to_mm(side.length - reached).value(),
                            .element = gaps.back().opening,
                            .fixed = round_to_mm(side.on_wall(side.length)),
                            .forward = !side.same});
  }
  return chain;
}

// The wall whose face bounds the space where a line across the plan (or up it) at `line`
// reaches `at`, if that face stands square to the line; else nil. The outline of an island has
// no walls to name.
ElementId wall_at(const geom::Space& space, bool across, double line, double at) {
  const auto along = [across](geom::Vec2 point) { return across ? point.x : point.y; };
  const auto level = [across](geom::Vec2 point) { return across ? point.y : point.x; };
  for (std::size_t i = 0; i < space.outline.size(); ++i) {
    const geom::Vec2 a = space.outline[i];
    const geom::Vec2 b = space.outline[(i + 1) % space.outline.size()];
    const bool square = std::abs(along(a) - along(b)) <= kEpsilon;
    const bool reached = std::abs(along(a) - at) <= kEpsilon &&
                         line >= std::min(level(a), level(b)) &&
                         line <= std::max(level(a), level(b));
    if (square && reached) {
      return space.sides[i];
    }
  }
  return {};
}

// The clear size of a space along a stretch of a line across the plan (or up it): a handle
// when a wall stands square to it at each end.
Chain clear_size(const geom::Space& space, bool across, double line, Stretch stretch) {
  const ElementId near = wall_at(space, across, line, stretch.from);
  const ElementId far = wall_at(space, across, line, stretch.to);
  const bool handle = !near.is_nil() && !far.is_nil() && near != far;
  return {.start = across ? geom::Vec2{.x = stretch.from, .y = line}
                          : geom::Vec2{.x = line, .y = stretch.from},
          .dir = across ? geom::Vec2{.x = 1.0, .y = 0.0} : geom::Vec2{.x = 0.0, .y = 1.0},
          .stroke = Stroke::kChain,
          .pieces = {{.from = 0.0,
                      .to = stretch.length(),
                      .kind = TextKind::kClear,
                      .value = round_to_mm(stretch.length()).value(),
                      .element = handle ? far : ElementId{},
                      .fixed = {},
                      .forward = false,
                      .other = handle ? near : ElementId{}}}};
}

// Whether a line across the plan (or up it) at `line`, along `stretch`, would run through one
// of the numbers already written: a dimension line may cross another line, not a number.
bool strikes(std::span<const Text> written, bool across, double line, Stretch stretch,
             TextSize text) {
  return std::ranges::any_of(written, [&](const Text& number) {
    const double width = text_width(number.value, text);
    // The number's box, by how far it reaches along the line and across it.
    const double along_line = std::abs(across ? number.along.x : number.along.y);
    const double reach = ((along_line * width) + ((1.0 - along_line) * text.height)) / 2;
    const double rise = ((along_line * text.height) + ((1.0 - along_line) * width)) / 2;
    const double at = across ? number.at.x : number.at.y;
    const double level = across ? number.at.y : number.at.x;
    return std::abs(level - line) < rise + (kTextGap * text.height) && at + reach > stretch.from &&
           at - reach < stretch.to;
  });
}

// One piece from end to end.
Chain single(geom::Vec2 from, geom::Vec2 dir, double length, Stroke stroke, TextKind kind) {
  return {.start = from,
          .dir = dir,
          .stroke = stroke,
          .pieces = {{.from = 0.0,
                      .to = length,
                      .kind = kind,
                      .value = round_to_mm(length).value(),
                      .element = {},
                      .fixed = {}}}};
}

}  // namespace

void write_overall(std::span<const geom::WallFootprint> footprints, TextSize text,
                   Drawing& drawing) {
  std::optional<geom::Vec2> low;
  std::optional<geom::Vec2> high;
  for (const geom::WallFootprint& footprint : footprints) {
    for (const geom::Vec2 point : footprint.outline) {
      low =
          low ? geom::Vec2{.x = std::min(low->x, point.x), .y = std::min(low->y, point.y)} : point;
      high = high ? geom::Vec2{.x = std::max(high->x, point.x), .y = std::max(high->y, point.y)}
                  : point;
    }
  }
  if (!low || !high) {
    return;
  }
  const double offset = kOverallOffset * text.height;
  const double reach = offset + (kTick * text.height);
  const geom::Vec2 across{.x = 1.0, .y = 0.0};
  const geom::Vec2 up{.x = 0.0, .y = 1.0};
  const geom::Vec2 top_left{.x = low->x, .y = high->y};
  write(single(top_left + (up * offset), across, high->x - low->x, Stroke::kDimension,
               TextKind::kOverall),
        text, drawing);
  write(single(*low - (across * offset), up, high->y - low->y, Stroke::kDimension,
               TextKind::kOverall),
        text, drawing);
  // From the plan's corners out to the dimension lines.
  const auto lead = [&drawing](geom::Vec2 from, geom::Vec2 by) {
    drawing.lines.push_back(
        {.element = {}, .stroke = Stroke::kDimension, .from = from, .to = from + by});
  };
  lead(top_left, up * reach);
  lead(*high, up * reach);
  lead(top_left, across * -reach);
  lead(*low, across * -reach);
}

void write_opening_sizes(std::span<const RoomSpace> spaces, std::span<const geom::WallInput> walls,
                         std::span<const geom::PlacedOpening> openings, ElementId selected,
                         TextSize text, Drawing& drawing) {
  const auto opening = std::ranges::find(openings, selected, &geom::PlacedOpening::id);
  if (opening == openings.end()) {
    return;
  }
  // The side the leaf opens to is the one to measure a door from; any side will do otherwise.
  const bool to_left = opening->opening.swing == Swing::kLeft;
  std::optional<geom::Side> found;
  for (const RoomSpace& room : spaces) {
    for (const geom::Side& side : geom::sides_of(room.space, walls)) {
      const bool holds =
          side.wall == opening->opening.wall && geom::span_on(side, opening->opening);
      if (holds && (!found || side.same == to_left)) {
        found = side;
      }
    }
  }
  if (found) {
    write(chain_along(*found, gaps_of(*found, openings), text), text, drawing);
  }
}

namespace {

// What a space's sizes are written with: the stroke of their lines and, on a sheet, the
// openings that already stand in a chain, sorted.
struct Sizing {
  Stroke stroke = Stroke::kChain;
  std::vector<ElementId>* chained = nullptr;
};

// Whether a chain along the side would say nothing new.
bool is_chained(std::span<const Gap> gaps, const std::vector<ElementId>* chained) {
  return chained != nullptr && std::ranges::all_of(gaps, [chained](const Gap& gap) {
           return std::ranges::binary_search(*chained, gap.opening);
         });
}

// What is written of a space so far: its numbers and the lines they stand by.
struct Written {
  std::span<const Text> numbers;
  std::span<const Line> lines;
};

// Whether one of the lines runs the same way within half a step of a line across the plan (or
// up it) at `level`, along `stretch`: a chain that the clear size would lie on.
bool runs_beside(std::span<const Line> lines, bool across, double level, Stretch stretch,
                 TextSize text) {
  const auto along = [across](geom::Vec2 point) { return across ? point.x : point.y; };
  const auto at = [across](geom::Vec2 point) { return across ? point.y : point.x; };
  return std::ranges::any_of(lines, [&](const Line& line) {
    return std::abs(at(line.from) - at(line.to)) <= kEpsilon &&
           std::abs(at(line.from) - level) < kClearStep * text.height / 2 &&
           std::max(along(line.from), along(line.to)) > stretch.from &&
           std::min(along(line.from), along(line.to)) < stretch.to;
  });
}

// Which clear size is placed, and for what.
struct Clear {
  bool across = true;  // across the plan, near the top; else up it, near the left
  bool sheet = false;  // beside the room's label, which stands at its label point
};

// Where a clear size's line runs, and the stretch of floor it spans.
struct ClearLine {
  double level = 0.0;
  Stretch stretch;
};

// A line that would run through a number written before it moves further in, a step at a
// time. Where that brings it to the label point, a sheet has it by the wall instead, if no
// chain runs there and it strikes nothing; else it runs through the label point, as it does in
// a low or narrow space.
ClearLine clear_line(const geom::Space& space, const RoomLabel& label, Clear clear, Written written,
                     TextSize text) {
  const bool across = clear.across;
  const double middle = across ? label.at.y : label.at.x;
  const Stretch whole = across ? label.wide : label.tall;
  // From the top down, or from the left in: `in` is how far from that wall.
  const auto level_at = [&](double in) {
    return across ? label.tall.to - in : label.wide.from + in;
  };
  const auto stretch_at = [&](double level) {
    return (across ? stretch_across(space, level, label.at.x)
                   : stretch_up(space, level, label.at.y))
        .value_or(whole);
  };
  const auto reaches_middle = [&](double level) {
    return across ? level <= middle : level >= middle;
  };
  for (double in = kClearInset * text.height;; in += kClearStep * text.height) {
    const double level = level_at(in);
    if (reaches_middle(level) || text.height <= 0.0) {
      break;
    }
    const Stretch stretch = stretch_at(level);
    if (!strikes(written.numbers, across, level, stretch, text)) {
      return {.level = level, .stretch = stretch};
    }
  }
  const double by_wall = level_at(kChainInset * text.height);
  const Stretch there = stretch_at(by_wall);
  if (clear.sheet && !reaches_middle(by_wall) &&
      !strikes(written.numbers, across, by_wall, there, text) &&
      !runs_beside(written.lines, across, by_wall, there, text)) {
    return {.level = by_wall, .stretch = there};
  }
  return {.level = middle, .stretch = whole};
}

void write_sizes_of(const RoomSpace& room, std::span<const geom::WallInput> walls,
                    std::span<const geom::PlacedOpening> openings, Sizing sizing, TextSize text,
                    Drawing& drawing) {
  const geom::Space& space = room.space;
  const std::size_t first = drawing.texts.size();  // the numbers of this space come after
  const std::size_t first_line = drawing.lines.size();
  for (const geom::Side& side : geom::sides_of(space, walls)) {
    const std::vector<Gap> gaps = gaps_of(side, openings);
    if (gaps.empty() || is_chained(gaps, sizing.chained)) {
      continue;
    }
    Chain chain = chain_along(side, gaps, text);
    chain.stroke = sizing.stroke;
    write(chain, text, drawing);
    if (sizing.chained != nullptr) {
      for (const Gap& gap : gaps) {
        sizing.chained->insert(std::ranges::lower_bound(*sizing.chained, gap.opening), gap.opening);
      }
    }
  }
  const std::optional<RoomLabel> label = label_of(space, text);
  if (!label) {
    return;
  }
  // Across near the top and up near the left, each clear of a chain along that side, so the
  // two lines cross in a corner.
  const bool sheet = sizing.chained != nullptr;
  for (const bool across : {true, false}) {
    const Written written{.numbers = std::span<const Text>{drawing.texts}.subspan(first),
                          .lines = std::span<const Line>{drawing.lines}.subspan(first_line)};
    const ClearLine line =
        clear_line(space, *label, {.across = across, .sheet = sheet}, written, text);
    Chain chain = clear_size(space, across, line.level, line.stretch);
    chain.stroke = sizing.stroke;
    write(chain, text, drawing);
  }
}

}  // namespace

void write_sizes(const RoomSpace& room, std::span<const geom::WallInput> walls,
                 std::span<const geom::PlacedOpening> openings, Stroke stroke, TextSize text,
                 Drawing& drawing) {
  write_sizes_of(room, walls, openings, {.stroke = stroke, .chained = nullptr}, text, drawing);
}

void write_sheet_sizes(std::span<const RoomSpace> spaces, std::span<const geom::WallInput> walls,
                       std::span<const geom::PlacedOpening> openings, TextSize text,
                       Drawing& drawing) {
  // How narrow a room is: the shorter of the stretches through its label point.
  struct Order {
    const RoomSpace* room = nullptr;
    double narrow = 0.0;
  };
  std::vector<Order> order;
  order.reserve(spaces.size());
  for (const RoomSpace& room : spaces) {
    const std::optional<RoomLabel> label = label_of(room.space, text);
    order.push_back({.room = &room,
                     .narrow = label ? std::min(label->wide.length(), label->tall.length()) : 0.0});
  }
  std::ranges::stable_sort(order, std::ranges::greater{}, &Order::narrow);
  std::vector<ElementId> chained;
  for (const Order& next : order) {
    write_sizes_of(*next.room, walls, openings, {.stroke = Stroke::kDimension, .chained = &chained},
                   text, drawing);
  }
}

}  // namespace otves::drafting
