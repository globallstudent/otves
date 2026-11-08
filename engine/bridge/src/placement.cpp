#include "placement.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <expected>
#include <optional>
#include <utility>
#include <vector>

#include <otves/core/command.h>
#include <otves/core/error.h>
#include <otves/core/level.h>
#include <otves/core/model.h>
#include <otves/core/opening.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/geom/openings.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>

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
  std::vector<geom::WallInput> walls;
  model.for_each<Wall>([&walls, level](ElementId id, const Wall& wall) {
    if (wall.level == level) {
      walls.push_back({.id = id, .wall = wall});
    }
  });
  return walls;
}

}  // namespace

Result<std::optional<ElementId>> pick(const Model& model, const Tap& tap) {
  if (model.find<Level>(tap.level) == nullptr) {
    return make_error(ErrorCode::kNotFound, "level not found");
  }
  const std::vector<geom::WallInput> walls = walls_on(model, tap.level);
  std::vector<geom::PlacedOpening> openings;
  model.for_each<Opening>([&](ElementId id, const Opening& opening) {
    const Wall* wall = model.find<Wall>(opening.wall);
    if (wall != nullptr && wall->level == tap.level) {
      openings.push_back({.id = id, .wall = *wall, .opening = opening});
    }
  });
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
