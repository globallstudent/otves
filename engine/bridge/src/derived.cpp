#include "derived.h"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <flatbuffers/flatbuffers.h>

#include <otves/core/build.h>
#include <otves/core/conditions.h>
#include <otves/core/error.h>
#include <otves/core/level.h>
#include <otves/core/model.h>
#include <otves/core/opening.h>
#include <otves/core/room.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/core/wall_type.h>
#include <otves/drafting/plan.h>
#include <otves/geom/openings.h>
#include <otves/geom/spaces.h>
#include <otves/geom/vec2.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/layers.h>
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/rules.h>
#include <otves/takeoff/wall_quantities.h>

#include "built.h"
#include "convert.h"
#include "engine_generated.h"
#include "placement.h"

namespace otves::bridge {
namespace {

std::vector<geom::WallInput> walls_of(const Model& model) { return inputs(built_walls(model)); }

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

schema::Fill fill_code(drafting::Fill fill) {
  switch (fill) {
    case drafting::Fill::kNone:
      return schema::Fill::None;
    case drafting::Fill::kWall:
      return schema::Fill::Wall;
    case drafting::Fill::kUnchosen:
      return schema::Fill::Unchosen;
    case drafting::Fill::kGap:
      return schema::Fill::Gap;
    case drafting::Fill::kFacing:
      return schema::Fill::Facing;
    case drafting::Fill::kInsulation:
      return schema::Fill::Insulation;
    case drafting::Fill::kFloor:
      return schema::Fill::Floor;
  }
  return schema::Fill::None;
}

schema::Stroke stroke_code(drafting::Stroke stroke) {
  switch (stroke) {
    case drafting::Stroke::kNone:
      return schema::Stroke::None;
    case drafting::Stroke::kWallEdge:
      return schema::Stroke::WallEdge;
    case drafting::Stroke::kBelow:
      return schema::Stroke::Below;
    case drafting::Stroke::kFrame:
      return schema::Stroke::Frame;
    case drafting::Stroke::kLeaf:
      return schema::Stroke::Leaf;
    case drafting::Stroke::kLayer:
      return schema::Stroke::Layer;
    case drafting::Stroke::kDimension:
      return schema::Stroke::Dimension;
    case drafting::Stroke::kChain:
      return schema::Stroke::Chain;
  }
  return schema::Stroke::None;
}

schema::TextKind text_code(drafting::TextKind kind) {
  switch (kind) {
    case drafting::TextKind::kLength:
      return schema::TextKind::Length;
    case drafting::TextKind::kAboutLength:
      return schema::TextKind::AboutLength;
    case drafting::TextKind::kRoomName:
      return schema::TextKind::RoomName;
    case drafting::TextKind::kRoomArea:
      return schema::TextKind::RoomArea;
    case drafting::TextKind::kOverall:
      return schema::TextKind::Overall;
    case drafting::TextKind::kClear:
      return schema::TextKind::Clear;
    case drafting::TextKind::kPier:
      return schema::TextKind::Pier;
    case drafting::TextKind::kOpeningWidth:
      return schema::TextKind::OpeningWidth;
  }
  return schema::TextKind::Length;
}

// A room names its level's space that holds its anchor; one that names none is reported, with
// its anchor as the point (docs/adr/0015).
void report_rooms(const Model& model,
                  const std::map<ElementId, std::vector<geom::WallInput>>& levels,
                  flatbuffers::FlatBufferBuilder& builder, IssueOffsets& issues) {
  std::map<ElementId, std::vector<geom::Anchor>> anchors;
  model.for_each<Room>([&anchors](ElementId id, const Room& room) {
    anchors[room.level].push_back({.room = id, .at = room.anchor});
  });
  for (const auto& [level, named] : anchors) {
    const auto walls = levels.find(level);
    const std::vector<geom::Space> spaces =
        walls != levels.end() ? geom::find_spaces(walls->second) : std::vector<geom::Space>{};
    const schema::Uuid on = to_schema(level);
    for (const geom::AnchorIssue& issue : geom::name_spaces(spaces, named).issues) {
      const schema::Uuid room = to_schema(issue.room);
      const schema::PointMm at = to_schema(model.find<Room>(issue.room)->anchor);
      const schema::IssueKind kind = issue.kind == geom::AnchorIssueKind::kNoSpace
                                         ? schema::IssueKind::RoomWithoutSpace
                                         : schema::IssueKind::RoomSharesSpace;
      issues.push_back(schema::CreateIssue(builder, kind, &room, &on, &at));
    }
  }
}

TakeoffOffsets failure(flatbuffers::FlatBufferBuilder& builder, const Error& error) {
  return {.lines = builder.CreateVector(std::vector<flatbuffers::Offset<schema::QuantityLine>>{}),
          .error = to_schema(error.code),
          .message = builder.CreateString(error.message)};
}

// Carries what the builder chose into the measurement (docs/adr/0008): each wall's ring beam and
// footing and each junction's meshes from their level, and whether the wall bears load from its
// type (docs/adr/0014).
void choices(const Model& model, takeoff::Measurement& measurement) {
  const auto level_of = [&model](ElementId wall_id) -> const Level* {
    const Wall* wall = model.find<Wall>(wall_id);
    return wall != nullptr ? model.find<Level>(wall->level) : nullptr;
  };
  for (takeoff::WallQuantities& measured : measurement.walls) {
    const Wall* wall = model.find<Wall>(measured.id);
    const Level* level = level_of(measured.id);
    measured.ring_beam = level != nullptr ? to_metres(level->ring_beam) : 0.0;
    measured.footing_width = level != nullptr ? to_metres(level->footing_width) : 0.0;
    measured.footing_depth = level != nullptr ? to_metres(level->footing_depth) : 0.0;
    const WallType* type = wall != nullptr ? model.find<WallType>(wall->type) : nullptr;
    if (type != nullptr) {
      measured.bearing = type->role != WallRole::kPartition;
    }
  }
  for (takeoff::JunctionQuantities& junction : measurement.junctions) {
    const Level* level = junction.walls.empty() ? nullptr : level_of(junction.walls.front());
    junction.meshes = level != nullptr && level->junction_meshes;
  }
}

// Reports the walls with a masonry layer that has no unit yet: such a layer is not in the
// take-off.
void report_unchosen(std::span<const takeoff::BuiltWall> walls,
                     flatbuffers::FlatBufferBuilder& builder, IssueOffsets& issues) {
  for (const takeoff::BuiltWall& built : walls) {
    const bool unchosen = std::ranges::any_of(built.layers, [](const BuiltLayer& layer) {
      return layer.is_masonry() && !layer.has_unit();
    });
    if (unchosen) {
      const schema::Uuid element = to_schema(built.id);
      const schema::Uuid on = to_schema(built.wall.level);
      issues.push_back(
          schema::CreateIssue(builder, schema::IssueKind::MaterialNotChosen, &element, &on));
    }
  }
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
  // A wall's type says whether it bears load; a wall with no type is judged by its thickness.
  const auto bearing = [&](ElementId id) -> const Wall* {
    const auto measured =
        std::ranges::lower_bound(measurement.walls, id, {}, &takeoff::WallQuantities::id);
    if (measured == measurement.walls.end() || measured->id != id) {
      return nullptr;
    }
    const bool bears = measured->bearing.value_or(measured->thickness >= advice.load_bearing);
    return bears ? model.find<Wall>(id) : nullptr;
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
      if (drawn == nullptr) {
        continue;
      }
      const Mm length =
          round_to_mm(geom::length(geom::to_vec2(drawn->end) - geom::to_vec2(drawn->start)));
      outlines.push_back(schema::CreateWallOutline(
          builder, &wall, &on, builder.CreateVectorOfStructs(points), length.value()));
    }
  }
  report_rooms(model, levels, builder, issues);
  return builder.CreateVector(outlines);
}

flatbuffers::Offset<schema::PickedSpace> encode_space(flatbuffers::FlatBufferBuilder& builder,
                                                      const FoundSpace& found) {
  const auto ring = [&builder](const std::vector<geom::Vec2>& outline) {
    std::vector<schema::PointM> points;
    points.reserve(outline.size());
    for (const geom::Vec2 point : outline) {
      points.emplace_back(point.x, point.y);
    }
    return builder.CreateVectorOfStructs(points);
  };
  std::vector<flatbuffers::Offset<schema::Ring>> islands;
  islands.reserve(found.space.islands.size());
  for (const std::vector<geom::Vec2>& island : found.space.islands) {
    islands.push_back(schema::CreateRing(builder, ring(island)));
  }
  const auto outline = ring(found.space.outline);
  const auto inside = builder.CreateVector(islands);
  const schema::Uuid room = to_schema(found.room);
  return schema::CreatePickedSpace(builder, found.room.is_nil() ? nullptr : &room,
                                   to_schema(found.kind), found.space.area, found.space.perimeter,
                                   outline, inside);
}

OpeningOutlinesOffset encode_openings(flatbuffers::FlatBufferBuilder& builder, const Model& model) {
  std::vector<takeoff::OpeningInput> openings = openings_of(model);
  std::ranges::sort(openings, {}, &takeoff::OpeningInput::id);
  std::vector<geom::WallInput> walls = walls_of(model);
  std::ranges::sort(walls, {}, &geom::WallInput::id);
  std::vector<flatbuffers::Offset<schema::OpeningOutline>> outlines;
  outlines.reserve(openings.size());
  for (const takeoff::OpeningInput& input : openings) {
    const auto host = std::ranges::lower_bound(walls, input.opening.wall, {}, &geom::WallInput::id);
    if (host == walls.end() || host->id != input.opening.wall) {
      continue;
    }
    const Wall* wall = &host->wall;
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

flatbuffers::Offset<schema::Drawing> encode_drawing(flatbuffers::FlatBufferBuilder& builder,
                                                    const drafting::Drawing& drawing) {
  const auto point = [](geom::Vec2 at) { return schema::PointM{at.x, at.y}; };
  // A primitive that draws no element carries no id.
  const auto key = [](ElementId id, schema::Uuid& out) -> const schema::Uuid* {
    out = to_schema(id);
    return id.is_nil() ? nullptr : &out;
  };
  schema::Uuid id;
  std::vector<flatbuffers::Offset<schema::DrawnPolygon>> polygons;
  polygons.reserve(drawing.polygons.size());
  for (const drafting::Polygon& polygon : drawing.polygons) {
    std::vector<schema::PointM> points;
    points.reserve(polygon.points.size());
    std::ranges::transform(polygon.points, std::back_inserter(points), point);
    const auto outline = builder.CreateVectorOfStructs(points);
    polygons.push_back(schema::CreateDrawnPolygon(builder, key(polygon.element, id),
                                                  fill_code(polygon.fill),
                                                  stroke_code(polygon.stroke), outline));
  }
  std::vector<flatbuffers::Offset<schema::DrawnLine>> lines;
  lines.reserve(drawing.lines.size());
  for (const drafting::Line& line : drawing.lines) {
    const schema::PointM from = point(line.from);
    const schema::PointM to = point(line.to);
    lines.push_back(schema::CreateDrawnLine(builder, key(line.element, id),
                                            stroke_code(line.stroke), &from, &to));
  }
  std::vector<flatbuffers::Offset<schema::DrawnArc>> arcs;
  arcs.reserve(drawing.arcs.size());
  for (const drafting::Arc& arc : drawing.arcs) {
    const schema::PointM centre = point(arc.centre);
    const schema::PointM from = point(arc.from);
    const schema::PointM to = point(arc.to);
    arcs.push_back(schema::CreateDrawnArc(builder, key(arc.element, id), stroke_code(arc.stroke),
                                          &centre, &from, &to));
  }
  std::vector<flatbuffers::Offset<schema::DrawnText>> texts;
  texts.reserve(drawing.texts.size());
  for (const drafting::Text& text : drawing.texts) {
    const schema::PointM at = point(text.at);
    const schema::PointM along = point(text.along);
    schema::Uuid other;
    const schema::Uuid* far = key(text.other, other);
    texts.push_back(schema::CreateDrawnText(builder, key(text.element, id), text_code(text.kind),
                                            text.value, &at, &along, text.fit, text.from.value(),
                                            text.forward, far));
  }
  return schema::CreateDrawing(builder, builder.CreateVector(polygons), builder.CreateVector(lines),
                               builder.CreateVector(arcs), builder.CreateVector(texts));
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
  const std::vector<takeoff::BuiltWall> built = built_walls(model);
  auto measurement = takeoff::measure_levels(inputs(built), openings_of(model), *lintels);
  if (!measurement) {
    return failure(builder, measurement.error());
  }
  choices(model, *measurement);
  report_unchosen(built, builder, issues);
  // Every masonry layer is counted as a wall of its own; insulation goes to its own rule.
  takeoff::Layers layers = takeoff::measure_layers(built, *measurement);
  measurement->insulation = std::move(layers.insulation);
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
  const auto masonry = takeoff::count_masonry(layers.masonry, layers.units, *joints);
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
