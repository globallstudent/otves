#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include <otves/core/assert.h>
#include <otves/core/point.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/polygon.h>
#include <otves/geom/spaces.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

#include "axis.h"
#include "frame.h"
#include "step.h"

namespace otves::geom {
namespace {

// One way along a piece of a wall between two points of the graph. The space it bounds lies on
// its left.
struct HalfEdge {
  std::size_t wall = 0;
  bool forward = true;  // from the wall's start towards its end
  std::size_t from = 0;
  std::size_t to = 0;
  std::size_t twin = 0;  // the same piece the other way
  std::size_t slot = 0;  // its place among the half-edges leaving `from`
};

// A point where wall ends sit: a joint of ends, or a T where they stand against a wall.
struct Node {
  PointMm at;
  std::vector<std::size_t> out;  // half-edges leaving, counter-clockwise from +x
};

// A corner of a face, and the wall whose face the outline runs along after it.
struct Corner {
  Vec2 at;
  std::size_t wall = 0;
};

// The corners without those that repeat the one before them, the last against the first too. The
// side after a repeated corner has no length, so the corner that stays leads to the later wall.
std::vector<Corner> drop_repeats(const std::vector<Corner>& corners) {
  std::vector<Corner> kept;
  kept.reserve(corners.size());
  for (const Corner& corner : corners) {
    if (!kept.empty() && length(corner.at - kept.back().at) <= kEpsilon) {
      kept.back().wall = corner.wall;
    } else {
      kept.push_back(corner);
    }
  }
  while (kept.size() > 1 && length(kept.front().at - kept.back().at) <= kEpsilon) {
    kept.pop_back();
  }
  return kept;
}

// A point of a wall's body where the ends of other walls stand against it.
struct Stop {
  double along = 0.0;
  std::size_t node = 0;
};

double perimeter_of(std::span<const Vec2> outline) {
  double total = 0.0;
  for (std::size_t i = 0; i < outline.size(); ++i) {
    total += length(outline[(i + 1) % outline.size()] - outline[i]);
  }
  return total;
}

// The walls of a level as a planar graph, by the definition join_walls joins them by, and the
// faces of that graph as the faces of the walls round them.
class Graph {
 public:
  explicit Graph(std::span<const WallInput> walls) {
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
    add_nodes();
    add_edges();
    for (std::size_t node = 0; node < nodes_.size(); ++node) {
      sort_round(node);
    }
  }

  [[nodiscard]] std::vector<Space> spaces() const {
    std::vector<Space> found;
    std::vector<std::size_t> found_in;  // the component each space's walls belong to
    std::vector<std::pair<std::vector<Vec2>, std::size_t>> outsides;
    const std::vector<std::size_t> component = components();
    std::vector<bool> walked(edges_.size(), false);
    for (std::size_t first = 0; first < edges_.size(); ++first) {
      if (walked[first]) {
        continue;
      }
      std::vector<std::size_t> face;
      for (std::size_t edge = first; !walked[edge]; edge = next(edge)) {
        walked[edge] = true;
        face.push_back(edge);
      }
      const std::vector<Corner> corners = corners_of(face);
      std::vector<Vec2> outline;
      outline.reserve(corners.size());
      for (const Corner& corner : corners) {
        outline.push_back(corner.at);
      }
      const std::size_t group = component[edges_[first].from];
      if (signed_area(outline) > 0.0 && turns_left(face)) {
        Space space{.outline = std::move(outline), .sides = {}, .islands = {}, .area = 0.0};
        space.sides.reserve(corners.size());
        for (const Corner& corner : corners) {
          space.sides.push_back(walls_[corner.wall].id);
        }
        found.push_back(std::move(space));
        found_in.push_back(group);
      } else if (!turns_left(face)) {
        outsides.emplace_back(std::move(outline), group);
      }
    }
    for (auto& [outline, group] : outsides) {
      if (const std::optional<std::size_t> around = space_round(found, found_in, outline, group)) {
        found[*around].islands.push_back(std::move(outline));
      }
    }
    for (Space& space : found) {
      space.area = signed_area(space.outline);
      space.perimeter = perimeter_of(space.outline);
      for (const std::vector<Vec2>& island : space.islands) {
        space.area += signed_area(island);
        space.perimeter += perimeter_of(island);
      }
    }
    return found;
  }

 private:
  void add_nodes() {
    std::map<PointMm, std::size_t> order;
    for (const WallInput& input : walls_) {
      order.emplace(input.wall.start, 0);
      order.emplace(input.wall.end, 0);
    }
    nodes_.reserve(order.size());
    for (auto& [at, index] : order) {
      index = nodes_.size();
      nodes_.push_back({.at = at, .out = {}});
    }
    starts_.reserve(walls_.size());
    ends_.reserve(walls_.size());
    for (const WallInput& input : walls_) {
      starts_.push_back(order.at(input.wall.start));
      ends_.push_back(order.at(input.wall.end));
    }
  }

  // Each wall runs from node to node: its ends, and between them the points where other walls
  // stand against it.
  void add_edges() {
    std::vector<std::vector<Stop>> stops(walls_.size());
    for (std::size_t node = 0; node < nodes_.size(); ++node) {
      const Vec2 point = to_vec2(nodes_[node].at);
      if (const std::optional<std::size_t> host = host_at(point)) {
        const Frame& frame = frames_[*host];
        stops[*host].push_back({.along = dot(point - frame.start, frame.dir), .node = node});
      }
    }
    for (std::size_t wall = 0; wall < walls_.size(); ++wall) {
      std::ranges::sort(stops[wall], [](const Stop& lhs, const Stop& rhs) {
        return std::tie(lhs.along, lhs.node) < std::tie(rhs.along, rhs.node);
      });
      std::size_t from = starts_[wall];
      for (const Stop& stop : stops[wall]) {
        add_piece(wall, from, stop.node);
        from = stop.node;
      }
      add_piece(wall, from, ends_[wall]);
    }
  }

  void add_piece(std::size_t wall, std::size_t from, std::size_t to) {
    const std::size_t there = edges_.size();
    const std::size_t back = there + 1;
    edges_.push_back({.wall = wall, .forward = true, .from = from, .to = to, .twin = back});
    edges_.push_back({.wall = wall, .forward = false, .from = to, .to = from, .twin = there});
    nodes_[from].out.push_back(there);
    nodes_[to].out.push_back(back);
  }

  // The first wall whose body holds the point away from both of its ends, as join_walls reads it.
  [[nodiscard]] std::optional<std::size_t> host_at(Vec2 point) const {
    for (std::size_t wall = 0; wall < frames_.size(); ++wall) {
      if (holds(frames_[wall], point)) {
        return wall;
      }
    }
    return std::nullopt;
  }

  void sort_round(std::size_t node) {
    std::vector<std::size_t>& out = nodes_[node].out;
    std::ranges::sort(out, [this](std::size_t lhs, std::size_t rhs) {
      if (turns_before(heading(lhs), heading(rhs))) {
        return true;
      }
      if (turns_before(heading(rhs), heading(lhs))) {
        return false;
      }
      return lhs < rhs;
    });
    for (std::size_t slot = 0; slot < out.size(); ++slot) {
      edges_[out[slot]].slot = slot;
    }
  }

  // The half-edge that goes on round the same face: at the point this one reaches, the first
  // one clockwise from the way back.
  [[nodiscard]] std::size_t next(std::size_t edge) const {
    const HalfEdge& back = edges_[edges_[edge].twin];
    const std::vector<std::size_t>& out = nodes_[back.from].out;
    return out[(back.slot + out.size() - 1) % out.size()];
  }

  [[nodiscard]] StepMm heading(std::size_t edge) const {
    const StepMm along = frames_[edges_[edge].wall].step;
    return edges_[edge].forward ? along : reversed(along);
  }

  // The face of the wall on the half-edge's left, as a line running its way.
  [[nodiscard]] Line side(std::size_t edge) const {
    const Frame& frame = frames_[edges_[edge].wall];
    if (edges_[edge].forward) {
      return {.point = frame.start + (frame.left * frame.left_offset), .dir = frame.dir};
    }
    return {.point = frame.start - (frame.left * frame.right_offset), .dir = -frame.dir};
  }

  // Whether the face runs counter-clockwise round the points it visits: a bounded face does,
  // the outside of a group of walls does not. Decided on whole millimetres; a plan is far too
  // small for the products to lose a bit in a double.
  [[nodiscard]] bool turns_left(std::span<const std::size_t> face) const {
    const PointMm origin = nodes_[edges_[face.front()].from].at;
    double twice = 0.0;
    for (const std::size_t edge : face) {
      const StepMm from = step(origin, nodes_[edges_[edge].from].at);
      const StepMm to = step(origin, nodes_[edges_[edge].to].at);
      twice += static_cast<double>(cross(from, to));
    }
    return twice > 0.0;
  }

  // The face as the plan shows it: each corner is where the wall faces of two half-edges cross.
  [[nodiscard]] std::vector<Corner> corners_of(std::span<const std::size_t> face) const {
    std::vector<Corner> corners;
    for (std::size_t i = 0; i < face.size(); ++i) {
      add_corner(corners, face[i], face[(i + 1) % face.size()]);
    }
    return drop_repeats(corners);
  }

  void add_corner(std::vector<Corner>& corners, std::size_t before, std::size_t after) const {
    const HalfEdge& in = edges_[before];
    const HalfEdge& out = edges_[after];
    if (in.wall == out.wall && in.forward == out.forward) {
      return;  // the wall runs on through a point where others stand against it
    }
    const Vec2 point = to_vec2(nodes_[in.to].at);
    const Line first = side(before);
    const Line second = side(after);
    if (cross(heading(before), heading(after)) != 0) {
      const double limit =
          kMaxMitre * std::max(frames_[in.wall].thickness, frames_[out.wall].thickness);
      const std::optional<Vec2> corner = intersect(first, second);
      if (corner && length(*corner - point) <= limit) {
        corners.push_back({.at = *corner, .wall = out.wall});
        return;
      }
    }
    // Square ends: a wall's free end, walls running on in one line, or a mitre too sharp to cut.
    // The end itself belongs to the wall that reaches it.
    corners.push_back({.at = foot(first, point), .wall = in.wall});
    corners.push_back({.at = foot(second, point), .wall = out.wall});
  }

  [[nodiscard]] static Vec2 foot(const Line& line, Vec2 point) {
    return line.point + (line.dir * dot(point - line.point, line.dir));
  }

  // Which group of joined walls each node belongs to.
  [[nodiscard]] std::vector<std::size_t> components() const {
    std::vector<std::size_t> group(nodes_.size(), nodes_.size());
    std::vector<std::size_t> pending;
    for (std::size_t start = 0; start < nodes_.size(); ++start) {
      if (group[start] != nodes_.size()) {
        continue;
      }
      group[start] = start;
      pending.push_back(start);
      while (!pending.empty()) {
        const std::size_t node = pending.back();
        pending.pop_back();
        for (const std::size_t edge : nodes_[node].out) {
          const std::size_t reached = edges_[edge].to;
          if (group[reached] == nodes_.size()) {
            group[reached] = start;
            pending.push_back(reached);
          }
        }
      }
    }
    return group;
  }

  // The smallest space of other walls that holds the outside of this group of walls.
  [[nodiscard]] static std::optional<std::size_t> space_round(std::span<const Space> found,
                                                              std::span<const std::size_t> found_in,
                                                              std::span<const Vec2> outline,
                                                              std::size_t group) {
    std::optional<std::size_t> around;
    double smallest = 0.0;
    for (std::size_t i = 0; i < found.size(); ++i) {
      if (outline.empty() || found_in[i] == group || !contains(found[i].outline, outline.front())) {
        continue;
      }
      const double area = signed_area(found[i].outline);
      if (!around || area < smallest) {
        around = i;
        smallest = area;
      }
    }
    return around;
  }

  std::vector<WallInput> walls_;
  std::vector<Frame> frames_;
  std::vector<std::size_t> starts_;  // the node at each wall's start
  std::vector<std::size_t> ends_;
  std::vector<Node> nodes_;  // in point order
  std::vector<HalfEdge> edges_;
};

}  // namespace

std::vector<Space> find_spaces(std::span<const WallInput> walls) { return Graph{walls}.spaces(); }

bool holds(const Space& space, Vec2 point) {
  return contains(space.outline, point) &&
         std::ranges::none_of(space.islands, [point](const std::vector<Vec2>& island) {
           return contains(island, point);
         });
}

Names name_spaces(std::span<const Space> spaces, std::span<const Anchor> anchors) {
  std::vector<Anchor> oldest_first(anchors.begin(), anchors.end());
  std::ranges::sort(oldest_first, {}, &Anchor::room);
  Names names{.rooms = std::vector<std::optional<ElementId>>(spaces.size()), .issues = {}};
  for (const Anchor& anchor : oldest_first) {
    const Vec2 point = to_vec2(anchor.at);
    const auto space = std::ranges::find_if(
        spaces, [point](const Space& candidate) { return holds(candidate, point); });
    if (space == spaces.end()) {
      names.issues.push_back({.room = anchor.room, .kind = AnchorIssueKind::kNoSpace});
      continue;
    }
    std::optional<ElementId>& named = names.rooms[static_cast<std::size_t>(space - spaces.begin())];
    if (named) {
      names.issues.push_back({.room = anchor.room, .kind = AnchorIssueKind::kSharedSpace});
    } else {
      named = anchor.room;
    }
  }
  std::ranges::sort(names.issues);
  return names;
}

}  // namespace otves::geom
