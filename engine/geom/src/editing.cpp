#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <expected>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/assert.h>
#include <otves/core/error.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/editing.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

#include "axis.h"
#include "faces.h"
#include "hold.h"
#include "step.h"

namespace otves::geom {
namespace {

constexpr WallEnd other(WallEnd end) {
  return end == WallEnd::kStart ? WallEnd::kEnd : WallEnd::kStart;
}

constexpr PointMm point_at(const Wall& wall, WallEnd end) {
  return end == WallEnd::kStart ? wall.start : wall.end;
}

// The nearest multiple of `grid`, a half going up; `value` itself when there is no grid.
constexpr Mm on_grid(Mm value, Mm grid) {
  if (grid <= Mm{0}) {
    return value;
  }
  const std::int64_t cell = grid.value();
  const std::int64_t shifted = value.value() + (cell / 2);
  const std::int64_t cells = shifted >= 0 ? shifted / cell : -((-shifted + cell - 1) / cell);
  return Mm{cells * cell};
}

// `from` moved `length` along `heading`, to the millimetre. Exact along an axis.
// Precondition: `heading` is not zero.
PointMm reached(PointMm from, StepMm heading, Mm length) {
  const double run = std::sqrt(static_cast<double>(dot(heading, heading)));
  const auto along = [&](std::int64_t part) {
    return Mm{std::llround(static_cast<double>(part) * static_cast<double>(length.value()) / run)};
  };
  return {.x = from.x + along(heading.x), .y = from.y + along(heading.y)};
}

struct End {
  std::size_t wall = 0;
  WallEnd which = WallEnd::kStart;
};

// The walls of one level read as a graph (docs/adr/0011): ends at one point form a joint, and a
// point in a wall's body, away from that wall's ends, stands against it.
class Plan {
 public:
  explicit Plan(std::span<const WallInput> walls) {
    std::vector<WallInput> sorted(walls.begin(), walls.end());
    std::ranges::sort(sorted, {}, &WallInput::id);
    for (const WallInput& input : sorted) {
      const bool valid = validate(input.wall).has_value();
      const bool unique = walls_.empty() || walls_.back().id < input.id;
      OTVES_ASSERT(valid && unique);
      if (!valid || !unique) {
        continue;
      }
      const std::size_t index = walls_.size();
      walls_.push_back(input);
      lines_.push_back(axis(input.wall));
      joints_[input.wall.start].push_back({.wall = index, .which = WallEnd::kStart});
      joints_[input.wall.end].push_back({.wall = index, .which = WallEnd::kEnd});
    }
  }

  [[nodiscard]] std::size_t size() const { return walls_.size(); }
  [[nodiscard]] const Wall& wall(std::size_t index) const { return walls_[index].wall; }
  [[nodiscard]] ElementId id(std::size_t index) const { return walls_[index].id; }

  [[nodiscard]] std::optional<std::size_t> find(ElementId id) const {
    const auto found = std::ranges::lower_bound(walls_, id, {}, &WallInput::id);
    if (found == walls_.end() || found->id != id) {
      return std::nullopt;
    }
    return static_cast<std::size_t>(found - walls_.begin());
  }

  // Every point where a wall ends, sorted.
  [[nodiscard]] std::vector<PointMm> joints() const {
    std::vector<PointMm> points;
    points.reserve(joints_.size());
    for (const auto& [point, ends] : joints_) {
      points.push_back(point);
    }
    return points;
  }

  [[nodiscard]] std::span<const End> ends_at(PointMm point) const {
    const auto found = joints_.find(point);
    return found == joints_.end() ? std::span<const End>{} : std::span<const End>{found->second};
  }

  // The walls a wall end at `point` would stand against.
  [[nodiscard]] std::vector<std::size_t> hosts_of(PointMm point) const {
    std::vector<std::size_t> hosts;
    for (std::size_t index = 0; index < walls_.size(); ++index) {
      if (holds(lines_[index], faces(walls_[index].wall), to_vec2(point))) {
        hosts.push_back(index);
      }
    }
    return hosts;
  }

  // The joints that stand against the wall.
  [[nodiscard]] std::vector<PointMm> standing_against(std::size_t index) const {
    std::vector<PointMm> points;
    for (const auto& [point, ends] : joints_) {
      if (holds(lines_[index], faces(walls_[index].wall), to_vec2(point))) {
        points.push_back(point);
      }
    }
    return points;
  }

  // Whether the wall's end is joined to another wall: at one point with its end, or against it.
  [[nodiscard]] bool joined(std::size_t index, WallEnd end) const {
    const PointMm point = point_at(walls_[index].wall, end);
    return ends_at(point).size() > 1 || !hosts_of(point).empty();
  }

  // Whether a wall ending at `point` joins the plan there.
  [[nodiscard]] bool touches(PointMm point) const {
    return joints_.contains(point) || !hosts_of(point).empty();
  }

 private:
  std::vector<WallInput> walls_;  // sorted by id
  std::vector<Axis> lines_;
  std::map<PointMm, std::vector<End>> joints_;
};

// The answer to an aim: the wall's length to `point`, the guides that still run through it, and
// whether it joins the plan there.
Aimed finished(const Plan& plan, std::optional<PointMm> from, PointMm point,
               std::vector<Guide> guides) {
  std::erase_if(guides, [point](const Guide& guide) {
    return guide.from == point || (guide.from.x != point.x && guide.from.y != point.y);
  });
  for (Guide& guide : guides) {
    guide.to = point;
  }
  std::ranges::sort(guides);
  const auto repeats = std::ranges::unique(guides);
  guides.erase(repeats.begin(), repeats.end());
  Mm length;
  if (from) {
    const StepMm run = step(*from, point);
    length = Mm{std::llround(std::sqrt(static_cast<double>(dot(run, run))))};
  }
  return {
      .point = point, .length = length, .guides = std::move(guides), .meets = plan.touches(point)};
}

// Which axis the wall being drawn is held to; a first point is free.
Held held(const Aim& request) {
  return request.from ? geom::held(*request.from, request.at, request.square) : Held::kFree;
}

// Settles one free coordinate: onto that of the nearest wall end within reach, which becomes a
// guide, or else onto the grid. A wall held to this axis runs from `own` and never lines up with
// it: it would have no length.
Mm settle(Mm value, Mm PointMm::* coordinate, std::span<const PointMm> ends, const Aim& request,
          std::optional<Mm> own, std::vector<Guide>& guides) {
  const auto gap = [&](PointMm end) { return magnitude((end.*coordinate - value).value()); };
  std::optional<PointMm> nearest;
  for (const PointMm end : ends) {
    if (end.*coordinate != own && (!nearest || gap(end) < gap(*nearest))) {
      nearest = end;
    }
  }
  if (!nearest || gap(*nearest) > request.reach.value()) {
    return on_grid(value, request.grid);
  }
  guides.push_back({.from = *nearest, .to = *nearest});
  return (*nearest).*coordinate;
}

// The point of the wall's line, strictly between its ends, where the wall being drawn would meet
// it: where its held line crosses, or for a free wall the point nearest the finger. Along a wall
// that runs with an axis a free wall keeps the coordinate it has settled, which is lined up or on
// the grid.
std::optional<PointMm> meeting(const Wall& wall, const Aim& aim, PointMm settled, Held hold) {
  const StepMm run = step(wall.start, wall.end);
  const auto between = [](double share) { return share > 0.0 && share < 1.0; };
  // How far along the wall, from 0 to 1, its line reaches `value` in one coordinate.
  const auto share = [](Mm value, Mm start, std::int64_t run_part) {
    return run_part == 0
               ? -1.0
               : static_cast<double>((value - start).value()) / static_cast<double>(run_part);
  };
  const auto along = [&](double part) {
    return PointMm{.x = wall.start.x + Mm{std::llround(part * static_cast<double>(run.x))},
                   .y = wall.start.y + Mm{std::llround(part * static_cast<double>(run.y))}};
  };
  const double at_y = share(settled.y, wall.start.y, run.y);
  const double at_x = share(settled.x, wall.start.x, run.x);
  switch (hold) {
    case Held::kAlongX:
      return between(at_y) ? std::optional{PointMm{.x = along(at_y).x, .y = settled.y}}
                           : std::nullopt;
    case Held::kAlongY:
      return between(at_x) ? std::optional{PointMm{.x = settled.x, .y = along(at_x).y}}
                           : std::nullopt;
    case Held::kFree:
      break;
  }
  if (run.x == 0 && between(at_y)) {
    return PointMm{.x = wall.start.x, .y = settled.y};
  }
  if (run.y == 0 && between(at_x)) {
    return PointMm{.x = settled.x, .y = wall.start.y};
  }
  const double nearest =
      static_cast<double>(dot(step(wall.start, aim.at), run)) / static_cast<double>(dot(run, run));
  return between(nearest) ? std::optional{along(nearest)} : std::nullopt;
}

// Carries a joint by a step, with everything that has to go with it for walls to keep their
// direction (docs/adr/0011).
class Carry {
 public:
  Carry(const Plan& plan, std::size_t resized, PointMm anchor, StepMm by)
      : plan_(&plan), resized_(resized), anchor_(anchor), by_(by) {}

  // Every end that moves when the joint at `joint` does, sorted by wall and end.
  [[nodiscard]] std::vector<EndMove> from(PointMm joint) && {
    take(joint);
    while (!queue_.empty()) {
      const PointMm point = queue_.front();
      queue_.pop_front();
      // The walls this point stands against come first: it must stay on them.
      for (const std::size_t host : plan_->hosts_of(point)) {
        follow(host);
      }
      for (const End end : plan_->ends_at(point)) {
        follow(end.wall);
      }
    }
    std::vector<EndMove> moves;
    for (std::size_t index = 0; index < plan_->size(); ++index) {
      for (const WallEnd end : {WallEnd::kStart, WallEnd::kEnd}) {
        const PointMm point = point_at(plan_->wall(index), end);
        if (moving_.contains(point)) {
          moves.push_back({.wall = plan_->id(index), .end = end, .to = moved(point, by_)});
        }
      }
    }
    return moves;
  }

 private:
  // A point moves once; the anchor never does.
  void take(PointMm point) {
    if (point != anchor_ && moving_.insert(point).second) {
      queue_.push_back(point);
    }
  }

  // A wall with a moving point on it keeps its direction. It stretches when it runs along the
  // move, as the resized wall does by definition. Otherwise it goes with the point whole, and
  // the wall ends standing against it go too; unless it reaches the anchor, about which it turns.
  void follow(std::size_t index) {
    const Wall& wall = plan_->wall(index);
    const bool stretches = index == resized_ || cross(step(wall.start, wall.end), by_) == 0;
    if (stretches || wall.start == anchor_ || wall.end == anchor_) {
      return;
    }
    take(wall.start);
    take(wall.end);
    for (const PointMm point : plan_->standing_against(index)) {
      take(point);
    }
  }

  const Plan* plan_;
  std::size_t resized_;
  PointMm anchor_;
  StepMm by_;
  std::set<PointMm> moving_;
  std::deque<PointMm> queue_;
};

// Where a wall's end is once the moves are made.
PointMm moved_end(const Plan& plan, std::span<const EndMove> moves, End end) {
  const auto move = std::ranges::find_if(moves, [&](const EndMove& candidate) {
    return candidate.wall == plan.id(end.wall) && candidate.end == end.which;
  });
  return move == moves.end() ? point_at(plan.wall(end.wall), end.which) : move->to;
}

// Checks the walls as the moves would leave them. None vanishes, turns back on itself or leaves
// the plan, and an end that stood against a wall still does, or has come to that wall's end:
// what is drawn joined stays joined.
Result<void> check(const Plan& plan, std::span<const EndMove> moves) {
  for (std::size_t index = 0; index < plan.size(); ++index) {
    const Wall& before = plan.wall(index);
    Wall after = before;
    after.start = moved_end(plan, moves, {.wall = index, .which = WallEnd::kStart});
    after.end = moved_end(plan, moves, {.wall = index, .which = WallEnd::kEnd});
    if (dot(step(before.start, before.end), step(after.start, after.end)) <= 0) {
      return make_error(ErrorCode::kBreaksJoin,
                        "a joined wall would vanish or turn back on itself");
    }
    if (auto valid = validate(after); !valid) {
      return std::unexpected{std::move(valid).error()};
    }
    const Axis line = axis(after);
    for (const PointMm joint : plan.standing_against(index)) {
      // Every end at a joint moves with it, so the first one says where the joint went.
      const PointMm now = moved_end(plan, moves, plan.ends_at(joint).front());
      const bool joined =
          now == after.start || now == after.end || holds(line, faces(after), to_vec2(now));
      if (!joined) {
        return make_error(ErrorCode::kBreaksJoin,
                          "a wall end would come off the wall it stands against");
      }
    }
  }
  return {};
}

}  // namespace

Aimed aim(std::span<const WallInput> walls, std::span<const PointMm> below, const Aim& aim) {
  const Plan plan{walls};
  std::vector<PointMm> ends = plan.joints();
  ends.insert(ends.end(), below.begin(), below.end());
  std::ranges::sort(ends);
  const auto repeats = std::ranges::unique(ends);
  ends.erase(repeats.begin(), repeats.end());
  if (aim.from) {
    std::erase(ends, *aim.from);
  }

  const auto gap = [&aim](PointMm point) { return apart(point, aim.at); };
  const auto nearest = std::ranges::min_element(ends, {}, gap);
  if (nearest != ends.end() && gap(*nearest) <= aim.reach.value()) {
    return finished(plan, aim.from, *nearest, {});
  }

  const Held hold = held(aim);
  const PointMm start = aim.from.value_or(aim.at);
  std::vector<Guide> guides;
  const auto own = [hold](Held along, Mm coordinate) {
    return hold == along ? std::optional{coordinate} : std::nullopt;
  };
  const PointMm settled{
      .x = hold == Held::kAlongY
               ? start.x
               : settle(aim.at.x, &PointMm::x, ends, aim, own(Held::kAlongX, start.x), guides),
      .y = hold == Held::kAlongX
               ? start.y
               : settle(aim.at.y, &PointMm::y, ends, aim, own(Held::kAlongY, start.y), guides)};

  std::optional<PointMm> met;
  for (std::size_t index = 0; index < plan.size(); ++index) {
    const std::optional<PointMm> point = meeting(plan.wall(index), aim, settled, hold);
    // A finger on the wall's body is on the wall: its faces lie about half its thickness from
    // its line.
    const std::int64_t reach = aim.reach.value() + (plan.wall(index).thickness.value() / 2);
    if (point && point != aim.from && gap(*point) <= reach && (!met || gap(*point) < gap(*met))) {
      met = point;
    }
  }
  return finished(plan, aim.from, met.value_or(settled), std::move(guides));
}

Aimed aim(std::span<const WallInput> walls, const Measured& measured) {
  if (measured.length <= Mm{0}) {
    return {.point = measured.from, .length = {}, .guides = {}, .meets = false};
  }
  StepMm heading{.x = 1, .y = 0};
  if (measured.before && *measured.before != measured.from) {
    heading = step(*measured.before, measured.from);
  }
  // The plan's y runs up, so a left turn is counter-clockwise.
  switch (measured.turn) {
    case Turn::kLeft:
      heading = {.x = -heading.y, .y = heading.x};
      break;
    case Turn::kRight:
      heading = {.x = heading.y, .y = -heading.x};
      break;
    case Turn::kStraight:
      break;
  }
  const Plan plan{walls};
  return finished(plan, measured.from, reached(measured.from, heading, measured.length), {});
}

Result<std::vector<EndMove>> resize_wall(std::span<const WallInput> walls, ElementId wall,
                                         Mm length, std::optional<WallEnd> fixed) {
  const Plan plan{walls};
  const std::optional<std::size_t> resized = plan.find(wall);
  if (!resized) {
    return make_error(ErrorCode::kNotFound, "wall not found");
  }
  if (length <= Mm{0}) {
    return make_error(ErrorCode::kZeroLength, "a wall's length must be positive");
  }
  const bool grows_from_end =
      !plan.joined(*resized, WallEnd::kStart) && plan.joined(*resized, WallEnd::kEnd);
  const WallEnd stays = fixed.value_or(grows_from_end ? WallEnd::kEnd : WallEnd::kStart);
  const PointMm anchor = point_at(plan.wall(*resized), stays);
  const PointMm joint = point_at(plan.wall(*resized), other(stays));
  const StepMm by = step(joint, reached(anchor, step(anchor, joint), length));
  if (by == StepMm{}) {
    return std::vector<EndMove>{};
  }
  std::vector<EndMove> moves = Carry{plan, *resized, anchor, by}.from(joint);
  if (auto fits = check(plan, moves); !fits) {
    return std::unexpected{std::move(fits).error()};
  }
  return moves;
}

}  // namespace otves::geom
