#include "placement.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/build.h>
#include <otves/core/command.h>
#include <otves/core/error.h>
#include <otves/core/level.h>
#include <otves/core/model.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/core/wall_type.h>
#include <otves/drafting/plan.h>
#include <otves/geom/editing.h>
#include <otves/geom/openings.h>
#include <otves/geom/sketch.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/layers.h>

#include "built.h"
#include "convert.h"

namespace otves::bridge {
namespace {

// The location line's length rounded down to the millimetre, so an opening that fits it never
// reaches past an end. Coordinates stay within kMaxCoordinate, so the squares fit in int64.
Mm axis_length(const Wall& wall) {
  const std::int64_t dx = (wall.end.x - wall.start.x).value();
  const std::int64_t dy = (wall.end.y - wall.start.y).value();
  const std::int64_t squared = (dx * dx) + (dy * dy);
  auto length = static_cast<std::int64_t>(std::sqrt(static_cast<double>(squared)));
  while (length * length > squared) {
    --length;
  }
  while ((length + 1) * (length + 1) <= squared) {
    ++length;
  }
  return Mm{length};
}

std::vector<geom::WallInput> walls_on(const Model& model, ElementId level) {
  return inputs(built_walls(model, level));
}

// The level under `level`: the one before it by elevation. Levels at one elevation keep the order
// of their ids, which grow with time.
std::optional<ElementId> level_below(const Model& model, ElementId level) {
  std::vector<std::pair<Mm, ElementId>> order;
  model.for_each<Level>(
      [&order](ElementId id, const Level& value) { order.emplace_back(value.elevation, id); });
  std::ranges::sort(order);
  const auto found = std::ranges::find(order, level, &std::pair<Mm, ElementId>::second);
  if (found == order.end() || found == order.begin()) {
    return std::nullopt;
  }
  return std::prev(found)->second;
}

// The openings in `walls`, each with its wall as geometry reads it. Precondition: `walls` is
// sorted by id.
std::vector<geom::PlacedOpening> openings_in(const Model& model,
                                             std::span<const geom::WallInput> walls) {
  std::vector<geom::PlacedOpening> openings;
  model.for_each<Opening>([&](ElementId id, const Opening& opening) {
    const auto wall = std::ranges::lower_bound(walls, opening.wall, {}, &geom::WallInput::id);
    if (wall != walls.end() && wall->id == opening.wall) {
      openings.push_back({.id = id, .wall = wall->wall, .opening = opening});
    }
  });
  return openings;
}

// The walls of `level` sorted by id, to look openings up in.
std::vector<geom::WallInput> sorted_walls_on(const Model& model, ElementId level) {
  std::vector<geom::WallInput> walls = walls_on(model, level);
  std::ranges::sort(walls, {}, &geom::WallInput::id);
  return walls;
}

// The wall ends of the level under `level`, so walls stand on walls.
std::vector<PointMm> ends_below(const Model& model, ElementId level) {
  std::vector<PointMm> below;
  if (const std::optional<ElementId> under = level_below(model, level)) {
    for (const geom::WallInput& input : walls_on(model, *under)) {
      below.push_back(input.wall.start);
      below.push_back(input.wall.end);
    }
  }
  return below;
}

}  // namespace

Result<geom::Aimed> aim(const Model& model, const Sight& sight) {
  if (model.find<Level>(sight.level) == nullptr) {
    return make_error(ErrorCode::kNotFound, "level not found");
  }
  return geom::aim(walls_on(model, sight.level), ends_below(model, sight.level), sight.aim);
}

Result<geom::Aimed> aim(const Model& model, const MeasuredSight& sight) {
  if (model.find<Level>(sight.level) == nullptr) {
    return make_error(ErrorCode::kNotFound, "level not found");
  }
  return geom::aim(walls_on(model, sight.level), sight.measured);
}

Result<std::vector<Command>> resize(const Model& model, const Resize& resize) {
  const Wall* wall = model.find<Wall>(resize.wall);
  if (wall == nullptr) {
    return make_error(ErrorCode::kNotFound, "wall not found");
  }
  return geom::resize_wall(walls_on(model, wall->level), resize.wall, resize.length, resize.fixed)
      .transform([&model, &resize, wall](const std::vector<geom::EndMove>& moves) {
        std::vector<MoveWallEnd> ordered;
        ordered.reserve(moves.size());
        for (const geom::EndMove& move : moves) {
          ordered.push_back({.wall = move.wall, .end = move.end, .to = move.to});
          // One end moves at a time. A wall sliding along its own line by its own length would
          // put its start on its end, so its end goes first.
          const Wall* moved = model.find<Wall>(move.wall);
          const std::size_t count = ordered.size();
          if (move.end == WallEnd::kEnd && count > 1 && ordered[count - 2].wall == move.wall &&
              moved != nullptr && ordered[count - 2].to == moved->end) {
            std::swap(ordered[count - 2], ordered[count - 1]);
          }
        }
        std::vector<Command> commands(ordered.begin(), ordered.end());
        // A typed length is a measured one, also when it is the length the wall already has.
        if (!wall->exact) {
          commands.emplace_back(SetWallExact{.wall = resize.wall, .exact = true});
        }
        return commands;
      });
}

Result<std::vector<Command>> sketch(const Model& model, const Stroke& stroke) {
  const ElementId level = stroke.wall.level;
  if (model.find<Level>(level) == nullptr) {
    return make_error(ErrorCode::kNotFound, "level not found");
  }
  if (!stroke.wall.type.is_nil() && model.find<WallType>(stroke.wall.type) == nullptr) {
    return make_error(ErrorCode::kNotFound, "wall type not found");
  }
  const geom::Sketch drawn{
      .points = stroke.points, .reach = stroke.reach, .grid = stroke.grid, .square = stroke.square};
  // The legs are read as walls as thick as the wall to build.
  const Wall like = geometric(stroke.wall, build_of(model, stroke.wall));
  return geom::straighten(walls_on(model, level), ends_below(model, level), drawn, like)
      .and_then([&stroke](const std::vector<geom::Leg>& legs) -> Result<std::vector<Command>> {
        // The layers of a type start from a wall's right face: the outside of a loop drawn
        // counter-clockwise. A loop drawn the other way has its outside on the left.
        const bool mirrored = geom::runs_clockwise(legs);
        std::vector<Command> commands;
        commands.reserve(legs.size());
        for (const geom::Leg& leg : legs) {
          Wall wall = stroke.wall;
          wall.start = leg.start;
          wall.end = leg.end;
          wall.exact = false;
          wall.mirrored = mirrored;
          // The legs were checked as plain walls; the wall to build may still be one that
          // cannot be, such as a typed wall with a thickness of its own.
          if (auto valid = validate(wall); !valid) {
            return std::unexpected{std::move(valid).error()};
          }
          commands.emplace_back(AddWall{.wall = wall});
        }
        return commands;
      });
}

Result<drafting::Drawing> draw(const Model& model, const View& view) {
  if (model.find<Level>(view.level) == nullptr) {
    return make_error(ErrorCode::kNotFound, "level not found");
  }
  const std::vector<takeoff::BuiltWall> built = built_walls(model, view.level);
  std::vector<geom::WallInput> walls = inputs(built);
  std::ranges::sort(walls, {}, &geom::WallInput::id);
  const std::vector<geom::PlacedOpening> openings = openings_in(model, walls);
  std::vector<drafting::WallBuild> builds;
  builds.reserve(built.size());
  for (const takeoff::BuiltWall& wall : built) {
    builds.push_back({.wall = wall.id, .layers = wall.layers});
  }
  std::vector<geom::WallInput> below;
  if (const std::optional<ElementId> under = level_below(model, view.level)) {
    below = walls_on(model, *under);
  }
  return drafting::draft({.walls = walls, .openings = openings, .below = below, .builds = builds},
                         view.text);
}

Result<std::optional<ElementId>> pick(const Model& model, const Tap& tap) {
  if (model.find<Level>(tap.level) == nullptr) {
    return make_error(ErrorCode::kNotFound, "level not found");
  }
  const std::vector<geom::WallInput> walls = sorted_walls_on(model, tap.level);
  const std::vector<geom::PlacedOpening> openings = openings_in(model, walls);
  const geom::Vec2 at = geom::to_vec2(tap.at);
  const double reach = to_metres(std::max(tap.reach, Mm{0}));
  // Under the finger first, so a wall beside an opening stays easy to pick.
  if (auto opening = geom::hit_opening(openings, at, 0.0)) {
    return opening;
  }
  if (auto wall = geom::hit_wall(walls, at, 0.0)) {
    return wall->wall;
  }
  if (auto opening = geom::hit_opening(openings, at, reach)) {
    return opening;
  }
  if (auto wall = geom::hit_wall(walls, at, reach)) {
    return wall->wall;
  }
  return std::nullopt;
}

Result<AddOpening> place_opening(const Model& model, const OpeningPlacement& placement) {
  Opening opening = placement.opening;
  if (auto valid = validate(opening); !valid) {
    return std::unexpected{std::move(valid).error()};
  }
  if (model.find<Level>(placement.level) == nullptr) {
    return make_error(ErrorCode::kNotFound, "level not found");
  }
  const std::vector<geom::WallInput> walls = walls_on(model, placement.level);
  const std::optional<geom::WallHit> hit = geom::hit_wall(
      walls, geom::to_vec2(placement.at), to_metres(std::max(placement.reach, Mm{0})));
  if (!hit) {
    return make_error(ErrorCode::kNoWall, "no wall within reach of the point");
  }
  const Wall* wall = model.find<Wall>(hit->wall);
  const Mm length = axis_length(*wall);
  if (opening.width > length) {
    return make_error(ErrorCode::kDoesNotFit, "opening is wider than its wall");
  }
  const Mm centre = round_to_mm(hit->along);
  opening.wall = hit->wall;
  opening.offset =
      std::clamp(centre - Mm{opening.width.value() / 2}, Mm{0}, length - opening.width);
  return AddOpening{.opening = opening};
}

}  // namespace otves::bridge
