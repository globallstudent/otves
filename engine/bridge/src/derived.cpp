#include "derived.h"

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

TakeoffOffsets failure(flatbuffers::FlatBufferBuilder& builder, const Error& error) {
  return {.lines = builder.CreateVector(std::vector<flatbuffers::Offset<schema::QuantityLine>>{}),
          .error = to_schema(error.code),
          .message = builder.CreateString(error.message)};
}

}  // namespace

OutlinesOffset encode_outlines(flatbuffers::FlatBufferBuilder& builder, const Model& model) {
  std::map<ElementId, std::vector<geom::WallInput>> levels;
  for (const geom::WallInput& input : walls_of(model)) {
    levels[input.wall.level].push_back(input);
  }
  std::vector<flatbuffers::Offset<schema::WallOutline>> outlines;
  for (const auto& [level, walls] : levels) {
    const schema::Uuid on = to_schema(level);
    for (const geom::WallFootprint& footprint : geom::join_walls(walls).footprints) {
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

TakeoffOffsets encode_takeoff(flatbuffers::FlatBufferBuilder& builder, const Model& model,
                              const std::optional<takeoff::RuleSet>& rules) {
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
