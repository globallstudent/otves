#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include <otves/core/assert.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/wall.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

namespace otves::geom {
namespace {

// Inputs sit on a 1 mm grid, so distances below 1 µm are rounding noise.
constexpr double kEpsilon = 1e-6;

// A wall direction in whole millimetres. Directions are sorted and tested for parallelism on these
// exactly, so the joins never depend on how a platform rounds floating point.
struct StepMm {
  std::int64_t x = 0;
  std::int64_t y = 0;
};

constexpr StepMm step(PointMm from, PointMm to) {
  return {.x = (to.x - from.x).value(), .y = (to.y - from.y).value()};
}
constexpr StepMm reversed(StepMm step) { return {.x = -step.x, .y = -step.y}; }
// Coordinates stay within kMaxCoordinate, so these products fit in int64.
constexpr std::int64_t cross(StepMm lhs, StepMm rhs) { return (lhs.x * rhs.y) - (lhs.y * rhs.x); }
constexpr std::int64_t dot(StepMm lhs, StepMm rhs) { return (lhs.x * rhs.x) + (lhs.y * rhs.y); }
constexpr int sign(std::int64_t value) {
  return static_cast<int>(value > 0) - static_cast<int>(value < 0);
}

// a and b lie strictly on opposite sides of the wall's location line.
constexpr bool straddles(const Wall& wall, PointMm a, PointMm b) {
  const StepMm along = step(wall.start, wall.end);
  return sign(cross(along, step(wall.start, a))) * sign(cross(along, step(wall.start, b))) < 0;
}

// The location lines cross at one point inside both walls, away from every end.
constexpr bool cross_inside(const Wall& lhs, const Wall& rhs) {
  return straddles(lhs, rhs.start, rhs.end) && straddles(rhs, lhs.start, lhs.end);
}

// Directions in [0°, 180°).
constexpr bool upper_half(StepMm step) { return step.y > 0 || (step.y == 0 && step.x > 0); }

// Counter-clockwise order from +x. Equal directions are neither before nor after each other.
constexpr bool turns_before(StepMm lhs, StepMm rhs) {
  if (upper_half(lhs) != upper_half(rhs)) {
    return upper_half(lhs);
  }
  return cross(lhs, rhs) > 0;
}

struct Line {
  Vec2 point;
  Vec2 dir;
};

std::optional<Vec2> intersect(const Line& lhs, const Line& rhs) {
  const double denominator = cross(lhs.dir, rhs.dir);
  if (denominator == 0.0) {
    return std::nullopt;
  }
  return lhs.point + (lhs.dir * (cross(rhs.point - lhs.point, rhs.dir) / denominator));
}

// One wall in metres, with the distance from its location line to each face.
struct Frame {
  Vec2 start;
  Vec2 end;
  Vec2 dir;
  Vec2 left;
  StepMm step;
  double length = 0.0;
  double thickness = 0.0;
  double left_offset = 0.0;
  double right_offset = 0.0;
};

Frame make_frame(const Wall& wall) {
  Frame frame;
  frame.start = to_vec2(wall.start);
  frame.end = to_vec2(wall.end);
  frame.step = step(wall.start, wall.end);
  frame.length = length(frame.end - frame.start);
  frame.dir = (frame.end - frame.start) * (1.0 / frame.length);
  frame.left = perp(frame.dir);
  frame.thickness = to_metres(wall.thickness);
  switch (wall.location_line) {
    case LocationLine::kCenter:
      frame.left_offset = frame.thickness / 2;
      frame.right_offset = frame.thickness / 2;
      break;
    case LocationLine::kLeftFace:
      frame.right_offset = frame.thickness;
      break;
    case LocationLine::kRightFace:
      frame.left_offset = frame.thickness;
      break;
  }
  return frame;
}

struct End {
  std::size_t wall = 0;
  WallEnd which = WallEnd::kStart;
};

struct EndCorners {
  Vec2 left;
  Vec2 right;
  // Three or more walls share this point: the outline runs through it, so the walls tile the
  // joint like slices around a centre.
  bool through_point = false;
};

struct Corners {
  EndCorners start;
  EndCorners end;
};

std::vector<Vec2> drop_repeats(const std::vector<Vec2>& points) {
  std::vector<Vec2> kept;
  kept.reserve(points.size());
  for (const Vec2 point : points) {
    if (kept.empty() || length(point - kept.back()) > kEpsilon) {
      kept.push_back(point);
    }
  }
  while (kept.size() > 1 && length(kept.front() - kept.back()) <= kEpsilon) {
    kept.pop_back();
  }
  return kept;
}

// Each end is described from the point where it sits, looking along `out` into the wall: its
// counter-clockwise face is the left face at a wall's start and the right face at its end.
class Joiner {
 public:
  explicit Joiner(std::span<const WallInput> walls) {
    std::vector<WallInput> sorted(walls.begin(), walls.end());
    std::ranges::sort(sorted, {}, &WallInput::id);
    for (const WallInput& input : sorted) {
      const bool valid = validate(input.wall).has_value();
      const bool unique = walls_.empty() || walls_.back().id < input.id;
      OTVES_ASSERT(valid && unique);
      if (valid && unique) {
        walls_.push_back(input);
        frames_.push_back(make_frame(input.wall));
      }
    }
    corners_.resize(walls_.size());
    for (std::size_t i = 0; i < walls_.size(); ++i) {
      for (const WallEnd which : {WallEnd::kStart, WallEnd::kEnd}) {
        const End end{.wall = i, .which = which};
        ccw_corner(end) = ccw_face(end).point;
        cw_corner(end) = cw_face(end).point;
      }
    }
  }

  [[nodiscard]] WallJoins join() && {
    std::map<PointMm, std::vector<End>> points;
    for (std::size_t i = 0; i < walls_.size(); ++i) {
      points[walls_[i].wall.start].push_back({.wall = i, .which = WallEnd::kStart});
      points[walls_[i].wall.end].push_back({.wall = i, .which = WallEnd::kEnd});
    }
    for (auto& [at, ends] : points) {
      if (const std::optional<std::size_t> host = host_at(to_vec2(at))) {
        record(at, ends, host);
        butt_all(at, *host, ends);
      } else if (ends.size() > 1) {
        if (!in_line(ends)) {
          record(at, ends, std::nullopt);
        }
        join_at(at, std::move(ends));
      }
    }
    find_crossings();
    return std::move(*this).finish();
  }

 private:
  // Two ends along one line: a straight run, or an overlap that join_at reports.
  [[nodiscard]] bool in_line(std::span<const End> ends) const {
    return ends.size() == 2 && cross(out_step(ends[0]), out_step(ends[1])) == 0;
  }

  // An end running along its host overlaps it rather than meeting it, so it is left out.
  void record(PointMm at, std::span<const End> ends, std::optional<std::size_t> host) {
    Junction junction{.at = at, .walls = {}};
    for (const End end : ends) {
      if (!host || cross(frames_[*host].step, out_step(end)) != 0) {
        junction.walls.push_back(walls_[end.wall].id);
      }
    }
    if (host) {
      junction.walls.push_back(walls_[*host].id);
    }
    std::ranges::sort(junction.walls);
    const auto [first, last] = std::ranges::unique(junction.walls);
    junction.walls.erase(first, last);
    if (junction.walls.size() >= 2) {
      junctions_.push_back(std::move(junction));
    }
  }

  // Mitres between neighbours in counter-clockwise order around the shared point.
  void join_at(PointMm at, std::vector<End> ends) {
    std::ranges::sort(ends, [this](End lhs, End rhs) {
      if (turns_before(out_step(lhs), out_step(rhs))) {
        return true;
      }
      if (turns_before(out_step(rhs), out_step(lhs))) {
        return false;
      }
      return std::tie(lhs.wall, lhs.which) < std::tie(rhs.wall, rhs.which);
    });
    const Vec2 point = to_vec2(at);
    for (std::size_t k = 0; k < ends.size(); ++k) {
      const End first = ends[k];
      const End second = ends[(k + 1) % ends.size()];
      const StepMm first_out = out_step(first);
      const StepMm second_out = out_step(second);
      if (cross(first_out, second_out) == 0) {
        if (dot(first_out, second_out) > 0) {
          issues_.push_back({.at = at, .kind = JoinIssueKind::kOverlap});
        }
        continue;  // straight on: both ends stay square
      }
      const std::optional<Vec2> corner = intersect(ccw_face(first), cw_face(second));
      if (!corner || length(*corner - point) > mitre_limit(first.wall, second.wall)) {
        issues_.push_back({.at = at, .kind = JoinIssueKind::kSharpAngle});
        continue;
      }
      ccw_corner(first) = *corner;
      cw_corner(second) = *corner;
    }
    if (ends.size() >= 3) {
      for (const End end : ends) {
        corners(end).through_point = true;
      }
    }
  }

  // Ends inside another wall are cut back to its face; that wall stays whole. Two ends on one
  // side would overlap each other at the face.
  void butt_all(PointMm at, std::size_t host, std::span<const End> ends) {
    int left = 0;
    int right = 0;
    for (const End end : ends) {
      const std::int64_t side = cross(frames_[host].step, out_step(end));
      left += static_cast<int>(side > 0);
      right += static_cast<int>(side < 0);
    }
    if (left > 1 || right > 1) {
      issues_.push_back({.at = at, .kind = JoinIssueKind::kOverlap});
    }
    for (const End end : ends) {
      butt(at, end, host);
    }
  }

  void butt(PointMm at, End end, std::size_t host) {
    const Vec2 point = to_vec2(at);
    const Frame& frame = frames_[host];
    const std::int64_t side = cross(frame.step, out_step(end));
    if (side == 0) {
      issues_.push_back({.at = at, .kind = JoinIssueKind::kOverlap});
      return;
    }
    const Line face =
        side > 0 ? Line{.point = frame.start + (frame.left * frame.left_offset), .dir = frame.dir}
                 : Line{.point = frame.start - (frame.left * frame.right_offset), .dir = frame.dir};
    const std::optional<Vec2> ccw = intersect(ccw_face(end), face);
    const std::optional<Vec2> cw = intersect(cw_face(end), face);
    const double limit = mitre_limit(end.wall, host);
    if (!ccw || !cw || length(*ccw - point) > limit || length(*cw - point) > limit) {
      issues_.push_back({.at = at, .kind = JoinIssueKind::kSharpAngle});
      return;
    }
    const double stem_length = frames_[end.wall].length - kEpsilon;
    if (dot(*ccw - point, out_dir(end)) >= stem_length ||
        dot(*cw - point, out_dir(end)) >= stem_length) {
      issues_.push_back({.at = at, .kind = JoinIssueKind::kOverlap});
      return;
    }
    ccw_corner(end) = *ccw;
    cw_corner(end) = *cw;
  }

  // Walls crossing away from their ends overlap there; split at the crossing, they join as four
  // ends at one point. An end inside the other wall is a butt, not a cross.
  void find_crossings() {
    for (std::size_t i = 0; i < walls_.size(); ++i) {
      for (std::size_t j = i + 1; j < walls_.size(); ++j) {
        if (!cross_inside(walls_[i].wall, walls_[j].wall) || has_end_inside(i, j) ||
            has_end_inside(j, i)) {
          continue;
        }
        const std::optional<Vec2> at =
            intersect({.point = frames_[i].start, .dir = frames_[i].dir},
                      {.point = frames_[j].start, .dir = frames_[j].dir});
        if (at) {
          issues_.push_back({.at = {.x = round_to_mm(at->x), .y = round_to_mm(at->y)},
                             .kind = JoinIssueKind::kOverlap});
        }
      }
    }
  }

  // The first wall whose body holds the point away from both of its ends. The search is linear;
  // a spatial index can replace it when profiling asks for one.
  [[nodiscard]] std::optional<std::size_t> host_at(Vec2 point) const {
    for (std::size_t j = 0; j < frames_.size(); ++j) {
      if (inside(j, point)) {
        return j;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] bool inside(std::size_t wall, Vec2 point) const {
    const Frame& frame = frames_[wall];
    const double along = dot(point - frame.start, frame.dir);
    const double across = dot(point - frame.start, frame.left);
    return along > kEpsilon && along < frame.length - kEpsilon &&
           across <= frame.left_offset + kEpsilon && across >= -frame.right_offset - kEpsilon;
  }

  [[nodiscard]] bool has_end_inside(std::size_t wall, std::size_t host) const {
    return inside(host, frames_[wall].start) || inside(host, frames_[wall].end);
  }

  [[nodiscard]] WallJoins finish() && {
    WallJoins result;
    result.footprints.reserve(walls_.size());
    for (std::size_t i = 0; i < walls_.size(); ++i) {
      const Corners& c = corners_[i];
      std::vector<Vec2> outline{c.start.right, c.end.right};
      if (c.end.through_point) {
        outline.push_back(frames_[i].end);
      }
      outline.push_back(c.end.left);
      outline.push_back(c.start.left);
      if (c.start.through_point) {
        outline.push_back(frames_[i].start);
      }
      result.footprints.push_back({.id = walls_[i].id, .outline = drop_repeats(outline)});
    }
    result.junctions = std::move(junctions_);
    std::ranges::sort(issues_);
    const auto [first, last] = std::ranges::unique(issues_);
    issues_.erase(first, last);
    result.issues = std::move(issues_);
    return result;
  }

  [[nodiscard]] StepMm out_step(End end) const {
    const StepMm step = frames_[end.wall].step;
    return end.which == WallEnd::kStart ? step : reversed(step);
  }

  [[nodiscard]] Vec2 out_dir(End end) const {
    const Vec2 dir = frames_[end.wall].dir;
    return end.which == WallEnd::kStart ? dir : -dir;
  }

  [[nodiscard]] Vec2 point_of(End end) const {
    const Frame& frame = frames_[end.wall];
    return end.which == WallEnd::kStart ? frame.start : frame.end;
  }

  [[nodiscard]] Line ccw_face(End end) const {
    const Frame& frame = frames_[end.wall];
    const double offset = end.which == WallEnd::kStart ? frame.left_offset : frame.right_offset;
    return {.point = point_of(end) + (perp(out_dir(end)) * offset), .dir = out_dir(end)};
  }

  [[nodiscard]] Line cw_face(End end) const {
    const Frame& frame = frames_[end.wall];
    const double offset = end.which == WallEnd::kStart ? frame.right_offset : frame.left_offset;
    return {.point = point_of(end) - (perp(out_dir(end)) * offset), .dir = out_dir(end)};
  }

  [[nodiscard]] EndCorners& corners(End end) {
    Corners& wall = corners_[end.wall];
    return end.which == WallEnd::kStart ? wall.start : wall.end;
  }

  [[nodiscard]] Vec2& ccw_corner(End end) {
    return end.which == WallEnd::kStart ? corners(end).left : corners(end).right;
  }

  [[nodiscard]] Vec2& cw_corner(End end) {
    return end.which == WallEnd::kStart ? corners(end).right : corners(end).left;
  }

  [[nodiscard]] double mitre_limit(std::size_t lhs, std::size_t rhs) const {
    return kMaxMitre * std::max(frames_[lhs].thickness, frames_[rhs].thickness);
  }

  std::vector<WallInput> walls_;
  std::vector<Frame> frames_;
  std::vector<Corners> corners_;
  std::vector<Junction> junctions_;  // in point order, as the join loop visits them
  std::vector<JoinIssue> issues_;
};

}  // namespace

WallJoins join_walls(std::span<const WallInput> walls) { return Joiner{walls}.join(); }

}  // namespace otves::geom
