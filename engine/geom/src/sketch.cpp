#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/editing.h>
#include <otves/geom/sketch.h>
#include <otves/geom/wall_joins.h>

#include "hold.h"
#include "step.h"

namespace otves::geom {
namespace {

// A stroke of more points is read through every second, third or further one. A finger gives no
// more detail, and finding the turns takes time that grows with the square of the count.
constexpr std::size_t kMaxPoints = 4096;

constexpr bool on_plan(PointMm point) {
  const auto within = [](Mm value) { return value >= -kMaxCoordinate && value <= kMaxCoordinate; };
  return within(point.x) && within(point.y);
}

// The stroke without points that repeat, thinned to about kMaxPoints; its last point stays.
std::vector<PointMm> sampled(std::span<const PointMm> points) {
  const std::size_t stride =
      std::max<std::size_t>(1, (points.size() + kMaxPoints - 1) / kMaxPoints);
  std::vector<PointMm> stroke;
  for (std::size_t index = 0; index < points.size(); ++index) {
    const bool wanted = index % stride == 0 || index + 1 == points.size();
    if (wanted && (stroke.empty() || stroke.back() != points[index])) {
      stroke.push_back(points[index]);
    }
  }
  return stroke;
}

// How far `point` lies from the stretch between `from` and `to`, in millimetres.
double distance(PointMm point, PointMm from, PointMm to) {
  const StepMm chord = step(from, to);
  const StepMm out = step(from, point);
  const std::int64_t along = dot(out, chord);
  const std::int64_t whole = dot(chord, chord);
  const auto length = [](StepMm run) { return std::sqrt(static_cast<double>(dot(run, run))); };
  if (along <= 0) {
    return length(out);
  }
  if (along >= whole) {
    return length(step(to, point));
  }
  return static_cast<double>(magnitude(cross(chord, out))) / std::sqrt(static_cast<double>(whole));
}

// The points where the stroke turns, as indices: its ends, and between two turns the point
// furthest from the line between them while that is further than `reach` (Ramer, Douglas and
// Peucker). Precondition: the stroke has two points or more.
std::vector<std::size_t> turns(std::span<const PointMm> stroke, Mm reach) {
  std::vector<bool> kept(stroke.size(), false);
  kept.front() = true;
  kept.back() = true;
  std::vector<std::pair<std::size_t, std::size_t>> open{{0, stroke.size() - 1}};
  while (!open.empty()) {
    const auto [first, last] = open.back();
    open.pop_back();
    std::size_t furthest = first;
    auto most = static_cast<double>(reach.value());
    for (std::size_t index = first + 1; index < last; ++index) {
      const double off = distance(stroke[index], stroke[first], stroke[last]);
      if (off > most) {
        most = off;
        furthest = index;
      }
    }
    if (furthest != first) {
      kept[furthest] = true;
      open.emplace_back(first, furthest);
      open.emplace_back(furthest, last);
    }
  }
  std::vector<std::size_t> at;
  for (std::size_t index = 0; index < stroke.size(); ++index) {
    if (kept[index]) {
      at.push_back(index);
    }
  }
  return at;
}

// A leg of the stroke: the corners it runs between, its points, and the axis it is held to.
struct Run {
  std::size_t from = 0;
  std::size_t to = 0;
  std::span<const PointMm> points;
  Held along = Held::kFree;
};

// The stroke as corners and the legs between them. A loop's last leg runs to its first corner.
struct Outline {
  std::vector<PointMm> corners;
  std::vector<Run> runs;
  bool loop = false;
};

Outline outline(std::span<const PointMm> stroke, Mm reach, bool square) {
  std::vector<std::size_t> at = turns(stroke, reach);
  // The first and the last point are each a finger's, so they are one point within two reaches.
  // A stroke that runs past its first corner turns there again; the turn nearest that corner
  // closes the loop and what follows is dropped.
  const std::int64_t near = 2 * reach.value();
  const auto home = [&](std::size_t turn) { return apart(stroke[at[turn]], stroke[0]) <= near; };
  Outline read;
  read.loop = at.size() >= 4 && home(at.size() - 1);
  while (read.loop && at.size() > 4 && home(at.size() - 2)) {
    at.pop_back();
  }
  const std::size_t corners = read.loop ? at.size() - 1 : at.size();
  for (std::size_t turn = 0; turn < corners; ++turn) {
    read.corners.push_back(stroke[at[turn]]);
  }
  for (std::size_t turn = 0; turn + 1 < at.size(); ++turn) {
    read.runs.push_back({.from = turn,
                         .to = turn + 1 == corners ? 0 : turn + 1,
                         .points = stroke.subspan(at[turn], at[turn + 1] - at[turn] + 1),
                         .along = held(stroke[at[turn]], stroke[at[turn + 1]], square)});
  }
  return read;
}

// Where the corners of a stroke are taken to be. Both ends of a leg held to an axis lie on one
// line across it, so corners share lines: columns with one x and rows with one y.
class Corners {
 public:
  explicit Corners(const Outline& read)
      : spots_(read.corners),
        pinned_(spots_.size(), false),
        columns_(spots_.size(), &PointMm::x),
        rows_(spots_.size(), &PointMm::y) {
    for (const Run& run : read.runs) {
      if (run.along == Held::kAlongX) {
        rows_.join(run.from, run.to);
      } else if (run.along == Held::kAlongY) {
        columns_.join(run.from, run.to);
      }
    }
    level(columns_, Held::kAlongY, read.runs);
    level(rows_, Held::kAlongX, read.runs);
  }

  [[nodiscard]] std::span<const PointMm> spots() const { return spots_; }
  [[nodiscard]] PointMm spot(std::size_t corner) const { return spots_[corner]; }

  // The corner is a wall end of the plan. It stays there, whatever its lines do.
  void pin(std::size_t corner, PointMm at) {
    pinned_[corner] = true;
    spots_[corner] = at;
    place(corner, at);
  }

  // The first corner placed on a line says where the line is: the free corners on it follow.
  void place(std::size_t corner, PointMm at) {
    settle(columns_, corner, at.x);
    settle(rows_, corner, at.y);
  }

 private:
  struct Lines {
    Lines(std::size_t corners, Mm PointMm::* across)
        : coordinate(across), of(corners), settled(corners, false) {
      for (std::size_t corner = 0; corner < corners; ++corner) {
        of[corner] = corner;
      }
    }

    void join(std::size_t a, std::size_t b) {
      const std::size_t from = of[b];
      const std::size_t to = of[a];
      std::ranges::replace(of, from, to);
    }

    Mm PointMm::* coordinate;
    std::vector<std::size_t> of;  // the line of each corner
    std::vector<bool> settled;    // by line
  };

  void put(const Lines& lines, std::size_t line, Mm value) {
    for (std::size_t corner = 0; corner < spots_.size(); ++corner) {
      if (lines.of[corner] == line && !pinned_[corner]) {
        spots_[corner].*lines.coordinate = value;
      }
    }
  }

  void settle(Lines& lines, std::size_t corner, Mm value) {
    const std::size_t line = lines.of[corner];
    if (!lines.settled[line]) {
      lines.settled[line] = true;
      put(lines, line, value);
    }
  }

  // Puts each line where the stroke ran along it: at the middle value of the legs' points, which
  // the rounded corners at their ends do not move.
  void level(const Lines& lines, Held along, std::span<const Run> runs) {
    std::map<std::size_t, std::vector<Mm>> drawn;
    for (const Run& run : runs) {
      if (run.along != along) {
        continue;
      }
      std::vector<Mm>& values = drawn[lines.of[run.from]];
      for (const PointMm point : run.points) {
        values.push_back(point.*lines.coordinate);
      }
    }
    for (auto& [line, values] : drawn) {
      const auto middle = values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2);
      std::ranges::nth_element(values, middle);
      put(lines, line, *middle);
    }
  }

  std::vector<PointMm> spots_;
  std::vector<bool> pinned_;
  Lines columns_;
  Lines rows_;
};

// Every wall end a corner can be caught by, sorted.
std::vector<PointMm> wall_ends(std::span<const WallInput> walls, std::span<const PointMm> below) {
  std::vector<PointMm> ends(below.begin(), below.end());
  for (const WallInput& input : walls) {
    ends.push_back(input.wall.start);
    ends.push_back(input.wall.end);
  }
  std::ranges::sort(ends);
  return ends;
}

// Ids for the legs while they are read as walls: none that a wall of the plan has.
class Spare {
 public:
  explicit Spare(std::span<const WallInput> walls) {
    taken_.reserve(walls.size());
    for (const WallInput& input : walls) {
      taken_.push_back(input.id);
    }
    std::ranges::sort(taken_);
  }

  [[nodiscard]] ElementId next() {
    ElementId id;
    do {
      id = ElementId{Uuid{0, ++count_}};
    } while (std::ranges::binary_search(taken_, id));
    return id;
  }

 private:
  std::vector<ElementId> taken_;
  std::uint64_t count_ = 0;
};

// Whether a wall from the leg's end to `to` runs on in the leg's line.
constexpr bool runs_on(const Leg& leg, PointMm to) {
  const StepMm before = step(leg.start, leg.end);
  const StepMm after = step(leg.end, to);
  return cross(before, after) == 0 && dot(before, after) > 0;
}

// The legs as they are laid one after another, each read as a wall by the next.
class Laid {
 public:
  Laid(std::span<const WallInput> walls, const Wall& like)
      : plan_(walls.begin(), walls.end()), spare_(walls), like_(like) {}

  [[nodiscard]] std::span<const WallInput> plan() const { return plan_; }

  // A wall from `from` to `to`; one that runs on in the line of the wall before it lengthens
  // that wall instead.
  [[nodiscard]] Result<void> add(PointMm from, PointMm to) {
    Wall wall = like_;
    wall.start = from;
    wall.end = to;
    if (auto valid = validate(wall); !valid) {
      return std::unexpected{std::move(valid).error()};
    }
    if (!legs_.empty() && runs_on(legs_.back(), to)) {
      legs_.back().end = to;
      plan_.back().wall.end = to;
    } else {
      legs_.push_back({.start = from, .end = to});
      plan_.push_back({.id = spare_.next(), .wall = wall});
    }
    return {};
  }

  // A loop begun in the middle of a wall ends in the middle of it: its last leg runs on into its
  // first.
  [[nodiscard]] std::vector<Leg> take() && {
    if (legs_.size() >= 3 && legs_.back().end == legs_.front().start &&
        runs_on(legs_.back(), legs_.front().end)) {
      legs_.front().start = legs_.back().start;
      legs_.pop_back();
    }
    return std::move(legs_);
  }

 private:
  std::vector<WallInput> plan_;
  Spare spare_;
  Wall like_;
  std::vector<Leg> legs_;
};

}  // namespace

Result<std::vector<Leg>> straighten(std::span<const WallInput> walls,
                                    std::span<const PointMm> below, const Sketch& sketch,
                                    const Wall& like) {
  if (!std::ranges::all_of(sketch.points, on_plan)) {
    return make_error(ErrorCode::kOffPlan, "sketch point lies more than 100 km from the origin");
  }
  const std::vector<PointMm> stroke = sampled(sketch.points);
  if (stroke.size() < 2) {
    return make_error(ErrorCode::kZeroLength, "the stroke gives no wall");
  }
  Aim touch{.from = std::nullopt,
            .at = {},
            .reach = std::clamp(sketch.reach, Mm{0}, kMaxCoordinate),
            .grid = std::clamp(sketch.grid, Mm{0}, kMaxCoordinate),
            .square = sketch.square};
  // A stroke that never leaves the reach of its first point is a tap.
  const auto at_start = [&](PointMm point) {
    return apart(point, stroke.front()) <= touch.reach.value();
  };
  if (std::ranges::all_of(stroke, at_start)) {
    return make_error(ErrorCode::kZeroLength, "the stroke gives no wall");
  }
  const Outline read = outline(stroke, touch.reach, touch.square);
  Corners corners{read};

  // Wall ends of the plan come first: the lines through a caught corner are where that end is.
  const std::vector<PointMm> ends = wall_ends(walls, below);
  const std::vector<PointMm> drawn(corners.spots().begin(), corners.spots().end());
  for (std::size_t corner = 0; corner < drawn.size(); ++corner) {
    touch.at = drawn[corner];
    const PointMm caught = aim(walls, below, touch).point;
    if (std::ranges::binary_search(ends, caught)) {
      corners.pin(corner, caught);
    }
  }

  Laid laid{walls, like};
  touch.at = corners.spot(0);
  const PointMm first = aim(laid.plan(), below, touch).point;
  corners.place(0, first);
  PointMm from = first;
  for (const Run& run : read.runs) {
    const bool home = read.loop && &run == &read.runs.back();
    touch.from = from;
    touch.at = home ? first : corners.spot(run.to);
    const PointMm to = aim(laid.plan(), below, touch).point;
    if (!home) {
      corners.place(run.to, to);
    }
    if (to == from) {
      continue;
    }
    if (auto added = laid.add(from, to); !added) {
      return std::unexpected{std::move(added).error()};
    }
    from = to;
  }

  std::vector<Leg> legs = std::move(laid).take();
  if (legs.empty()) {
    return make_error(ErrorCode::kZeroLength, "the stroke gives no wall");
  }
  return legs;
}

}  // namespace otves::geom
