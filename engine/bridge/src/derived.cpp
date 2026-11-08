#include "derived.h"

#include <algorithm>
#include <expected>
#include <map>
#include <optional>
#include <vector>

#include <flatbuffers/flatbuffers.h>

#include <otves/core/error.h>
#include <otves/core/model.h>
#include <otves/core/opening.h>
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
      outlines.push_back(
          schema::CreateWallOutline(builder, &wall, &on, builder.CreateVectorOfStructs(points)));
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
  const auto masonry_spec = takeoff::masonry_spec(*rules);
  if (!masonry_spec) {
    return failure(builder, masonry_spec.error());
  }
  const auto measurement = takeoff::measure_levels(walls_of(model), openings_of(model), *lintels);
  if (!measurement) {
    return failure(builder, measurement.error());
  }
  for (const takeoff::OpeningIssue& issue : measurement->issues) {
    const auto* opening = model.find<Opening>(issue.opening);
    const Wall* wall = opening != nullptr ? model.find<Wall>(opening->wall) : nullptr;
    const schema::Uuid element = to_schema(issue.opening);
    const schema::Uuid level = to_schema(wall != nullptr ? wall->level : ElementId{});
    issues.push_back(schema::CreateIssue(builder, issue_code(issue.kind), &element, &level));
  }
  const auto masonry = takeoff::count_masonry(measurement->walls, *masonry_spec);
  if (!masonry) {
    return failure(builder, masonry.error());
  }
  const auto lines = rules->run(*measurement, *masonry, {});
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
