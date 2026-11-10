#include "derived.h"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <flatbuffers/flatbuffers.h>

#include <otves/core/conditions.h>
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
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/rules.h>
#include <otves/takeoff/wall_quantities.h>

#include "convert.h"
#include "engine_generated.h"

namespace otves::bridge {
namespace {

std::vector<geom::WallInput> walls_of(const Model& model) {
  std::vector<geom::WallInput> walls;
  walls.reserve(model.count<Wall>());
  model.for_each<Wall>(
      [&walls](ElementId id, const Wall& wall) { walls.push_back({.id = id, .wall = wall}); });
  return walls;
}

std::vector<takeoff::OpeningInput> openings_of(const Model& model) {
  std::vector<takeoff::OpeningInput> openings;
  openings.reserve(model.count<Opening>());
  model.for_each<Opening>([&openings](ElementId id, const Opening& opening) {
    openings.push_back({.id = id, .opening = opening});
  });
  return openings;
}

schema::Unit unit_code(takeoff::Unit unit) {
  switch (unit) {
    case takeoff::Unit::kPiece:
      return schema::Unit::Piece;
    case takeoff::Unit::kMetre:
      return schema::Unit::Metre;
    case takeoff::Unit::kSquareMetre:
      return schema::Unit::SquareMetre;
    case takeoff::Unit::kCubicMetre:
      return schema::Unit::CubicMetre;
    case takeoff::Unit::kKilogram:
      return schema::Unit::Kilogram;
  }
  return schema::Unit::Piece;
}

schema::IssueKind issue_code(takeoff::OpeningIssueKind kind) {
  switch (kind) {
    case takeoff::OpeningIssueKind::kOutsideWall:
      return schema::IssueKind::OpeningOutsideWall;
    case takeoff::OpeningIssueKind::kOverlap:
      return schema::IssueKind::OpeningOverlap;
    case takeoff::OpeningIssueKind::kNoWall:
      return schema::IssueKind::OpeningWithoutWall;
    case takeoff::OpeningIssueKind::kShortBearing:
      return schema::IssueKind::ShortBearing;
    case takeoff::OpeningIssueKind::kNoRoomForLintel:
      return schema::IssueKind::NoRoomForLintel;
  }
  return schema::IssueKind::OpeningOutsideWall;
}

schema::IssueKind issue_code(geom::JoinIssueKind kind) {
  switch (kind) {
    case geom::JoinIssueKind::kOverlap:
      return schema::IssueKind::WallOverlap;
    case geom::JoinIssueKind::kSharpAngle:
      return schema::IssueKind::SharpAngle;
  }
  return schema::IssueKind::WallOverlap;
}

TakeoffOffsets failure(flatbuffers::FlatBufferBuilder& builder, const Error& error) {
  return {.lines = builder.CreateVector(std::vector<flatbuffers::Offset<schema::QuantityLine>>{}),
          .error = to_schema(error.code),
          .message = builder.CreateString(error.message)};
}

// Carries what the builder chose into the measurement (docs/adr/0008): each wall's ring beam and
// each junction's meshes from their level. Returns the unit of each measured wall, absent where
// none was chosen; such a wall is reported in `issues`.
std::vector<std::optional<takeoff::MasonryUnit>> choices(const Model& model,
                                                         takeoff::Measurement& measurement,
                                                         flatbuffers::FlatBufferBuilder& builder,
                                                         IssueOffsets& issues) {
  const auto level_of = [&model](ElementId wall_id) -> const Level* {
    const Wall* wall = model.find<Wall>(wall_id);
    return wall != nullptr ? model.find<Level>(wall->level) : nullptr;
  };
  std::vector<std::optional<takeoff::MasonryUnit>> units;
  units.reserve(measurement.walls.size());
  for (takeoff::WallQuantities& measured : measurement.walls) {
    const Wall* wall = model.find<Wall>(measured.id);
    const Level* level = level_of(measured.id);
    measured.ring_beam = level != nullptr ? to_metres(level->ring_beam) : 0.0;
    if (wall != nullptr && wall->has_unit()) {
      units.emplace_back(takeoff::MasonryUnit{
          .length = wall->unit_length, .width = wall->unit_width, .height = wall->unit_height});
      continue;
    }
    units.emplace_back(std::nullopt);
    const schema::Uuid element = to_schema(measured.id);
    const schema::Uuid on = to_schema(wall != nullptr ? wall->level : ElementId{});
    issues.push_back(
        schema::CreateIssue(builder, schema::IssueKind::MaterialNotChosen, &element, &on));
  }
  for (takeoff::JunctionQuantities& junction : measurement.junctions) {
    const Level* level = junction.walls.empty() ? nullptr : level_of(junction.walls.front());
    junction.meshes = level != nullptr && level->junction_meshes;
  }
  return units;
}

// The job's conditions, or all unset when it has none.
Conditions conditions_of(const Model& model) {
  Conditions found;
  model.for_each<Conditions>(
      [&found](ElementId, const Conditions& conditions) { found = conditions; });
  return found;
}

// The conditions the builder chose, as the settings rules read (docs/adr/0009); unset ones are
// left out, so the rule takes the norm default.
takeoff::Settings settings_of(const Conditions& conditions) {
  constexpr double per_mille = 1000.0;
  takeoff::Settings settings;
  if (conditions.seismic_intensity != Conditions::kUnset) {
    settings.emplace("seismic_intensity", static_cast<double>(conditions.seismic_intensity));
  }
  if (conditions.cement_grade != Conditions::kUnset) {
    settings.emplace("cement", "PC" + std::to_string(conditions.cement_grade));
  }
  if (conditions.mortar_grade != Conditions::kUnset) {
    settings.emplace("mortar_grade", "M" + std::to_string(conditions.mortar_grade));
  }
  if (conditions.unit_waste != Conditions::kUnset) {
    settings.emplace("unit_waste", static_cast<double>(conditions.unit_waste) / per_mille);
  }
  if (conditions.mortar_waste != Conditions::kUnset) {
    settings.emplace("mortar_waste", static_cast<double>(conditions.mortar_waste) / per_mille);
  }
  return settings;
}

// Reports the levels whose load-bearing walls lack the ring beam or the junction meshes the norm
// asks for. The take-off never adds them (docs/adr/0008); the builder decides.
void advise(const Model& model, const takeoff::Measurement& measurement,
            const takeoff::SeismicAdvice& advice, flatbuffers::FlatBufferBuilder& builder,
            IssueOffsets& issues) {
  if (!advice.required) {
    return;
  }
  const auto bearing = [&model, &advice](ElementId id) -> const Wall* {
    const Wall* wall = model.find<Wall>(id);
    return wall != nullptr && wall->thickness >= advice.load_bearing ? wall : nullptr;
  };
  std::set<ElementId> no_beam;
  std::set<ElementId> no_meshes;
  for (const takeoff::WallQuantities& measured : measurement.walls) {
    const Wall* wall = bearing(measured.id);
    if (wall != nullptr && measured.ring_beam == 0.0) {
      no_beam.insert(wall->level);
    }
  }
  for (const takeoff::JunctionQuantities& junction : measurement.junctions) {
    const Wall* first = nullptr;
    const auto meeting = std::ranges::count_if(junction.walls, [&](ElementId id) {
      const Wall* wall = bearing(id);
      first = wall != nullptr ? wall : first;
      return wall != nullptr;
    });
    if (meeting >= 2 && !junction.meshes && first != nullptr) {
      no_meshes.insert(first->level);
    }
  }
  const auto report = [&](const std::set<ElementId>& levels, schema::IssueKind kind) {
    for (const ElementId level : levels) {
      const schema::Uuid on = to_schema(level);
      issues.push_back(schema::CreateIssue(builder, kind, nullptr, &on));
    }
  };
  report(no_beam, schema::IssueKind::RingBeamMissing);
  report(no_meshes, schema::IssueKind::JunctionMeshesMissing);
}

}  // namespace

OutlinesOffset encode_outlines(flatbuffers::FlatBufferBuilder& builder, const Model& model,
                               IssueOffsets& issues) {
  std::map<ElementId, std::vector<geom::WallInput>> levels;
  for (const geom::WallInput& input : walls_of(model)) {
    levels[input.wall.level].push_back(input);
  }
  std::vector<flatbuffers::Offset<schema::WallOutline>> outlines;
  for (const auto& [level, walls] : levels) {
    const schema::Uuid on = to_schema(level);
    const geom::WallJoins joins = geom::join_walls(walls);
    for (const geom::JoinIssue& issue : joins.issues) {
      const schema::PointMm at = to_schema(issue.at);
      issues.push_back(schema::CreateIssue(builder, issue_code(issue.kind), nullptr, &on, &at));
    }
    for (const geom::WallFootprint& footprint : joins.footprints) {
      std::vector<schema::PointM> points;
      points.reserve(footprint.outline.size());
      for (const geom::Vec2 point : footprint.outline) {
        points.emplace_back(point.x, point.y);
      }
      const schema::Uuid wall = to_schema(footprint.id);
      const Wall* drawn = model.find<Wall>(footprint.id);
      const Mm length =
          round_to_mm(geom::length(geom::to_vec2(drawn->end) - geom::to_vec2(drawn->start)));
      outlines.push_back(schema::CreateWallOutline(
          builder, &wall, &on, builder.CreateVectorOfStructs(points), length.value()));
    }
  }
  return builder.CreateVector(outlines);
}

OpeningOutlinesOffset encode_openings(flatbuffers::FlatBufferBuilder& builder, const Model& model) {
  std::vector<takeoff::OpeningInput> openings = openings_of(model);
  std::ranges::sort(openings, {}, &takeoff::OpeningInput::id);
  std::vector<flatbuffers::Offset<schema::OpeningOutline>> outlines;
  outlines.reserve(openings.size());
  for (const takeoff::OpeningInput& input : openings) {
    const Wall* wall = model.find<Wall>(input.opening.wall);
    if (wall == nullptr) {
      continue;
    }
    std::vector<schema::PointM> points;
    for (const geom::Vec2 point : geom::opening_outline(*wall, input.opening)) {
      points.emplace_back(point.x, point.y);
    }
    const schema::Uuid opening = to_schema(input.id);
    const schema::Uuid level = to_schema(wall->level);
    outlines.push_back(schema::CreateOpeningOutline(builder, &opening, &level,
                                                    to_schema(input.opening.kind),
                                                    builder.CreateVectorOfStructs(points)));
  }
  return builder.CreateVector(outlines);
}

TakeoffOffsets encode_takeoff(flatbuffers::FlatBufferBuilder& builder, const Model& model,
                              const std::optional<takeoff::RuleSet>& rules, IssueOffsets& issues) {
  if (!rules) {
    return {};
  }
  const auto lintels = takeoff::lintel_spec(*rules);
  if (!lintels) {
    return failure(builder, lintels.error());
  }
  const auto joints = takeoff::masonry_joints(*rules);
  if (!joints) {
    return failure(builder, joints.error());
  }
  auto measurement = takeoff::measure_levels(walls_of(model), openings_of(model), *lintels);
  if (!measurement) {
    return failure(builder, measurement.error());
  }
  const std::vector<std::optional<takeoff::MasonryUnit>> units =
      choices(model, *measurement, builder, issues);
  const Conditions conditions = conditions_of(model);
  // Advice is extra: content without these norms still gives a take-off.
  const std::optional<std::int64_t> intensity = conditions.seismic_intensity == Conditions::kUnset
                                                    ? std::nullopt
                                                    : std::optional{conditions.seismic_intensity};
  if (const auto advice = takeoff::seismic_advice(*rules, intensity)) {
    advise(model, *measurement, *advice, builder, issues);
  }
  for (const takeoff::OpeningIssue& issue : measurement->issues) {
    const auto* opening = model.find<Opening>(issue.opening);
    const Wall* wall = opening != nullptr ? model.find<Wall>(opening->wall) : nullptr;
    const schema::Uuid element = to_schema(issue.opening);
    const schema::Uuid level = to_schema(wall != nullptr ? wall->level : ElementId{});
    issues.push_back(schema::CreateIssue(builder, issue_code(issue.kind), &element, &level));
  }
  const auto masonry = takeoff::count_masonry(measurement->walls, units, *joints);
  if (!masonry) {
    return failure(builder, masonry.error());
  }
  const auto lines = rules->run(*measurement, *masonry, settings_of(conditions));
  if (!lines) {
    return failure(builder, lines.error());
  }
  std::vector<flatbuffers::Offset<schema::QuantityLine>> encoded;
  encoded.reserve(lines->size());
  for (const takeoff::QuantityLine& line : *lines) {
    encoded.push_back(schema::CreateQuantityLine(builder, builder.CreateString(line.rule),
                                                 builder.CreateString(line.item),
                                                 unit_code(line.unit), line.amount));
  }
  return {.lines = builder.CreateVector(encoded), .error = schema::ErrorCode::None, .message = {}};
}

}  // namespace otves::bridge
