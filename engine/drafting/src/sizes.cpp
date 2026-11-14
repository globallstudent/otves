#include "sizes.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
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

void write_sizes(const RoomSpace& room, std::span<const geom::WallInput> walls,
                 std::span<const geom::PlacedOpening> openings, Stroke stroke, TextSize text,
                 Drawing& drawing) {
  const geom::Space& space = room.space;
  const std::size_t first = drawing.texts.size();  // the numbers of this space come after
  for (const geom::Side& side : geom::sides_of(space, walls)) {
    const std::vector<Gap> gaps = gaps_of(side, openings);
    if (!gaps.empty()) {
      Chain chain = chain_along(side, gaps, text);
      chain.stroke = stroke;
      write(chain, text, drawing);
    }
  }
  const std::optional<RoomLabel> label = label_of(space, text);
  if (!label) {
    return;
  }
  // Across near the top and up near the left, each clear of a chain along that side, so the
  // two lines cross in a corner. A line that would run through a number written before it
  // moves further in, a step at a time; in a low or narrow space it runs through the middle.
  const auto numbers = [&drawing, first] {
    return std::span<const Text>{drawing.texts}.subspan(first);
  };
  const double inset = kClearInset * text.height;
  const double step = kClearStep * text.height;
  double height = label->tall.to - inset;
  Stretch wide = label->wide;
  for (;; height -= step) {
    if (height <= label->at.y) {
      height = label->at.y;
      wide = label->wide;
      break;
    }
    wide = stretch_across(space, height, label->at.x).value_or(label->wide);
    if (!strikes(numbers(), true, height, wide, text)) {
      break;
    }
  }
  Chain level = clear_size(space, true, height, wide);
  level.stroke = stroke;
  write(level, text, drawing);
  double across = label->wide.from + inset;
  Stretch tall = label->tall;
  for (;; across += step) {
    if (across >= label->at.x) {
      across = label->at.x;
      tall = label->tall;
      break;
    }
    tall = stretch_up(space, across, label->at.y).value_or(label->tall);
    if (!strikes(numbers(), false, across, tall, text)) {
      break;
    }
  }
  Chain upright = clear_size(space, false, across, tall);
  upright.stroke = stroke;
  write(upright, text, drawing);
}

}  // namespace otves::drafting
