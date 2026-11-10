#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <otves/core/assert.h>
#include <otves/core/error.h>
#include <otves/core/units.h>
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {

std::expected<void, Error> validate(const MasonrySpec& spec) {
  const MasonryUnit& unit = spec.unit;
  if (unit.length <= Mm{0} || unit.width <= Mm{0} || unit.height <= Mm{0}) {
    return make_error(ErrorCode::kUnitSizeNotPositive, "masonry unit sizes must be positive");
  }
  if (spec.joints.bed < Mm{0} || spec.joints.head < Mm{0}) {
    return make_error(ErrorCode::kJointNegative, "joint thickness must not be negative");
  }
  return {};
}

namespace {

// Adds one wall laid from `spec` to the result.
void count_wall(const WallQuantities& wall, const MasonrySpec& spec, UnitMasonry& of_unit,
                Masonry& result) {
  const MasonryUnit& unit = spec.unit;
  const Joints& joints = spec.joints;
  if (wall.thickness < unit.width) {
    result.issues.push_back({.wall = wall.id, .kind = MasonryIssueKind::kThinnerThanUnit});
    result.walls.push_back({.id = wall.id});
    return;
  }
  // A wall n wythes thick is n unit widths with n − 1 head joints between them.
  const std::int64_t wythe_pitch = (unit.width + joints.head).value();
  const std::int64_t span = (wall.thickness + joints.head).value();
  if (span % wythe_pitch != 0) {
    result.issues.push_back({.wall = wall.id, .kind = MasonryIssueKind::kPartialWythe});
  }
  const double face_cell =
      to_metres(unit.length + joints.head) * to_metres(unit.height + joints.bed);
  const double unit_volume =
      to_metres(unit.length) * to_metres(unit.width) * to_metres(unit.height);
  const double wythes = static_cast<double>(span) / static_cast<double>(wythe_pitch);
  const double units = wall.face_area * wythes / face_cell;
  // Never negative in exact arithmetic once the wall is at least one unit thick; the clamp
  // absorbs rounding when joints are zero.
  const double mortar = std::max(0.0, wall.volume - (units * unit_volume));
  result.walls.push_back({.id = wall.id, .units = units, .mortar = mortar});
  result.units += units;
  result.mortar += mortar;
  of_unit.units += units;
  of_unit.mortar += mortar;
}

}  // namespace

std::expected<Masonry, Error> count_masonry(std::span<const WallQuantities> walls,
                                            const MasonrySpec& spec) {
  if (auto valid = validate(spec); !valid) {
    return std::unexpected{std::move(valid).error()};
  }
  const std::vector<std::optional<MasonryUnit>> units(walls.size(), spec.unit);
  return count_masonry(walls, units, spec.joints);
}

std::expected<Masonry, Error> count_masonry(std::span<const WallQuantities> walls,
                                            std::span<const std::optional<MasonryUnit>> units,
                                            const Joints& joints) {
  OTVES_ASSERT(units.size() == walls.size());
  std::map<MasonryUnit, UnitMasonry> by_unit;
  Masonry result;
  result.walls.reserve(walls.size());
  for (std::size_t i = 0; i < walls.size() && i < units.size(); ++i) {
    const std::optional<MasonryUnit>& unit = units[i];
    if (!unit) {
      result.walls.push_back({.id = walls[i].id});
      continue;
    }
    const MasonrySpec spec{.unit = *unit, .joints = joints};
    if (auto valid = validate(spec); !valid) {
      return std::unexpected{std::move(valid).error()};
    }
    UnitMasonry& of_unit =
        by_unit.try_emplace(spec.unit, UnitMasonry{.unit = spec.unit}).first->second;
    count_wall(walls[i], spec, of_unit, result);
  }
  result.by_unit.reserve(by_unit.size());
  for (const auto& [unit, counted] : by_unit) {
    result.by_unit.push_back(counted);
  }
  std::ranges::sort(result.issues);
  return result;
}

}  // namespace otves::takeoff
