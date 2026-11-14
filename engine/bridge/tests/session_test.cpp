#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <flatbuffers/flatbuffers.h>
#include <gtest/gtest.h>

#include <otves/bridge/session.h>
#include <otves/core/error.h>
#include <otves/core/op.h>
#include <otves/core/uuid.h>

#include "engine_generated.h"
#include "oplog.h"

namespace otves::bridge {
namespace {

using schema::Command;

ElementId to_id(const schema::Uuid* uuid) {
  return uuid == nullptr ? ElementId{} : ElementId{Uuid{uuid->high(), uuid->low()}};
}

schema::Uuid key(ElementId id) { return {id.uuid().high(), id.uuid().low()}; }

// Builds a Request the way the app does.
class Request {
 public:
  Request& add_level(std::int64_t elevation, std::int64_t height) {
    const auto level = schema::CreateLevel(builder_, elevation, height);
    return add(Command::AddLevel, schema::CreateAddLevel(builder_, level).Union());
  }

  Request& add_wall(ElementId level, std::int64_t length) {
    const schema::Uuid on = key(level);
    const schema::PointMm start{0, 0};
    const schema::PointMm end{length, 0};
    const auto wall = schema::CreateWall(builder_, &on, &start, &end, 380, 2800);
    return add(Command::AddWall, schema::CreateAddWall(builder_, wall).Union());
  }

  Request& add_wall(ElementId level, schema::PointMm start, schema::PointMm end) {
    const schema::Uuid on = key(level);
    const auto wall = schema::CreateWall(builder_, &on, &start, &end, 380, 2800);
    return add(Command::AddWall, schema::CreateAddWall(builder_, wall).Union());
  }

  // A touch with a 300 mm reach on a 100 mm grid, for a wall from `from` if there is one.
  Request& aim(ElementId level, schema::PointMm at, const schema::PointMm* from = nullptr) {
    const schema::Uuid on = key(level);
    return add(Command::Aim, schema::CreateAim(builder_, &on, from, &at, 300, 100).Union());
  }

  // A wall of a typed length turning from the wall that ran from `before` to `from`.
  Request& aim_measured(ElementId level, schema::PointMm from, schema::PointMm before,
                        schema::Turn turn, std::int64_t length) {
    const schema::Uuid on = key(level);
    return add(Command::AimMeasured,
               schema::CreateAimMeasured(builder_, &on, &from, &before, turn, length).Union());
  }

  Request& set_length(ElementId wall, std::int64_t length,
                      schema::FixedEnd fixed = schema::FixedEnd::Joined) {
    const schema::Uuid id = key(wall);
    return add(Command::SetWallLength,
               schema::CreateSetWallLength(builder_, &id, length, fixed).Union());
  }

  // A stroke read with a 300 mm reach on a 100 mm grid, its walls held to the axes.
  Request& add_sketch(ElementId level, const std::vector<schema::PointMm>& points,
                      std::int64_t thickness = 380) {
    const schema::Uuid on = key(level);
    const auto wall = schema::CreateWall(builder_, &on, nullptr, nullptr, thickness, 2800);
    const auto path = builder_.CreateVectorOfStructs(points);
    return add(Command::AddSketch,
               schema::CreateAddSketch(builder_, wall, path, 300, 100, true).Union());
  }

  // The drawing of a level with label text 300 mm high on the plan and digits 150 mm wide.
  Request& draw(ElementId level) {
    const schema::Uuid on = key(level);
    return add(Command::Draw, schema::CreateDraw(builder_, &on, 300, 150).Union());
  }

  // The level with the space at `focus` drawn with its sizes.
  Request& draw(ElementId level, schema::PointMm focus) {
    const schema::Uuid on = key(level);
    return add(Command::Draw, schema::CreateDraw(builder_, &on, 300, 150, &focus).Union());
  }

  // A typed value for the size a drawing's text showed.
  Request& set_dimension(const schema::DrawnText& text, std::int64_t value) {
    return add(Command::SetDimension,
               schema::CreateSetDimension(builder_, text.kind(), text.element(), text.from(),
                                          text.forward(), value, text.other())
                   .Union());
  }

  Request& set_dimension(schema::TextKind kind, ElementId element, std::int64_t value) {
    const schema::Uuid id = key(element);
    return add(Command::SetDimension,
               schema::CreateSetDimension(builder_, kind, &id, 0, false, value).Union());
  }

  // The outside wall of the reference plans: 120 of facing brick, 100 of insulation and 190 of
  // block from the outer face in, or as many millimetres of insulation as asked.
  Request& add_wall_type(schema::WallRole role = schema::WallRole::Exterior,
                         std::int64_t insulation = 100) {
    const std::vector layers{
        schema::CreateWallLayer(builder_, nullptr, 0, schema::LayerFunction::Facing, 120, 250, 120,
                                65),
        schema::CreateWallLayer(builder_, nullptr, 0, schema::LayerFunction::Insulation,
                                insulation),
        schema::CreateWallLayer(builder_, nullptr, 0, schema::LayerFunction::Structure, 190, 390,
                                190, 188)};
    return add(Command::AddWallType,
               schema::CreateAddWallType(builder_, role, builder_.CreateVector(layers)).Union());
  }

  // A wall of a type: it carries no thickness of its own.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters): a level, then a type, as everywhere
  Request& add_typed_wall(ElementId level, ElementId type, schema::PointMm start,
                          schema::PointMm end, std::int64_t thickness = 0) {
    const schema::Uuid on = key(level);
    const schema::Uuid of = key(type);
    const auto wall = schema::CreateWall(builder_, &on, &start, &end, thickness, 2800,
                                         schema::LocationLine::Center, 0, 0, 0, true, &of);
    return add(Command::AddWall, schema::CreateAddWall(builder_, wall).Union());
  }

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters): the wall, then what it becomes
  Request& set_wall_type(ElementId wall, ElementId type) {
    const schema::Uuid id = key(wall);
    const schema::Uuid of = key(type);
    return add(Command::SetWallType, schema::CreateSetWallType(builder_, &id, &of).Union());
  }

  Request& set_mirrored(ElementId wall, bool mirrored) {
    const schema::Uuid id = key(wall);
    return add(Command::SetWallMirrored,
               schema::CreateSetWallMirrored(builder_, &id, mirrored).Union());
  }

  // A layer of insulation that thick.
  Request& set_layer(ElementId layer, std::int64_t thickness) {
    const schema::Uuid id = key(layer);
    return add(Command::SetWallLayer, schema::CreateSetWallLayer(builder_, &id, thickness).Union());
  }

  Request& set_role(ElementId type, schema::WallRole role) {
    const schema::Uuid id = key(type);
    return add(Command::SetWallTypeRole,
               schema::CreateSetWallTypeRole(builder_, &id, role).Union());
  }

  // A stroke read as walls of a type.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters): a level, then a type, as everywhere
  Request& add_typed_sketch(ElementId level, ElementId type,
                            const std::vector<schema::PointMm>& points) {
    const schema::Uuid on = key(level);
    const schema::Uuid of = key(type);
    const auto wall = schema::CreateWall(builder_, &on, nullptr, nullptr, 0, 2800,
                                         schema::LocationLine::Center, 0, 0, 0, true, &of);
    const auto path = builder_.CreateVectorOfStructs(points);
    return add(Command::AddSketch,
               schema::CreateAddSketch(builder_, wall, path, 300, 100, true).Union());
  }

  // A wall up from the origin, meeting add_wall's at a corner.
  Request& add_wall_up(ElementId level, std::int64_t length) {
    const schema::Uuid on = key(level);
    const schema::PointMm start{0, 0};
    const schema::PointMm end{0, length};
    const auto wall = schema::CreateWall(builder_, &on, &start, &end, 380, 2800);
    return add(Command::AddWall, schema::CreateAddWall(builder_, wall).Union());
  }

  // 1НФ brick, as laid.
  Request& set_unit(ElementId wall) {
    const schema::Uuid id = key(wall);
    return add(Command::SetWallUnit,
               schema::CreateSetWallUnit(builder_, &id, 250, 120, 65).Union());
  }

  Request& set_ring_beam(ElementId level, std::int64_t height) {
    const schema::Uuid id = key(level);
    return add(Command::SetLevelRingBeam,
               schema::CreateSetLevelRingBeam(builder_, &id, height).Union());
  }

  Request& set_condition(schema::ConditionsField field, std::int64_t value) {
    return add(Command::SetCondition, schema::CreateSetCondition(builder_, field, value).Union());
  }

  Request& set_footing(ElementId level, std::int64_t width, std::int64_t depth) {
    const schema::Uuid id = key(level);
    return add(Command::SetLevelFooting,
               schema::CreateSetLevelFooting(builder_, &id, width, depth).Union());
  }

  Request& set_meshes(ElementId level, bool meshes) {
    const schema::Uuid id = key(level);
    return add(Command::SetLevelJunctionMeshes,
               schema::CreateSetLevelJunctionMeshes(builder_, &id, meshes).Union());
  }

  Request& add_window(ElementId wall) {
    const schema::Uuid in = key(wall);
    const auto opening = schema::CreateOpening(builder_, &in, 1200, 1500, 1500, 900);
    return add(Command::AddOpening, schema::CreateAddOpening(builder_, opening).Union());
  }

  // A 1500 × 1500 window tapped with a 300 mm reach, or a door of that width.
  Request& place(ElementId level, schema::PointMm at, std::int64_t width = 1500,
                 schema::OpeningKind kind = schema::OpeningKind::Window) {
    const schema::Uuid on = key(level);
    return add(Command::PlaceOpening,
               schema::CreatePlaceOpening(builder_, &on, &at, 300, width, 1500, 800, kind).Union());
  }

  Request& move_end(ElementId wall, std::int64_t x, schema::WallEnd end = schema::WallEnd::End) {
    const schema::Uuid id = key(wall);
    const schema::PointMm to{x, 0};
    return add(Command::MoveWallEnd, schema::CreateMoveWallEnd(builder_, &id, end, &to).Union());
  }

  Request& set_height(ElementId wall, std::int64_t height) {
    const schema::Uuid id = key(wall);
    return add(Command::SetWallHeight, schema::CreateSetWallHeight(builder_, &id, height).Union());
  }

  Request& remove_wall(ElementId wall) {
    const schema::Uuid id = key(wall);
    return add(Command::RemoveWall, schema::CreateRemoveWall(builder_, &id).Union());
  }

  // A name for the space at `at`.
  Request& name_room(ElementId level, schema::PointMm at, schema::RoomKind kind) {
    const schema::Uuid on = key(level);
    const auto room = schema::CreateRoom(builder_, &on, &at, kind);
    return add(Command::AddRoom, schema::CreateAddRoom(builder_, room).Union());
  }

  Request& set_room_kind(ElementId room, schema::RoomKind kind) {
    const schema::Uuid id = key(room);
    return add(Command::SetRoomKind, schema::CreateSetRoomKind(builder_, &id, kind).Union());
  }

  Request& move_room(ElementId room, schema::PointMm anchor) {
    const schema::Uuid id = key(room);
    return add(Command::MoveRoom, schema::CreateMoveRoom(builder_, &id, &anchor).Union());
  }

  Request& remove_room(ElementId room) {
    const schema::Uuid id = key(room);
    return add(Command::RemoveRoom, schema::CreateRemoveRoom(builder_, &id).Union());
  }

  // The level as a sheet has it: every room with its sizes.
  Request& draw_sheet(ElementId level) {
    const schema::Uuid on = key(level);
    return add(Command::Draw,
               schema::CreateDraw(builder_, &on, 300, 150, nullptr, nullptr, true).Union());
  }

  // The level with the selected element marked for the engine.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters): a level, then what is on it
  Request& draw_selected(ElementId level, ElementId selected) {
    const schema::Uuid on = key(level);
    const schema::Uuid chosen = key(selected);
    return add(Command::Draw,
               schema::CreateDraw(builder_, &on, 300, 150, nullptr, &chosen).Union());
  }

  Request& set_hinge(ElementId opening, schema::Hinge hinge) {
    const schema::Uuid id = key(opening);
    return add(Command::SetOpeningHinge,
               schema::CreateSetOpeningHinge(builder_, &id, hinge).Union());
  }

  Request& set_swing(ElementId opening, schema::Swing swing) {
    const schema::Uuid id = key(opening);
    return add(Command::SetOpeningSwing,
               schema::CreateSetOpeningSwing(builder_, &id, swing).Union());
  }

  Request& remove_opening(ElementId opening) {
    const schema::Uuid id = key(opening);
    return add(Command::RemoveOpening, schema::CreateRemoveOpening(builder_, &id).Union());
  }

  Request& center(ElementId opening) {
    const schema::Uuid id = key(opening);
    return add(Command::CenterOpening, schema::CreateCenterOpening(builder_, &id).Union());
  }

  // What a tap with a 300 mm reach picks.
  Request& pick(ElementId level, schema::PointMm at) {
    const schema::Uuid on = key(level);
    return add(Command::Pick, schema::CreatePick(builder_, &on, &at, 300).Union());
  }

  // A saved ground floor (Uuid{10, 1}) with a 6.4 m wall (Uuid{10, 2}) and, if asked, a window
  // in it (Uuid{10, 3}) or one in a wall that is not saved.
  enum class Window : std::uint8_t { kNone, kInWall, kOrphan };

  Request& open_model(std::uint64_t clock, Window window = Window::kInWall) {
    std::vector<flatbuffers::Offset<schema::ElementChange>> saved;
    const schema::Uuid level{10, 1};
    const schema::Uuid wall{10, 2};
    const schema::Uuid opening{10, 3};
    const schema::Uuid missing{10, 9};
    saved.push_back(schema::CreateElementChange(builder_, &level, schema::Element::Level,
                                                schema::CreateLevel(builder_, 0, 3000).Union()));
    const schema::PointMm start{0, 0};
    const schema::PointMm end{6400, 0};
    saved.push_back(schema::CreateElementChange(
        builder_, &wall, schema::Element::Wall,
        schema::CreateWall(builder_, &level, &start, &end, 380, 2800).Union()));
    if (window != Window::kNone) {
      const schema::Uuid* host = window == Window::kInWall ? &wall : &missing;
      saved.push_back(schema::CreateElementChange(
          builder_, &opening, schema::Element::Opening,
          schema::CreateOpening(builder_, host, 1200, 1500, 1500, 900).Union()));
    }
    return add(Command::OpenModel,
               schema::CreateOpenModel(builder_, builder_.CreateVector(saved), clock).Union());
  }

  Request& begin_gesture() {
    return add(Command::BeginGesture, schema::CreateBeginGesture(builder_).Union());
  }
  Request& end_gesture() {
    return add(Command::EndGesture, schema::CreateEndGesture(builder_).Union());
  }
  Request& undo() { return add(Command::Undo, schema::CreateUndo(builder_).Union()); }

  // Script files as (name, source); the name is a norm's key or a rule's tag.
  using Files = std::vector<std::pair<std::string, std::string>>;

  Request& load_content(const Files& norms, const Files& rules) {
    const auto scripts = [this](const Files& files) {
      std::vector<flatbuffers::Offset<schema::Script>> out;
      for (const auto& [name, source] : files) {
        out.push_back(schema::CreateScript(builder_, builder_.CreateString(name),
                                           builder_.CreateString(source)));
      }
      return builder_.CreateVector(out);
    };
    const auto norm_scripts = scripts(norms);
    const auto rule_scripts = scripts(rules);
    return add(Command::LoadContent,
               schema::CreateLoadContent(builder_, norm_scripts, rule_scripts).Union());
  }
  Request& redo() { return add(Command::Redo, schema::CreateRedo(builder_).Union()); }

  // A union entry with its type set but no table, which the verifier lets through.
  Request& empty(Command type) { return add(type, 0); }

  std::vector<std::uint8_t> bytes(std::uint32_t version = 1) {
    builder_.Finish(schema::CreateRequest(builder_, version, builder_.CreateVector(entries_)),
                    "OTVS");
    const std::span<const std::uint8_t> data{builder_.GetBufferPointer(), builder_.GetSize()};
    return {data.begin(), data.end()};
  }

 private:
  Request& add(Command type, flatbuffers::Offset<void> command) {
    entries_.push_back(schema::CreateCommandEntry(builder_, type, command));
    return *this;
  }

  flatbuffers::FlatBufferBuilder builder_;
  std::vector<flatbuffers::Offset<schema::CommandEntry>> entries_;
};

// The response stays valid until the session's next call.
const schema::Response& respond(Session& session, std::span<const std::uint8_t> request) {
  const std::span<const std::uint8_t> bytes = session.handle(request);
  flatbuffers::Verifier verifier(bytes.data(), bytes.size());
  EXPECT_TRUE(verifier.VerifyBuffer<schema::Response>("OTVS"));
  return *flatbuffers::GetRoot<schema::Response>(bytes.data());
}

constexpr DeviceId kDevice{Uuid{1, 1}};

ElementId add_level(Session& session) {
  const auto& response = respond(session, Request{}.add_level(0, 3000).bytes());
  return to_id(response.results()->Get(0)->created());
}

ElementId add_wall(Session& session, ElementId level) {
  const auto& response = respond(session, Request{}.add_wall(level, 6400).bytes());
  return to_id(response.results()->Get(0)->created());
}

TEST(Session, AddedElementsComeBackWithTheirIdsAndValues) {
  Session session{kDevice};

  const auto& first = respond(session, Request{}.add_level(-450, 3000).bytes());
  ASSERT_EQ(first.results()->size(), 1U);
  const auto* added = first.results()->Get(0);
  EXPECT_EQ(added->error(), schema::ErrorCode::None);
  const ElementId level = to_id(added->created());
  EXPECT_EQ(level.uuid().version(), 7U);
  ASSERT_EQ(first.changes()->size(), 1U);
  const auto* change = first.changes()->Get(0);
  EXPECT_EQ(to_id(change->id()), level);
  ASSERT_EQ(change->element_type(), schema::Element::Level);
  EXPECT_EQ(change->element_as_Level()->elevation(), -450);
  EXPECT_EQ(change->element_as_Level()->height(), 3000);
  EXPECT_TRUE(first.can_undo());

  const auto& second = respond(session, Request{}.add_wall(level, 6400).bytes());
  const ElementId wall = to_id(second.results()->Get(0)->created());
  ASSERT_EQ(second.changes()->size(), 1U);
  const auto* wall_change = second.changes()->Get(0)->element_as_Wall();
  ASSERT_NE(wall_change, nullptr);
  EXPECT_EQ(to_id(second.changes()->Get(0)->id()), wall);
  EXPECT_EQ(to_id(wall_change->level()), level);
  EXPECT_EQ(wall_change->end()->x(), 6400);
  EXPECT_EQ(wall_change->thickness(), 380);
}

TEST(Session, EachCommandGetsItsOwnResult) {
  Session session{kDevice};
  const ElementId missing{Uuid{7, 7}};

  const auto& response = respond(
      session, Request{}.set_height(missing, 3000).add_level(0, 0).add_level(0, 3000).bytes());

  ASSERT_EQ(response.results()->size(), 3U);
  EXPECT_EQ(response.results()->Get(0)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(response.results()->Get(1)->error(), schema::ErrorCode::HeightNotPositive);
  EXPECT_NE(response.results()->Get(1)->message(), nullptr);
  EXPECT_EQ(response.results()->Get(2)->error(), schema::ErrorCode::None);
  EXPECT_EQ(response.changes()->size(), 1U);
}

TEST(Session, CorruptRequestChangesNothing) {
  Session session{kDevice};
  const std::vector<std::uint8_t> garbage{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  std::vector<std::uint8_t> wrong_identifier = Request{}.add_level(0, 3000).bytes();
  wrong_identifier[4] = 'X';

  for (const auto& bytes : {garbage, wrong_identifier, std::vector<std::uint8_t>{}}) {
    const auto& response = respond(session, bytes);
    ASSERT_EQ(response.results()->size(), 1U);
    EXPECT_EQ(response.results()->Get(0)->error(), schema::ErrorCode::CorruptData);
    EXPECT_EQ(response.changes()->size(), 0U);
    EXPECT_FALSE(response.can_undo());
  }
}

TEST(Session, NewerRequestVersionIsRefused) {
  Session session{kDevice};

  const auto& response = respond(session, Request{}.add_level(0, 3000).bytes(2));

  ASSERT_EQ(response.results()->size(), 1U);
  EXPECT_EQ(response.results()->Get(0)->error(), schema::ErrorCode::UnsupportedVersion);
  EXPECT_EQ(response.changes()->size(), 0U);
}

TEST(Session, UnknownEnumValueOrMissingTableIsCorrupt) {
  Session session{kDevice};
  const ElementId wall = add_wall(session, add_level(session));

  // An app built against a newer schema may send an end this engine does not know.
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  const auto unknown_end = static_cast<schema::WallEnd>(7);
  const auto& response = respond(session, Request{}
                                              .move_end(wall, 7000, unknown_end)
                                              .empty(Command::AddWall)
                                              .empty(Command::Undo)
                                              .bytes());

  ASSERT_EQ(response.results()->size(), 3U);
  EXPECT_EQ(response.results()->Get(0)->error(), schema::ErrorCode::CorruptData);
  EXPECT_EQ(response.results()->Get(1)->error(), schema::ErrorCode::CorruptData);
  // Gesture and undo entries carry no data, so an absent table is still a valid undo.
  EXPECT_EQ(response.results()->Get(2)->error(), schema::ErrorCode::None);
}

TEST(Session, UndoReportsDeletionAndRedoBringsItBack) {
  Session session{kDevice};
  const ElementId level = add_level(session);

  const auto& undone = respond(session, Request{}.undo().bytes());
  ASSERT_EQ(undone.changes()->size(), 1U);
  EXPECT_EQ(to_id(undone.changes()->Get(0)->id()), level);
  EXPECT_EQ(undone.changes()->Get(0)->element_type(), schema::Element::NONE);
  EXPECT_FALSE(undone.can_undo());
  EXPECT_TRUE(undone.can_redo());

  const auto& redone = respond(session, Request{}.redo().bytes());
  ASSERT_EQ(redone.changes()->size(), 1U);
  EXPECT_EQ(redone.changes()->Get(0)->element_type(), schema::Element::Level);
  EXPECT_EQ(respond(session, Request{}.redo().bytes()).results()->Get(0)->error(),
            schema::ErrorCode::NothingToRedo);
}

TEST(Session, DragInOneGestureIsOneUndoStep) {
  Session session{kDevice};
  const ElementId wall = add_wall(session, add_level(session));

  Request drag;
  drag.begin_gesture();
  for (std::int64_t x = 6500; x <= 7000; x += 100) {
    drag.move_end(wall, x);
  }
  drag.end_gesture();
  const auto& moved = respond(session, drag.bytes());
  ASSERT_EQ(moved.results()->size(), 8U);
  EXPECT_EQ(moved.changes()->Get(0)->element_as_Wall()->end()->x(), 7000);

  const auto& undone = respond(session, Request{}.undo().bytes());
  EXPECT_EQ(undone.changes()->Get(0)->element_as_Wall()->end()->x(), 6400);
}

TEST(Session, RemovingAWallReportsItsOpeningsGone) {
  Session session{kDevice};
  const ElementId wall = add_wall(session, add_level(session));
  const auto& added = respond(session, Request{}.add_window(wall).bytes());
  const ElementId window = to_id(added.results()->Get(0)->created());
  EXPECT_EQ(added.changes()->Get(0)->element_as_Opening()->width(), 1500);

  const auto& removed = respond(session, Request{}.remove_wall(wall).bytes());

  ASSERT_EQ(removed.changes()->size(), 2U);
  for (const schema::ElementChange* change : *removed.changes()) {
    EXPECT_EQ(change->element_type(), schema::Element::NONE);
    const ElementId id = to_id(change->id());
    EXPECT_TRUE(id == wall || id == window);
  }
}

TEST(Session, PlacedOpeningIsCentredOnTheTapAndOutlined) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const ElementId wall = add_wall(session, level);

  const auto& placed = respond(session, Request{}.place(level, {3000, 250}).bytes());

  ASSERT_EQ(placed.results()->Get(0)->error(), schema::ErrorCode::None);
  const ElementId window = to_id(placed.results()->Get(0)->created());
  ASSERT_EQ(placed.changes()->size(), 1U);
  const auto* opening = placed.changes()->Get(0)->element_as_Opening();
  ASSERT_NE(opening, nullptr);
  EXPECT_EQ(to_id(opening->wall()), wall);
  EXPECT_EQ(opening->offset(), 2250);
  EXPECT_EQ(opening->sill(), 800);
  ASSERT_NE(placed.openings(), nullptr);
  ASSERT_EQ(placed.openings()->size(), 1U);
  const auto* outline = placed.openings()->Get(0);
  EXPECT_EQ(to_id(outline->opening()), window);
  EXPECT_EQ(to_id(outline->level()), level);
  EXPECT_EQ(outline->kind(), schema::OpeningKind::Window);
  ASSERT_EQ(outline->points()->size(), 4U);
  EXPECT_DOUBLE_EQ(outline->points()->Get(0)->x(), 2.25);
  EXPECT_DOUBLE_EQ(outline->points()->Get(0)->y(), -0.19);
  EXPECT_DOUBLE_EQ(outline->points()->Get(2)->x(), 3.75);

  const auto& undone = respond(session, Request{}.undo().bytes());
  EXPECT_EQ(undone.changes()->Get(0)->element_type(), schema::Element::NONE);
  EXPECT_EQ(undone.openings()->size(), 0U);
}

TEST(Session, PlacedOpeningMovesAlongTheWallToFit) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_wall(session, level);

  const auto& placed =
      respond(session, Request{}.place(level, {6300, 0}, 900, schema::OpeningKind::Door).bytes());

  EXPECT_EQ(placed.changes()->Get(0)->element_as_Opening()->offset(), 5500);
  EXPECT_EQ(placed.openings()->Get(0)->kind(), schema::OpeningKind::Door);
}

TEST(Session, PlacingAnOpeningAwayFromAWallOrTooWideFails) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_wall(session, level);

  const auto& response = respond(session, Request{}
                                              .place(level, {3000, 600})
                                              .place(level, {3000, 0}, 7000)
                                              .place(ElementId{Uuid{7, 7}}, {3000, 0})
                                              .place(level, {3000, 0}, 0)
                                              .bytes());

  ASSERT_EQ(response.results()->size(), 4U);
  EXPECT_EQ(response.results()->Get(0)->error(), schema::ErrorCode::NoWall);
  EXPECT_EQ(response.results()->Get(1)->error(), schema::ErrorCode::DoesNotFit);
  EXPECT_EQ(response.results()->Get(2)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(response.results()->Get(3)->error(), schema::ErrorCode::WidthNotPositive);
  EXPECT_EQ(response.changes()->size(), 0U);
}

Result<OpGroup> group_of(const schema::LogEntry& entry) {
  return decode_group({entry.ops()->data(), entry.ops()->size()});
}

TEST(Session, ResponsesCarryTheGroupsAddedToTheLog) {
  Session session{kDevice};

  const auto& added = respond(session, Request{}.add_level(0, 3000).bytes());
  ASSERT_NE(added.log(), nullptr);
  ASSERT_EQ(added.log()->size(), 1U);
  const auto* entry = added.log()->Get(0);
  EXPECT_EQ(entry->kind(), schema::CommandKind::AddLevel);
  EXPECT_EQ(to_id(entry->device()).uuid(), kDevice.uuid());
  const Result<OpGroup> group = group_of(*entry);
  ASSERT_TRUE(group.has_value());
  EXPECT_EQ(group->stamp.hlc.value, entry->hlc());
  const std::uint64_t added_at = entry->hlc();  // the next call reuses the response
  ASSERT_EQ(group->ops.size(), 1U);
  EXPECT_TRUE(std::holds_alternative<CreateLevel>(group->ops.front()));

  const auto& undone = respond(session, Request{}.undo().bytes());
  ASSERT_EQ(undone.log()->size(), 1U);
  EXPECT_EQ(undone.log()->Get(0)->kind(), schema::CommandKind::Undo);
  EXPECT_EQ(to_id(undone.log()->Get(0)->origin()).uuid(), group->id.uuid());
  EXPECT_GT(undone.log()->Get(0)->hlc(), added_at);

  const auto& idle = respond(session, Request{}.undo().bytes());
  EXPECT_EQ(idle.log()->size(), 0U);
}

TEST(Session, PickFindsWhatIsUnderTheTapOpeningsFirst) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const ElementId wall = add_wall(session, level);
  const auto& placed = respond(session, Request{}.place(level, {3000, 0}).bytes());
  const ElementId window = to_id(placed.results()->Get(0)->created());

  // The window spans 2 250…3 750 mm; the wall's faces are 190 mm either side of its line.
  const auto& picked = respond(session, Request{}
                                            .pick(level, {3000, 100})
                                            .pick(level, {2100, 100})
                                            .pick(level, {3000, 400})
                                            .pick(level, {3000, 900})
                                            .pick(ElementId{Uuid{7, 7}}, {0, 0})
                                            .bytes());

  ASSERT_EQ(picked.results()->size(), 5U);
  EXPECT_EQ(to_id(picked.results()->Get(0)->picked()), window);
  EXPECT_EQ(to_id(picked.results()->Get(1)->picked()), wall);
  EXPECT_EQ(to_id(picked.results()->Get(2)->picked()), window);
  EXPECT_EQ(picked.results()->Get(3)->picked(), nullptr);
  EXPECT_EQ(picked.results()->Get(4)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(picked.changes()->size(), 0U);
  EXPECT_EQ(picked.log()->size(), 0U);
  EXPECT_EQ(picked.outlines(), nullptr);
}

// The wall's line as the response reports it: start x, start y, end x, end y.
std::vector<std::int64_t> line(const schema::Response& response, ElementId wall) {
  for (const schema::ElementChange* change : *response.changes()) {
    const schema::Wall* value = change->element_as_Wall();
    if (to_id(change->id()) == wall && value != nullptr) {
      return {value->start()->x(), value->start()->y(), value->end()->x(), value->end()->y()};
    }
  }
  return {};
}

using Line = std::vector<std::int64_t>;

TEST(Session, AimAnswersWhereAWallWouldEndAndChangesNothing) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_wall(session, level);
  const schema::PointMm from{6400, 0};

  const auto& aimed = respond(session, Request{}
                                           .aim(level, {6300, 150})
                                           .aim(level, {6480, 3960}, &from)
                                           .aim(level, {3040, 260})
                                           .aim(ElementId{Uuid{7, 7}}, {0, 0})
                                           .empty(Command::Aim)
                                           .bytes());

  ASSERT_EQ(aimed.results()->size(), 5U);
  // A first point within reach of a wall end is that end.
  const schema::Aimed* end = aimed.results()->Get(0)->aimed();
  ASSERT_NE(end, nullptr);
  EXPECT_EQ(end->point()->x(), 6400);
  EXPECT_EQ(end->point()->y(), 0);
  EXPECT_EQ(end->length(), 0);
  EXPECT_TRUE(end->meets());
  // A wall from that end turns onto the axis and stops on the grid.
  const schema::Aimed* up = aimed.results()->Get(1)->aimed();
  ASSERT_NE(up, nullptr);
  EXPECT_EQ(up->point()->x(), 6400);
  EXPECT_EQ(up->point()->y(), 4000);
  EXPECT_EQ(up->length(), 4000);
  EXPECT_FALSE(up->meets());
  // A first point beside the wall moves onto its line.
  const schema::Aimed* on_wall = aimed.results()->Get(2)->aimed();
  ASSERT_NE(on_wall, nullptr);
  EXPECT_EQ(on_wall->point()->x(), 3000);
  EXPECT_EQ(on_wall->point()->y(), 0);
  EXPECT_TRUE(on_wall->meets());
  EXPECT_EQ(aimed.results()->Get(3)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(aimed.results()->Get(3)->aimed(), nullptr);
  EXPECT_EQ(aimed.results()->Get(4)->error(), schema::ErrorCode::CorruptData);
  EXPECT_EQ(aimed.changes()->size(), 0U);
  EXPECT_EQ(aimed.log()->size(), 0U);
  EXPECT_EQ(aimed.outlines(), nullptr);
}

TEST(Session, AimCatchesTheWallEndsOfTheStoreyBelow) {
  Session session{kDevice};
  const ElementId ground = add_level(session);
  add_wall(session, ground);
  const auto& added =
      respond(session, Request{}.add_level(3000, 3000).add_level(-3000, 2500).bytes());
  const ElementId upper = to_id(added.results()->Get(0)->created());
  const ElementId cellar = to_id(added.results()->Get(1)->created());

  const auto& aimed =
      respond(session, Request{}.aim(upper, {6250, 180}).aim(cellar, {6250, 180}).bytes());

  // The upper storey stands on the ground floor's walls; the cellar has nothing under it.
  const schema::Aimed* above = aimed.results()->Get(0)->aimed();
  ASSERT_NE(above, nullptr);
  EXPECT_EQ(above->point()->x(), 6400);
  EXPECT_EQ(above->point()->y(), 0);
  EXPECT_FALSE(above->meets());
  const schema::Aimed* under = aimed.results()->Get(1)->aimed();
  ASSERT_NE(under, nullptr);
  EXPECT_EQ(under->point()->x(), 6300);
  EXPECT_EQ(under->point()->y(), 200);
}

TEST(Session, AimMeasuredTurnsFromTheWallBefore) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_wall(session, level);

  const auto& aimed =
      respond(session, Request{}
                           .aim_measured(level, {6400, 0}, {0, 0}, schema::Turn::Left, 4000)
                           .aim_measured(level, {6400, 0}, {0, 0}, schema::Turn::Right, 4000)
                           .aim_measured(level, {0, 4000}, {6400, 4000}, schema::Turn::Left, 4000)
                           .bytes());

  ASSERT_EQ(aimed.results()->size(), 3U);
  EXPECT_EQ(aimed.results()->Get(0)->aimed()->point()->y(), 4000);
  EXPECT_EQ(aimed.results()->Get(0)->aimed()->length(), 4000);
  EXPECT_EQ(aimed.results()->Get(1)->aimed()->point()->y(), -4000);
  // A run that comes back to where a wall starts closes on it.
  const schema::Aimed* closing = aimed.results()->Get(2)->aimed();
  EXPECT_EQ(closing->point()->x(), 0);
  EXPECT_EQ(closing->point()->y(), 0);
  EXPECT_TRUE(closing->meets());
  EXPECT_EQ(aimed.changes()->size(), 0U);
}

// A 6 000 × 4 000 room drawn round from the origin: bottom, right, top, left.
std::vector<ElementId> add_room(Session& session, ElementId level) {
  const auto& added = respond(session, Request{}
                                           .add_wall(level, {0, 0}, {6000, 0})
                                           .add_wall(level, {6000, 0}, {6000, 4000})
                                           .add_wall(level, {6000, 4000}, {0, 4000})
                                           .add_wall(level, {0, 4000}, {0, 0})
                                           .bytes());
  std::vector<ElementId> walls;
  for (const schema::CommandResult* result : *added.results()) {
    walls.push_back(to_id(result->created()));
  }
  return walls;
}

TEST(Session, ATypedLengthKeepsTheRoomSquareInOneUndoStep) {
  Session session{kDevice};
  const std::vector<ElementId> room = add_room(session, add_level(session));

  const auto& longer = respond(session, Request{}.set_length(room[0], 6400).bytes());

  ASSERT_EQ(longer.results()->Get(0)->error(), schema::ErrorCode::None);
  EXPECT_EQ(longer.changes()->size(), 3U);
  EXPECT_EQ(line(longer, room[0]), (Line{0, 0, 6400, 0}));
  EXPECT_EQ(line(longer, room[1]), (Line{6400, 0, 6400, 4000}));
  EXPECT_EQ(line(longer, room[2]), (Line{6400, 4000, 0, 4000}));
  ASSERT_EQ(longer.log()->size(), 1U);
  EXPECT_EQ(longer.log()->Get(0)->kind(), schema::CommandKind::MoveWallEnd);

  const auto& undone = respond(session, Request{}.undo().bytes());
  EXPECT_EQ(line(undone, room[0]), (Line{0, 0, 6000, 0}));
  EXPECT_EQ(line(undone, room[1]), (Line{6000, 0, 6000, 4000}));
  EXPECT_EQ(line(undone, room[2]), (Line{6000, 4000, 0, 4000}));
  EXPECT_FALSE(respond(session, Request{}.undo().undo().undo().undo().undo().bytes()).can_undo());
}

TEST(Session, APartitionGrowsFromTheWallItStandsAgainst) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_room(session, level);
  // Drawn from inside the room down onto the bottom wall.
  const auto& added = respond(session, Request{}.add_wall(level, {3000, 3000}, {3000, 0}).bytes());
  const ElementId partition = to_id(added.results()->Get(0)->created());

  const auto& longer = respond(session, Request{}.set_length(partition, 3500).bytes());

  ASSERT_EQ(longer.results()->Get(0)->error(), schema::ErrorCode::None);
  ASSERT_EQ(longer.changes()->size(), 1U);
  EXPECT_EQ(line(longer, partition), (Line{3000, 3500, 3000, 0}));

  // Named, the other end stays instead and the wall it stands against goes with it.
  const auto& pushed =
      respond(session, Request{}.set_length(partition, 4000, schema::FixedEnd::Start).bytes());
  EXPECT_EQ(line(pushed, partition), (Line{3000, 3500, 3000, -500}));
  EXPECT_EQ(pushed.changes()->size(), 4U);
}

TEST(Session, ATypedLengthJoinsTheGestureAroundIt) {
  Session session{kDevice};
  const std::vector<ElementId> room = add_room(session, add_level(session));

  const auto& sized = respond(session, Request{}
                                           .begin_gesture()
                                           .set_length(room[0], 6400)
                                           .set_length(room[1], 5000)
                                           .end_gesture()
                                           .bytes());

  EXPECT_EQ(line(sized, room[2]), (Line{6400, 5000, 0, 5000}));
  ASSERT_EQ(sized.log()->size(), 1U);
  const auto& undone = respond(session, Request{}.undo().bytes());
  EXPECT_EQ(line(undone, room[2]), (Line{6000, 4000, 0, 4000}));
}

TEST(Session, ATypedLengthThatBreaksAWallChangesNothing) {
  Session session{kDevice};
  const std::vector<ElementId> room = add_room(session, add_level(session));

  const auto& refused = respond(session, Request{}
                                             .set_length(room[0], 0)
                                             .set_length(ElementId{Uuid{7, 7}}, 6400)
                                             .set_length(room[0], 6000)
                                             .empty(Command::SetWallLength)
                                             .bytes());

  ASSERT_EQ(refused.results()->size(), 4U);
  EXPECT_EQ(refused.results()->Get(0)->error(), schema::ErrorCode::ZeroLength);
  EXPECT_EQ(refused.results()->Get(1)->error(), schema::ErrorCode::NotFound);
  // The length it already has is not a change.
  EXPECT_EQ(refused.results()->Get(2)->error(), schema::ErrorCode::None);
  EXPECT_EQ(refused.results()->Get(3)->error(), schema::ErrorCode::CorruptData);
  EXPECT_EQ(refused.changes()->size(), 0U);
  EXPECT_EQ(refused.log()->size(), 0U);
}

TEST(Session, AWallCarriedAlongItsOwnLineByItsOwnLengthArrives) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  // A wall with a post, a brace from the post's top and a tie from the brace back to the post:
  // the tie lies along the move and is as long as the move.
  const auto& added = respond(session, Request{}
                                           .add_wall(level, {0, 0}, {6000, 0})
                                           .add_wall(level, {6000, 0}, {6000, 4000})
                                           .add_wall(level, {6000, 4000}, {7000, 2000})
                                           .add_wall(level, {6000, 2000}, {7000, 2000})
                                           .bytes());
  const ElementId wall = to_id(added.results()->Get(0)->created());
  const ElementId tie = to_id(added.results()->Get(3)->created());

  const auto& longer =
      respond(session, Request{}.set_length(wall, 7000, schema::FixedEnd::Start).bytes());

  ASSERT_EQ(longer.results()->Get(0)->error(), schema::ErrorCode::None);
  EXPECT_EQ(line(longer, tie), (Line{7000, 2000, 8000, 2000}));
  EXPECT_EQ(longer.log()->size(), 1U);
}

std::vector<ElementId> deleted(const schema::Response& response) {
  std::vector<ElementId> ids;
  for (const schema::ElementChange* change : *response.changes()) {
    if (change->element_type() == schema::Element::NONE) {
      ids.push_back(to_id(change->id()));
    }
  }
  return ids;
}

// The walls a response reports that are not exact, each as its line, sorted.
std::vector<Line> rough_walls(const schema::Response& response) {
  std::vector<Line> lines;
  for (const schema::ElementChange* change : *response.changes()) {
    const schema::Wall* wall = change->element_as_Wall();
    if (wall != nullptr && !wall->exact()) {
      lines.push_back(line(response, to_id(change->id())));
    }
  }
  std::ranges::sort(lines);
  return lines;
}

TEST(Session, ASketchBecomesWallsInOneUndoStep) {
  Session session{kDevice};
  const ElementId level = add_level(session);

  // A room drawn roughly: no corner on the grid, the loop not quite closed.
  const auto& drawn = respond(session, Request{}
                                           .add_sketch(level, {{40, -30},
                                                               {3000, 20},
                                                               {5960, 50},
                                                               {6030, 2000},
                                                               {5980, 4040},
                                                               {3000, 3970},
                                                               {-20, 3960},
                                                               {30, 2000},
                                                               {10, 60}})
                                           .bytes());

  ASSERT_EQ(drawn.results()->Get(0)->error(), schema::ErrorCode::None);
  ASSERT_EQ(drawn.changes()->size(), 4U);
  EXPECT_EQ(rough_walls(drawn),
            (std::vector<Line>{
                {0, 0, 6000, 0}, {0, 4000, 0, 0}, {6000, 0, 6000, 4000}, {6000, 4000, 0, 4000}}));
  EXPECT_EQ(drawn.outlines()->size(), 4U);
  ASSERT_EQ(drawn.log()->size(), 1U);
  EXPECT_EQ(drawn.log()->Get(0)->kind(), schema::CommandKind::AddWall);

  const auto& undone = respond(session, Request{}.undo().bytes());
  EXPECT_EQ(deleted(undone).size(), 4U);
}

TEST(Session, ATypedLengthMakesASketchedWallExact) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const auto& drawn =
      respond(session, Request{}.add_sketch(level, {{0, 0}, {3000, 40}, {6040, -20}}).bytes());
  ASSERT_EQ(drawn.changes()->size(), 1U);
  const ElementId wall = to_id(drawn.changes()->Get(0)->id());
  EXPECT_EQ(line(drawn, wall), (Line{0, 0, 6000, 0}));
  EXPECT_FALSE(drawn.changes()->Get(0)->element_as_Wall()->exact());

  // The length it has, typed: nothing moves, and the wall is measured now.
  const auto& kept = respond(session, Request{}.set_length(wall, 6000).bytes());
  ASSERT_EQ(kept.changes()->size(), 1U);
  EXPECT_TRUE(kept.changes()->Get(0)->element_as_Wall()->exact());
  EXPECT_EQ(line(kept, wall), (Line{0, 0, 6000, 0}));
  ASSERT_EQ(kept.log()->size(), 1U);
  EXPECT_EQ(kept.log()->Get(0)->kind(), schema::CommandKind::SetWallExact);

  const auto& undone = respond(session, Request{}.undo().bytes());
  ASSERT_EQ(undone.changes()->size(), 1U);
  EXPECT_FALSE(undone.changes()->Get(0)->element_as_Wall()->exact());

  // Another length moves the end and makes the wall exact in the same step.
  const auto& longer = respond(session, Request{}.set_length(wall, 6400).bytes());
  ASSERT_EQ(longer.changes()->size(), 1U);
  EXPECT_TRUE(longer.changes()->Get(0)->element_as_Wall()->exact());
  EXPECT_EQ(line(longer, wall), (Line{0, 0, 6400, 0}));
  ASSERT_EQ(longer.log()->size(), 1U);
  EXPECT_EQ(longer.log()->Get(0)->kind(), schema::CommandKind::MoveWallEnd);
}

TEST(Session, ASketchThatGivesNoWallChangesNothing) {
  Session session{kDevice};
  const ElementId level = add_level(session);

  const auto& refused = respond(session, Request{}
                                             .add_sketch(level, {{100, 100}, {180, 60}})
                                             .add_sketch(ElementId{Uuid{7, 7}}, {{0, 0}, {6000, 0}})
                                             .add_sketch(level, {{0, 0}, {6000, 0}}, 0)
                                             .add_sketch(level, {{0, 0}, {200'000'000, 0}})
                                             .add_sketch(level, {})
                                             .empty(Command::AddSketch)
                                             .bytes());

  ASSERT_EQ(refused.results()->size(), 6U);
  // A tap, a level that is not there, a wall without thickness, a point 200 km away, no stroke.
  EXPECT_EQ(refused.results()->Get(0)->error(), schema::ErrorCode::ZeroLength);
  EXPECT_EQ(refused.results()->Get(1)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(refused.results()->Get(2)->error(), schema::ErrorCode::ThicknessNotPositive);
  EXPECT_EQ(refused.results()->Get(3)->error(), schema::ErrorCode::OffPlan);
  EXPECT_EQ(refused.results()->Get(4)->error(), schema::ErrorCode::ZeroLength);
  EXPECT_EQ(refused.results()->Get(5)->error(), schema::ErrorCode::CorruptData);
  EXPECT_EQ(refused.changes()->size(), 0U);
  EXPECT_EQ(refused.log()->size(), 0U);
}

TEST(Session, ADrawingIsTheLevelAsADisplayListAndChangesNothing) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const std::vector<ElementId> room = add_room(session, level);
  respond(session, Request{}.place(level, {3000, 0}).bytes());

  const auto& asked = respond(
      session, Request{}.draw(level).draw(ElementId{Uuid{7, 7}}).empty(Command::Draw).bytes());

  ASSERT_EQ(asked.results()->size(), 3U);
  const schema::Drawing* drawing = asked.results()->Get(0)->drawing();
  ASSERT_NE(drawing, nullptr);
  // The floor the walls close; four walls with no material yet and the window's gap; its
  // jambs, faces and glazing; the floor's area and the plan's size across and up, each a line
  // with two ticks and two lines out to it.
  ASSERT_EQ(drawing->polygons()->size(), 6U);
  EXPECT_EQ(drawing->polygons()->Get(0)->fill(), schema::Fill::Floor);
  EXPECT_EQ(drawing->polygons()->Get(0)->element(), nullptr);
  EXPECT_EQ(to_id(drawing->polygons()->Get(1)->element()), room[0]);
  EXPECT_EQ(drawing->polygons()->Get(1)->fill(), schema::Fill::Unchosen);
  EXPECT_EQ(drawing->polygons()->Get(1)->stroke(), schema::Stroke::WallEdge);
  EXPECT_EQ(drawing->polygons()->Get(5)->fill(), schema::Fill::Gap);
  EXPECT_EQ(drawing->lines()->size(), 15U);
  EXPECT_EQ(drawing->arcs()->size(), 0U);
  // The walls were drawn to their lengths and close a space, so its sizes speak for them. The
  // window has the first mark of its kind.
  ASSERT_EQ(drawing->texts()->size(), 4U);
  EXPECT_EQ(drawing->texts()->Get(0)->kind(), schema::TextKind::RoomArea);
  EXPECT_EQ(drawing->texts()->Get(1)->kind(), schema::TextKind::WindowMark);
  EXPECT_EQ(drawing->texts()->Get(1)->value(), 1);
  EXPECT_EQ(drawing->texts()->Get(2)->kind(), schema::TextKind::Overall);
  EXPECT_EQ(drawing->texts()->Get(2)->element(), nullptr);
  EXPECT_EQ(drawing->texts()->Get(2)->value(), 6380);
  EXPECT_DOUBLE_EQ(drawing->texts()->Get(2)->at()->x(), 3.0);
  EXPECT_DOUBLE_EQ(drawing->texts()->Get(2)->along()->x(), 1.0);
  EXPECT_EQ(drawing->texts()->Get(3)->value(), 4380);

  EXPECT_EQ(asked.results()->Get(1)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(asked.results()->Get(2)->error(), schema::ErrorCode::CorruptData);
  EXPECT_EQ(asked.changes()->size(), 0U);
  EXPECT_EQ(asked.log()->size(), 0U);
}

TEST(Session, ADrawingSeesWhatTheCommandsBeforeItInTheRequestDid) {
  Session session{kDevice};
  const ElementId level = add_level(session);

  const auto& built = respond(session, Request{}.add_wall(level, 6400).draw(level).bytes());

  ASSERT_EQ(built.results()->size(), 2U);
  const schema::Drawing* drawing = built.results()->Get(1)->drawing();
  ASSERT_NE(drawing, nullptr);
  ASSERT_EQ(drawing->polygons()->size(), 1U);
  EXPECT_EQ(to_id(drawing->polygons()->Get(0)->element()),
            to_id(built.results()->Get(0)->created()));
  ASSERT_EQ(drawing->texts()->size(), 1U);
  EXPECT_EQ(drawing->texts()->Get(0)->value(), 6400);
}

TEST(Session, TheStoreyBelowIsDrawnInOutlineUnderTheWalls) {
  Session session{kDevice};
  const ElementId ground = add_level(session);
  add_room(session, ground);
  const ElementId upper =
      to_id(respond(session, Request{}.add_level(3000, 3000).bytes()).results()->Get(0)->created());

  const auto& asked =
      respond(session, Request{}.add_wall(upper, {0, 0}, {6000, 0}).draw(upper).bytes());

  const schema::Drawing* drawing = asked.results()->Get(1)->drawing();
  ASSERT_NE(drawing, nullptr);
  ASSERT_EQ(drawing->polygons()->size(), 5U);
  // The four walls under it carry no element: they cannot be picked from up here.
  EXPECT_EQ(drawing->polygons()->Get(0)->stroke(), schema::Stroke::Below);
  EXPECT_EQ(drawing->polygons()->Get(0)->element(), nullptr);
  EXPECT_EQ(drawing->polygons()->Get(4)->stroke(), schema::Stroke::WallEdge);
  EXPECT_EQ(drawing->texts()->size(), 1U);
}

TEST(Session, OpeningASavedJobReplacesTheModel) {
  Session session{kDevice};
  const ElementId old_level = add_level(session);

  const auto& opened = respond(session, Request{}.open_model(0).bytes());

  ASSERT_EQ(opened.results()->Get(0)->error(), schema::ErrorCode::None);
  ASSERT_EQ(opened.changes()->size(), 4U);  // the old level gone and three saved elements
  EXPECT_EQ(deleted(opened), std::vector{old_level});
  EXPECT_EQ(opened.outlines()->size(), 1U);
  EXPECT_EQ(opened.openings()->size(), 1U);
  EXPECT_FALSE(opened.can_undo());
  EXPECT_EQ(opened.log()->size(), 0U);
}

TEST(Session, EditsAfterOpeningStampAfterTheSavedClock) {
  Session session{kDevice};
  const std::uint64_t saved_clock = 0x0100'0000'0000'0000ULL;
  respond(session, Request{}.open_model(saved_clock).bytes());

  const auto& added = respond(session, Request{}.add_wall(ElementId{Uuid{10, 1}}, 3000).bytes());

  ASSERT_EQ(added.results()->Get(0)->error(), schema::ErrorCode::None);
  ASSERT_EQ(added.log()->size(), 1U);
  EXPECT_GT(added.log()->Get(0)->hlc(), saved_clock);
}

TEST(Session, GroupsBeforeAnOpenInTheSameRequestAreStillReported) {
  Session session{kDevice};

  const auto& response =
      respond(session, Request{}.add_level(0, 3000).open_model(0, Request::Window::kNone).bytes());

  ASSERT_EQ(response.log()->size(), 1U);
  EXPECT_EQ(response.log()->Get(0)->kind(), schema::CommandKind::AddLevel);
}

TEST(Session, BrokenSavedJobChangesNothing) {
  Session session{kDevice};
  const ElementId level = add_level(session);

  const auto& response =
      respond(session, Request{}.open_model(0, Request::Window::kOrphan).bytes());

  EXPECT_EQ(response.results()->Get(0)->error(), schema::ErrorCode::CorruptData);
  EXPECT_EQ(response.changes()->size(), 0U);
  EXPECT_TRUE(response.can_undo());
  EXPECT_EQ(to_id(respond(session, Request{}.undo().bytes()).changes()->Get(0)->id()), level);
}

std::pair<std::string, std::string> content_file(std::string_view folder, std::string_view name) {
  std::ifstream in{std::filesystem::path{OTVES_CONTENT_DIR} / folder /
                   (std::string{name} + ".luau")};
  EXPECT_TRUE(in.good()) << name;
  return {std::string{name},
          std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}}};
}

// Everything in content/, as the app sends it from its assets.
std::vector<std::uint8_t> load_all_content() {
  Request::Files norms;
  for (const std::string_view name :
       {"losses", "masonry", "mortar", "practice", "rebar", "reinforcement", "seismic"}) {
    norms.push_back(content_file("norms", name));
  }
  Request::Files rules;
  for (const std::string_view name :
       {"footing", "insulation", "junction_meshes", "lintels", "masonry", "ring_beam"}) {
    rules.push_back(content_file("rules", name));
  }
  return Request{}.load_content(norms, rules).bytes();
}

double amount(const schema::Response& response, std::string_view item) {
  for (const schema::QuantityLine* line : *response.takeoff()) {
    if (line->item()->string_view() == item) {
      return line->amount();
    }
  }
  ADD_FAILURE() << "no line " << item;
  return 0.0;
}

TEST(Session, OutlinesFollowTheModelEvenWithoutContent) {
  Session session{kDevice};
  const ElementId level = add_level(session);

  const auto& built = respond(session, Request{}.add_wall(level, 6400).bytes());

  ASSERT_NE(built.outlines(), nullptr);
  ASSERT_EQ(built.outlines()->size(), 1U);
  const auto* outline = built.outlines()->Get(0);
  EXPECT_EQ(to_id(outline->level()), level);
  // A free 6.4 m wall, 380 mm thick: a rectangle around its centre line, in metres.
  ASSERT_EQ(outline->points()->size(), 4U);
  EXPECT_DOUBLE_EQ(outline->points()->Get(1)->x(), 6.4);
  EXPECT_EQ(outline->length(), 6400);
  EXPECT_EQ(built.takeoff(), nullptr);

  // Nothing changed, so nothing derived is sent.
  const auto& idle = respond(session, Request{}.set_height(ElementId{Uuid{7, 7}}, 3000).bytes());
  EXPECT_EQ(idle.outlines(), nullptr);
}

TEST(Session, TakeoffRunsTheLoadedContent) {
  Session session{kDevice};
  const auto& loaded = respond(session, load_all_content());
  ASSERT_EQ(loaded.results()->Get(0)->error(), schema::ErrorCode::None);
  ASSERT_NE(loaded.takeoff(), nullptr);
  EXPECT_EQ(loaded.takeoff_error(), schema::ErrorCode::None);

  const ElementId level = add_level(session);
  const auto& drawn = respond(session, Request{}.add_wall(level, 6400).bytes());
  const ElementId wall = to_id(drawn.results()->Get(0)->created());

  // Nothing was chosen yet, so nothing is counted, and the wall asks for its material.
  ASSERT_NE(drawn.takeoff(), nullptr);
  EXPECT_EQ(drawn.takeoff_error(), schema::ErrorCode::None);
  EXPECT_EQ(drawn.takeoff()->size(), 0U);
  // At 9 points the norm also asks for a ring beam on its storey.
  ASSERT_EQ(drawn.issues()->size(), 2U);
  EXPECT_EQ(drawn.issues()->Get(0)->kind(), schema::IssueKind::MaterialNotChosen);
  EXPECT_EQ(to_id(drawn.issues()->Get(0)->element()), wall);
  EXPECT_EQ(drawn.issues()->Get(1)->kind(), schema::IssueKind::RingBeamMissing);
  EXPECT_EQ(drawn.issues()->Get(1)->element(), nullptr);
  EXPECT_EQ(to_id(drawn.issues()->Get(1)->level()), level);

  const auto& built = respond(session, Request{}.set_unit(wall).set_ring_beam(level, 150).bytes());
  ASSERT_NE(built.takeoff(), nullptr);
  EXPECT_EQ(built.takeoff_error(), schema::ErrorCode::None);
  EXPECT_EQ(built.issues()->size(), 0U);
  // 6.4 m × 2.8 m of 380 mm brick: about 2 686 units net, plus 3 % waste.
  EXPECT_GT(amount(built, "masonry_unit.250x120x65"), 2686);
  EXPECT_GT(amount(built, "concrete.b15"), 0.0);
  const auto* changed = built.changes();
  ASSERT_EQ(changed->size(), 2U);

  const auto& undone = respond(session, Request{}.undo().bytes());
  // The ring beam is gone; the brick stays with its mortar, cement, bags and sand.
  EXPECT_EQ(undone.takeoff()->size(), 5U);
}

TEST(Session, ChoicesAreLoggedAndUndone) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const ElementId wall = add_wall(session, level);

  const auto& set = respond(session, Request{}.set_unit(wall).set_meshes(level, true).bytes());
  ASSERT_EQ(set.log()->size(), 2U);
  EXPECT_EQ(set.log()->Get(0)->kind(), schema::CommandKind::SetWallUnit);
  EXPECT_EQ(set.log()->Get(1)->kind(), schema::CommandKind::SetLevelJunctionMeshes);
  const auto unit = group_of(*set.log()->Get(0));
  ASSERT_TRUE(unit.has_value()) << unit.error().message;
  EXPECT_EQ(unit->ops.size(), 3U);
  const auto meshes = group_of(*set.log()->Get(1));
  ASSERT_TRUE(meshes.has_value()) << meshes.error().message;
  EXPECT_EQ(std::get<SetLevelField>(meshes->ops.front()).after, LevelValue{true});

  const auto& undone = respond(session, Request{}.undo().undo().bytes());
  bool cleared = false;
  for (const schema::ElementChange* change : *undone.changes()) {
    if (const auto* value = change->element_as_Wall()) {
      cleared = value->unit_length() == 0;
    }
  }
  EXPECT_TRUE(cleared);
}

TEST(Session, NormsReportTheMeshesACornerLacks) {
  Session session{kDevice};
  ASSERT_EQ(respond(session, load_all_content()).results()->Get(0)->error(),
            schema::ErrorCode::None);
  const ElementId level = add_level(session);
  respond(session, Request{}.set_ring_beam(level, 150).bytes());
  const ElementId first = add_wall(session, level);
  const auto& corner = respond(session, Request{}.add_wall_up(level, 4000).bytes());
  const ElementId second = to_id(corner.results()->Get(0)->created());

  const auto& chosen = respond(session, Request{}.set_unit(first).set_unit(second).bytes());
  ASSERT_EQ(chosen.issues()->size(), 1U);
  EXPECT_EQ(chosen.issues()->Get(0)->kind(), schema::IssueKind::JunctionMeshesMissing);
  EXPECT_EQ(to_id(chosen.issues()->Get(0)->level()), level);

  const auto& meshed = respond(session, Request{}.set_meshes(level, true).bytes());
  EXPECT_EQ(meshed.issues()->size(), 0U);
  EXPECT_GT(amount(meshed, "mesh.junction"), 0.0);
}

TEST(Session, ConditionsChangeTheTakeoffAndTheAdvice) {
  Session session{kDevice};
  ASSERT_EQ(respond(session, load_all_content()).results()->Get(0)->error(),
            schema::ErrorCode::None);
  const ElementId level = add_level(session);
  const auto& built = respond(session, Request{}.set_unit(add_wall(session, level)).bytes());
  // СП 82-101-98: 300 kg of ПЦ400 per m³ of М100 mortar by default.
  const double mortar = amount(built, "mortar");
  EXPECT_NEAR(amount(built, "mortar.cement"), mortar * 300, 1e-6);
  ASSERT_EQ(built.issues()->size(), 1U);  // a ring beam is asked for at 9 points

  using schema::ConditionsField;
  const auto& set = respond(session, Request{}
                                         .set_condition(ConditionsField::CementGrade, 500)
                                         .set_condition(ConditionsField::MortarGrade, 150)
                                         .set_condition(ConditionsField::MortarWaste, 0)
                                         .set_condition(ConditionsField::SeismicIntensity, 6)
                                         .bytes());
  ASSERT_EQ(set.takeoff_error(), schema::ErrorCode::None);
  // 330 kg of ПЦ500 per m³ of М150, and no allowance on the mortar: 7 % less of it.
  EXPECT_NEAR(amount(set, "mortar"), mortar / 1.07, 1e-9);
  EXPECT_NEAR(amount(set, "mortar.cement"), mortar / 1.07 * 330, 1e-6);
  EXPECT_EQ(set.issues()->size(), 0U);  // no seismic detailing below 7 points

  // A grade the norms do not list fails the rule, not the engine.
  const auto& odd =
      respond(session, Request{}.set_condition(ConditionsField::MortarGrade, 333).bytes());
  EXPECT_EQ(odd.takeoff_error(), schema::ErrorCode::RuleFailed);
}

TEST(Session, ConditionsAreReportedAndLoggedAsAnElement) {
  Session session{kDevice};
  using schema::ConditionsField;
  const auto& set = respond(session, Request{}
                                         .set_condition(ConditionsField::CementGrade, 500)
                                         .set_condition(ConditionsField::MortarGrade, 150)
                                         .bytes());
  bool reported = false;
  for (const schema::ElementChange* change : *set.changes()) {
    if (const auto* conditions = change->element_as_Conditions()) {
      reported = conditions->cement_grade() == 500 && conditions->unit_waste() == -1;
    }
  }
  EXPECT_TRUE(reported);
  const auto first = group_of(*set.log()->Get(0));
  ASSERT_TRUE(first.has_value()) << first.error().message;
  EXPECT_TRUE(std::holds_alternative<CreateConditions>(first->ops.front()));
  const auto second = group_of(*set.log()->Get(1));
  ASSERT_TRUE(second.has_value()) << second.error().message;
  EXPECT_TRUE(std::holds_alternative<SetConditionsField>(second->ops.front()));
}

TEST(Session, AStoreysFootingIsCountedLoggedAndUndone) {
  Session session{kDevice};
  ASSERT_EQ(respond(session, load_all_content()).results()->Get(0)->error(),
            schema::ErrorCode::None);
  const ElementId level = add_level(session);
  add_wall(session, level);

  const auto& set = respond(session, Request{}.set_footing(level, 500, 800).bytes());
  ASSERT_EQ(set.takeoff_error(), schema::ErrorCode::None);
  // 6.4 m of footing 0.5 × 0.8 m in B20 with 1.8 % loss, and its formwork on both sides.
  EXPECT_NEAR(amount(set, "concrete.b20"), 6.4 * 0.5 * 0.8 * 1.018, 1e-9);
  EXPECT_NEAR(amount(set, "formwork"), 2 * 6.4 * 0.8, 1e-9);
  EXPECT_EQ(set.changes()->Get(0)->element_as_Level()->footing_depth(), 800);
  ASSERT_EQ(set.log()->size(), 1U);
  EXPECT_EQ(set.log()->Get(0)->kind(), schema::CommandKind::SetLevelFooting);

  const auto& half = respond(session, Request{}.set_footing(level, 500, 0).bytes());
  EXPECT_EQ(half.results()->Get(0)->error(), schema::ErrorCode::HeightNotPositive);

  const auto& undone = respond(session, Request{}.undo().bytes());
  EXPECT_EQ(undone.changes()->Get(0)->element_as_Level()->footing_width(), 0);
}

TEST(Session, BrokenContentKeepsThePreviousContent) {
  Session session{kDevice};
  ASSERT_EQ(respond(session, load_all_content()).results()->Get(0)->error(),
            schema::ErrorCode::None);

  const auto& broken =
      respond(session, Request{}.load_content({}, {{"masonry", "return {"}}).bytes());
  EXPECT_EQ(broken.results()->Get(0)->error(), schema::ErrorCode::RuleSyntax);
  EXPECT_EQ(broken.takeoff(), nullptr);

  const ElementId level = add_level(session);
  const ElementId wall = add_wall(session, level);
  const auto& built = respond(session, Request{}.set_unit(wall).bytes());
  EXPECT_GT(amount(built, "masonry_unit.250x120x65"), 0.0);
}

TEST(Session, IssuesNameTheOpeningsAndJoinsToFix) {
  Session session{kDevice};
  ASSERT_EQ(respond(session, load_all_content()).results()->Get(0)->error(),
            schema::ErrorCode::None);
  const ElementId level = add_level(session);
  respond(session, Request{}.set_unit(add_wall(session, level)).set_ring_beam(level, 150).bytes());

  // A door at the wall's start leaves no masonry for its lintel to rest on; a second wall along
  // the first from the same point overlaps it.
  const auto& placed =
      respond(session, Request{}.place(level, {0, 0}, 900, schema::OpeningKind::Door).bytes());
  const ElementId door = to_id(placed.results()->Get(0)->created());
  ASSERT_NE(placed.issues(), nullptr);
  ASSERT_EQ(placed.issues()->size(), 1U);
  const auto* issue = placed.issues()->Get(0);
  EXPECT_EQ(issue->kind(), schema::IssueKind::ShortBearing);
  EXPECT_EQ(to_id(issue->element()), door);
  EXPECT_EQ(to_id(issue->level()), level);

  const auto& overlapped = respond(session, Request{}.add_wall(level, 3000).bytes());
  const auto* overlap = overlapped.issues()->Get(0);
  EXPECT_EQ(overlap->kind(), schema::IssueKind::WallOverlap);
  EXPECT_EQ(overlap->element(), nullptr);
  ASSERT_NE(overlap->at(), nullptr);
  EXPECT_EQ(overlap->at()->x(), 0);
}

// The fills of a drawing's polygons, in order.
std::vector<schema::Fill> fills(const schema::Drawing& drawing) {
  std::vector<schema::Fill> all;
  for (const schema::DrawnPolygon* polygon : *drawing.polygons()) {
    all.push_back(polygon->fill());
  }
  return all;
}

// From where to where a polygon reaches along one axis, to the nearest tenth of a millimetre.
using Range = std::pair<double, double>;

Range across(const schema::DrawnPolygon& polygon, double (schema::PointM::*axis)() const) {
  const auto rounded = [](double metres) { return std::round(metres * 1e4) / 1e4; };
  Range reach{1e9, -1e9};
  for (const schema::PointM* point : *polygon.points()) {
    reach.first = std::min(reach.first, rounded((point->*axis)()));
    reach.second = std::max(reach.second, rounded((point->*axis)()));
  }
  return reach;
}

// The ids of the wall layers a response reports, in the order of their positions.
std::vector<ElementId> layers_in(const schema::Response& response) {
  std::vector<std::pair<std::int64_t, ElementId>> found;
  for (const schema::ElementChange* change : *response.changes()) {
    if (const schema::WallLayer* layer = change->element_as_WallLayer()) {
      found.emplace_back(layer->position(), to_id(change->id()));
    }
  }
  std::ranges::sort(found);
  std::vector<ElementId> ids;
  ids.reserve(found.size());
  for (const auto& [position, id] : found) {
    ids.push_back(id);
  }
  return ids;
}

// Whether the response reports an issue of this kind.
bool reports(const schema::Response& response, schema::IssueKind kind) {
  return std::ranges::any_of(*response.issues(),
                             [kind](const schema::Issue* issue) { return issue->kind() == kind; });
}

// How many of the walls a response reports are mirrored.
std::ptrdiff_t mirrored_walls(const schema::Response& response) {
  return std::ranges::count_if(*response.changes(), [](const schema::ElementChange* change) {
    const schema::Wall* wall = change->element_as_Wall();
    return wall != nullptr && wall->mirrored();
  });
}

TEST(Session, AWallOfATypeIsDrawnAndCountedLayerByLayer) {
  Session session{kDevice};
  ASSERT_EQ(respond(session, load_all_content()).results()->Get(0)->error(),
            schema::ErrorCode::None);
  const ElementId level = add_level(session);

  const auto& typed = respond(session, Request{}.add_wall_type().bytes());
  const ElementId type = to_id(typed.results()->Get(0)->created());
  // The type and its three layers, in one step.
  ASSERT_EQ(typed.changes()->size(), 4U);
  ASSERT_EQ(typed.log()->size(), 1U);
  EXPECT_EQ(typed.log()->Get(0)->kind(), schema::CommandKind::AddWallType);
  ASSERT_EQ(layers_in(typed).size(), 3U);

  const auto& built = respond(
      session, Request{}.add_typed_wall(level, type, {0, 0}, {6000, 0}).draw(level).bytes());

  ASSERT_EQ(built.results()->Get(0)->error(), schema::ErrorCode::None);
  const schema::Wall* wall = built.changes()->Get(0)->element_as_Wall();
  ASSERT_NE(wall, nullptr);
  EXPECT_EQ(to_id(wall->type()), type);
  EXPECT_EQ(wall->thickness(), 0);
  // 410 mm between its faces.
  ASSERT_EQ(built.outlines()->size(), 1U);
  EXPECT_DOUBLE_EQ(built.outlines()->Get(0)->points()->Get(0)->y(), -0.205);
  const schema::Drawing* drawing = built.results()->Get(1)->drawing();
  ASSERT_NE(drawing, nullptr);
  EXPECT_EQ(fills(*drawing), (std::vector{schema::Fill::Facing, schema::Fill::Insulation,
                                          schema::Fill::Wall, schema::Fill::None}));
  EXPECT_EQ(drawing->polygons()->Get(0)->stroke(), schema::Stroke::Layer);
  EXPECT_EQ(drawing->polygons()->Get(3)->stroke(), schema::Stroke::WallEdge);
  // 6 m × 2,8 m of each layer: bricks, blocks and insulation, each on its own line.
  EXPECT_NEAR(amount(built, "insulation.100"), 16.8, 1e-9);
  EXPECT_GT(amount(built, "masonry_unit.250x120x65"), 800.0);
  EXPECT_GT(amount(built, "masonry_unit.390x190x188"), 200.0);
  // Every masonry layer has its unit, so nothing asks for a material.
  EXPECT_FALSE(reports(built, schema::IssueKind::MaterialNotChosen));
}

TEST(Session, ALayerEditedOnceChangesEveryWallOfItsType) {
  Session session{kDevice};
  ASSERT_EQ(respond(session, load_all_content()).results()->Get(0)->error(),
            schema::ErrorCode::None);
  const ElementId level = add_level(session);
  const auto& typed = respond(session, Request{}.add_wall_type().bytes());
  const ElementId type = to_id(typed.results()->Get(0)->created());
  const ElementId insulation = layers_in(typed)[1];
  respond(session, Request{}
                       .add_typed_wall(level, type, {0, 0}, {6000, 0})
                       .add_typed_wall(level, type, {0, 5000}, {4000, 5000})
                       .bytes());

  const auto& thicker = respond(session, Request{}.set_layer(insulation, 150).bytes());

  ASSERT_EQ(thicker.results()->Get(0)->error(), schema::ErrorCode::None);
  // Only the layer changed; both walls are 460 mm now.
  ASSERT_EQ(thicker.changes()->size(), 1U);
  EXPECT_EQ(thicker.changes()->Get(0)->element_as_WallLayer()->thickness(), 150);
  ASSERT_EQ(thicker.outlines()->size(), 2U);
  EXPECT_DOUBLE_EQ(thicker.outlines()->Get(0)->points()->Get(0)->y(), -0.23);
  EXPECT_NEAR(amount(thicker, "insulation.150"), (6.0 + 4.0) * 2.8, 1e-9);
  EXPECT_EQ(thicker.log()->Get(0)->kind(), schema::CommandKind::SetWallLayer);

  const auto& undone = respond(session, Request{}.undo().bytes());
  EXPECT_NEAR(amount(undone, "insulation.100"), (6.0 + 4.0) * 2.8, 1e-9);
}

TEST(Session, AWallTakesATypeAndTheTypeDecidesFromThenOn) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const ElementId wall = add_wall(session, level);
  const auto& typed = respond(session, Request{}.add_wall_type().bytes());
  const ElementId type = to_id(typed.results()->Get(0)->created());

  const auto& taken = respond(session, Request{}.set_wall_type(wall, type).bytes());

  ASSERT_EQ(taken.results()->Get(0)->error(), schema::ErrorCode::None);
  const schema::Wall* now = taken.changes()->Get(0)->element_as_Wall();
  EXPECT_EQ(to_id(now->type()), type);
  EXPECT_EQ(now->thickness(), 0);
  ASSERT_EQ(taken.log()->size(), 1U);
  EXPECT_EQ(taken.log()->Get(0)->kind(), schema::CommandKind::SetWallType);

  const auto& refused =
      respond(session, Request{}
                           .set_unit(wall)
                           .add_typed_wall(level, type, {0, 0}, {0, 4000}, 380)
                           .add_typed_wall(level, ElementId{Uuid{7, 7}}, {0, 0}, {0, 4000})
                           .set_wall_type(wall, ElementId{Uuid{7, 7}})
                           .empty(Command::AddWallType)
                           .bytes());
  ASSERT_EQ(refused.results()->size(), 5U);
  EXPECT_EQ(refused.results()->Get(0)->error(), schema::ErrorCode::TypeDecides);
  EXPECT_EQ(refused.results()->Get(1)->error(), schema::ErrorCode::TypeDecides);
  EXPECT_EQ(refused.results()->Get(2)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(refused.results()->Get(3)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(refused.results()->Get(4)->error(), schema::ErrorCode::CorruptData);
  EXPECT_EQ(refused.changes()->size(), 0U);

  // Turned round, the facing lies on the wall's left face.
  const auto& turned = respond(session, Request{}.set_mirrored(wall, true).draw(level).bytes());
  EXPECT_TRUE(turned.changes()->Get(0)->element_as_Wall()->mirrored());
  const schema::Drawing* drawing = turned.results()->Get(1)->drawing();
  ASSERT_EQ(drawing->polygons()->Get(0)->fill(), schema::Fill::Facing);
  EXPECT_EQ(across(*drawing->polygons()->Get(0), &schema::PointM::y), (Range{0.085, 0.205}));

  const auto& back = respond(session, Request{}.undo().undo().bytes());
  const schema::Wall* plain = back.changes()->Get(0)->element_as_Wall();
  EXPECT_EQ(plain->type(), nullptr);
  EXPECT_EQ(plain->thickness(), 380);
}

TEST(Session, ATypeSaysWhetherItsWallsBearLoad) {
  Session session{kDevice};
  ASSERT_EQ(respond(session, load_all_content()).results()->Get(0)->error(),
            schema::ErrorCode::None);
  const ElementId level = add_level(session);
  const auto& typed =
      respond(session, Request{}.add_wall_type(schema::WallRole::Partition).bytes());
  const ElementId type = to_id(typed.results()->Get(0)->created());

  // 410 mm thick, but a partition: the norm asks for no ring beam on it.
  const auto& built =
      respond(session, Request{}.add_typed_wall(level, type, {0, 0}, {6000, 0}).bytes());
  EXPECT_FALSE(reports(built, schema::IssueKind::RingBeamMissing));

  const auto& bearing =
      respond(session, Request{}.set_role(type, schema::WallRole::Bearing).bytes());
  ASSERT_EQ(bearing.issues()->size(), 1U);
  EXPECT_EQ(bearing.issues()->Get(0)->kind(), schema::IssueKind::RingBeamMissing);
}

TEST(Session, ALoopSketchedClockwiseStartsItsLayersFromTheLeftFace) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const auto& typed = respond(session, Request{}.add_wall_type().bytes());
  const ElementId type = to_id(typed.results()->Get(0)->created());

  // Up the left side first: the room's outside is on the left of every wall.
  const auto& drawn = respond(
      session,
      Request{}
          .add_typed_sketch(level, type, {{0, 0}, {0, 4000}, {6000, 4000}, {6000, 0}, {0, 0}})
          .draw(level)
          .bytes());

  ASSERT_EQ(drawn.results()->Get(0)->error(), schema::ErrorCode::None);
  ASSERT_EQ(drawn.changes()->size(), 4U);
  EXPECT_EQ(mirrored_walls(drawn), 4);
  const schema::Wall* first = drawn.changes()->Get(0)->element_as_Wall();
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(to_id(first->type()), type);
  EXPECT_FALSE(first->exact());
  // After the room's floor comes the first wall, up the left side; its facing is its outermost
  // 120 mm.
  const schema::Drawing* drawing = drawn.results()->Get(1)->drawing();
  ASSERT_EQ(drawing->polygons()->Get(0)->fill(), schema::Fill::Floor);
  ASSERT_EQ(drawing->polygons()->Get(1)->fill(), schema::Fill::Facing);
  EXPECT_EQ(across(*drawing->polygons()->Get(1), &schema::PointM::x), (Range{-0.205, -0.085}));

  // The other way round nothing is mirrored.
  respond(session, Request{}.undo().bytes());
  const auto& again = respond(
      session,
      Request{}
          .add_typed_sketch(level, type, {{0, 0}, {6000, 0}, {6000, 4000}, {0, 4000}, {0, 0}})
          .bytes());
  ASSERT_EQ(again.changes()->size(), 4U);
  EXPECT_EQ(mirrored_walls(again), 0);
}

// The texts of a drawing of one kind.
std::vector<const schema::DrawnText*> texts(const schema::Drawing& drawing, schema::TextKind kind) {
  std::vector<const schema::DrawnText*> found;
  for (const schema::DrawnText* text : *drawing.texts()) {
    if (text->kind() == kind) {
      found.push_back(text);
    }
  }
  return found;
}

// The issues of a kind, as the rooms they name.
std::vector<ElementId> rooms_with(const schema::Response& response, schema::IssueKind kind) {
  std::vector<ElementId> rooms;
  for (const schema::Issue* issue : *response.issues()) {
    if (issue->kind() == kind) {
      rooms.push_back(to_id(issue->element()));
    }
  }
  return rooms;
}

TEST(Session, ARoomNamesTheSpaceThatHoldsItsAnchor) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const std::vector<ElementId> walls = add_room(session, level);

  const auto& named =
      respond(session, Request{}.name_room(level, {1000, 1000}, schema::RoomKind::Bedroom).bytes());
  ASSERT_EQ(named.results()->Get(0)->error(), schema::ErrorCode::None);
  const ElementId room = to_id(named.results()->Get(0)->created());
  ASSERT_EQ(named.changes()->size(), 1U);
  const schema::Room* saved = named.changes()->Get(0)->element_as_Room();
  ASSERT_NE(saved, nullptr);
  EXPECT_EQ(to_id(saved->level()), level);
  EXPECT_EQ(saved->anchor()->x(), 1000);
  EXPECT_EQ(saved->kind(), schema::RoomKind::Bedroom);
  EXPECT_EQ(named.log()->Get(0)->kind(), schema::CommandKind::AddRoom);
  EXPECT_EQ(named.issues()->size(), 0U);

  // A tap in the middle finds the space with its room; beside a wall, the wall; outside, nothing.
  const auto& picked = respond(session, Request{}
                                            .pick(level, {3000, 2000})
                                            .pick(level, {3000, 300})
                                            .pick(level, {9000, 2000})
                                            .draw(level)
                                            .bytes());
  const schema::PickedSpace* space = picked.results()->Get(0)->space();
  ASSERT_NE(space, nullptr);
  EXPECT_EQ(picked.results()->Get(0)->picked(), nullptr);
  EXPECT_EQ(to_id(space->room()), room);
  EXPECT_EQ(space->kind(), schema::RoomKind::Bedroom);
  // Between the faces of 380 mm walls on their centre lines.
  EXPECT_NEAR(space->area(), 5.62 * 3.62, 1e-9);
  EXPECT_NEAR(space->perimeter(), 2 * (5.62 + 3.62), 1e-9);
  EXPECT_EQ(space->outline()->size(), 4U);
  EXPECT_EQ(space->islands()->size(), 0U);
  EXPECT_EQ(to_id(picked.results()->Get(1)->picked()), walls[0]);
  EXPECT_EQ(picked.results()->Get(1)->space(), nullptr);
  EXPECT_EQ(picked.results()->Get(2)->picked(), nullptr);
  EXPECT_EQ(picked.results()->Get(2)->space(), nullptr);

  // The drawing has the floor, named, and the room's name over its area.
  const schema::Drawing* drawing = picked.results()->Get(3)->drawing();
  ASSERT_NE(drawing, nullptr);
  EXPECT_EQ(drawing->polygons()->Get(0)->fill(), schema::Fill::Floor);
  EXPECT_EQ(to_id(drawing->polygons()->Get(0)->element()), room);
  const auto names = texts(*drawing, schema::TextKind::RoomName);
  const auto areas = texts(*drawing, schema::TextKind::RoomArea);
  ASSERT_EQ(names.size(), 1U);
  ASSERT_EQ(areas.size(), 1U);
  EXPECT_EQ(names[0]->value(), static_cast<std::int64_t>(schema::RoomKind::Bedroom));
  EXPECT_EQ(to_id(names[0]->element()), room);
  EXPECT_EQ(areas[0]->value(), 203'444);
  EXPECT_NEAR(names[0]->fit(), 5.62 * 0.8, 1e-9);
  EXPECT_DOUBLE_EQ(names[0]->at()->x(), 3.0);

  // Renamed, and the name before it comes back with undo.
  const auto& renamed =
      respond(session, Request{}.set_room_kind(room, schema::RoomKind::Study).bytes());
  EXPECT_EQ(renamed.changes()->Get(0)->element_as_Room()->kind(), schema::RoomKind::Study);
  const auto& undone = respond(session, Request{}.undo().bytes());
  EXPECT_EQ(undone.changes()->Get(0)->element_as_Room()->kind(), schema::RoomKind::Bedroom);
}

// The first issue of a kind, or null.
const schema::Issue* issue_of(const schema::Response& response, schema::IssueKind kind) {
  const auto found = std::ranges::find(*response.issues(), kind, &schema::Issue::kind);
  return found != response.issues()->end() ? *found : nullptr;
}

TEST(Session, ARoomWithNoSpaceOfItsOwnIsReported) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_room(session, level);
  const auto& named =
      respond(session, Request{}
                           .name_room(level, {1000, 1000}, schema::RoomKind::Kitchen)
                           .name_room(level, {5000, 3000}, schema::RoomKind::Hall)
                           .name_room(level, {9000, 9000}, schema::RoomKind::Garage)
                           .bytes());
  const ElementId hall = to_id(named.results()->Get(1)->created());
  const ElementId garage = to_id(named.results()->Get(2)->created());

  // The kitchen was there first; the garage stands where no walls close.
  EXPECT_EQ(rooms_with(named, schema::IssueKind::RoomSharesSpace), std::vector{hall});
  EXPECT_EQ(rooms_with(named, schema::IssueKind::RoomWithoutSpace), std::vector{garage});
  const schema::Issue* outside = issue_of(named, schema::IssueKind::RoomWithoutSpace);
  ASSERT_NE(outside, nullptr);
  EXPECT_EQ(to_id(outside->level()), level);
  ASSERT_NE(outside->at(), nullptr);
  EXPECT_EQ(outside->at()->x(), 9000);

  // Moved into the space, the garage shares it too; taken off, both are in order again.
  const auto& moved = respond(session, Request{}.move_room(garage, {3000, 3000}).bytes());
  EXPECT_EQ(rooms_with(moved, schema::IssueKind::RoomSharesSpace), (std::vector{hall, garage}));
  const auto& fixed = respond(session, Request{}.remove_room(hall).remove_room(garage).bytes());
  EXPECT_EQ(fixed.issues()->size(), 0U);
  EXPECT_EQ(deleted(fixed).size(), 2U);
}

TEST(Session, ARoomLosesItsSpaceWhenItsWallsNoLongerClose) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const std::vector<ElementId> walls = add_room(session, level);
  const auto& named =
      respond(session, Request{}.name_room(level, {1000, 1000}, schema::RoomKind::Kitchen).bytes());
  const ElementId kitchen = to_id(named.results()->Get(0)->created());

  const auto& opened = respond(session, Request{}.remove_wall(walls[2]).bytes());
  EXPECT_EQ(rooms_with(opened, schema::IssueKind::RoomWithoutSpace), std::vector{kitchen});
  const auto& asked = respond(session, Request{}.pick(level, {3000, 2000}).draw(level).bytes());
  EXPECT_EQ(asked.results()->Get(0)->space(), nullptr);
  EXPECT_TRUE(texts(*asked.results()->Get(1)->drawing(), schema::TextKind::RoomName).empty());

  // The wall back, the kitchen is a room again.
  const auto& closed = respond(session, Request{}.undo().bytes());
  EXPECT_EQ(closed.issues()->size(), 0U);
}

TEST(Session, ARoomCommandWithAnUnknownKindOrNoPointIsCorrupt) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  // An app built against a newer schema may name a room this engine does not know.
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  const auto no_such_kind = static_cast<schema::RoomKind>(200);

  const auto& refused =
      respond(session, Request{}
                           .name_room(level, {1000, 1000}, no_such_kind)
                           .name_room(ElementId{Uuid{7, 7}}, {1000, 1000}, schema::RoomKind::Room)
                           .remove_room(ElementId{Uuid{7, 7}})
                           .empty(Command::AddRoom)
                           .bytes());

  EXPECT_EQ(refused.results()->Get(0)->error(), schema::ErrorCode::CorruptData);
  EXPECT_EQ(refused.results()->Get(1)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(refused.results()->Get(2)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(refused.results()->Get(3)->error(), schema::ErrorCode::CorruptData);
  EXPECT_EQ(refused.changes()->size(), 0U);
}

// The values of a drawing's texts of one kind, in order.
std::vector<std::int64_t> sizes(const schema::Drawing& drawing, schema::TextKind kind) {
  std::vector<std::int64_t> values;
  for (const schema::DrawnText* text : texts(drawing, kind)) {
    values.push_back(text->value());
  }
  return values;
}

// The opening as the response reports it changed.
const schema::Opening* changed_opening(const schema::Response& response) {
  return response.changes()->size() == 1 ? response.changes()->Get(0)->element_as_Opening()
                                         : nullptr;
}

TEST(Session, TheSpaceInFocusIsDrawnWithItsSizes) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_room(session, level);
  const auto& placed = respond(session, Request{}.place(level, {3000, 0}).bytes());
  const ElementId window = to_id(placed.results()->Get(0)->created());

  const auto& asked = respond(session, Request{}.draw(level, {3000, 2000}).bytes());
  const schema::Drawing* drawing = asked.results()->Get(0)->drawing();
  ASSERT_NE(drawing, nullptr);

  // Between the faces of 380 mm walls; the window, 1.5 m wide, in the middle of the lower one.
  EXPECT_EQ(sizes(*drawing, schema::TextKind::Clear), (std::vector<std::int64_t>{5620, 3620}));
  EXPECT_EQ(sizes(*drawing, schema::TextKind::Pier), (std::vector<std::int64_t>{2060, 2060}));
  EXPECT_EQ(sizes(*drawing, schema::TextKind::OpeningWidth), std::vector<std::int64_t>{1500});
  EXPECT_EQ(sizes(*drawing, schema::TextKind::Overall), (std::vector<std::int64_t>{6380, 4380}));
  EXPECT_TRUE(texts(*drawing, schema::TextKind::RoomArea).empty());
  const schema::DrawnText* pier = texts(*drawing, schema::TextKind::Pier)[0];
  EXPECT_EQ(to_id(pier->element()), window);
  EXPECT_EQ(pier->from(), 190);
  EXPECT_TRUE(pier->forward());
  EXPECT_EQ(asked.changes()->size(), 0U);
}

TEST(Session, ATypedSizeMovesOrResizesWhatItMeasures) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const std::vector<ElementId> walls = add_room(session, level);
  respond(session, Request{}.place(level, {3000, 0}).bytes());
  const auto& asked = respond(session, Request{}.draw(level, {3000, 2000}).bytes());
  const schema::Drawing* drawing = asked.results()->Get(0)->drawing();
  // The request is built while the drawing is still there to read.
  const std::vector<std::uint8_t> typed =
      Request{}
          .set_dimension(*texts(*drawing, schema::TextKind::Pier)[0], 1000)
          .set_dimension(*texts(*drawing, schema::TextKind::OpeningWidth)[0], 1200)
          .bytes();
  const std::vector<std::uint8_t> too_far =
      Request{}.set_dimension(*texts(*drawing, schema::TextKind::Pier)[1], 9000).bytes();

  // 1 000 mm of wall from the room's corner, 190 mm along the wall, to the window; then the
  // window 1 200 mm wide from there.
  const auto& sized = respond(session, typed);
  ASSERT_EQ(sized.results()->Get(0)->error(), schema::ErrorCode::None);
  ASSERT_EQ(sized.results()->Get(1)->error(), schema::ErrorCode::None);
  const schema::Opening* moved = changed_opening(sized);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->offset(), 1190);
  EXPECT_EQ(moved->width(), 1200);
  EXPECT_EQ(sized.log()->size(), 2U);

  // A piece of wall longer than the wall has room for is refused, and nothing changes.
  const auto& refused = respond(session, too_far);
  EXPECT_EQ(refused.results()->Get(0)->error(), schema::ErrorCode::DoesNotFit);
  EXPECT_EQ(refused.changes()->size(), 0U);

  // A wall's length is a size too; the plan's overall size is not one to type.
  const auto& other =
      respond(session, Request{}
                           .set_dimension(schema::TextKind::Length, walls[0], 5000)
                           .set_dimension(schema::TextKind::Overall, walls[0], 5000)
                           .set_dimension(schema::TextKind::Pier, ElementId{Uuid{7, 7}}, 500)
                           .bytes());
  EXPECT_EQ(other.results()->Get(0)->error(), schema::ErrorCode::None);
  EXPECT_EQ(other.results()->Get(1)->error(), schema::ErrorCode::CorruptData);
  EXPECT_EQ(other.results()->Get(2)->error(), schema::ErrorCode::NotFound);
  EXPECT_EQ(line(other, walls[0]), (Line{0, 0, 5000, 0}));
}

TEST(Session, ATypedClearSizeMovesAWallWithWhatIsJoinedToIt) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  const std::vector<ElementId> walls = add_room(session, level);
  const auto& asked = respond(session, Request{}.draw(level, {3000, 2000}).bytes());
  const schema::DrawnText* across =
      texts(*asked.results()->Get(0)->drawing(), schema::TextKind::Clear)[0];
  // Between the left wall and the right one, which is the one to move.
  EXPECT_EQ(to_id(across->other()), walls[3]);
  EXPECT_EQ(to_id(across->element()), walls[1]);
  const std::vector<std::uint8_t> typed = Request{}.set_dimension(*across, 5000).bytes();

  // 5 620 mm clear becomes 5 000: the right wall comes 620 mm in, and the walls at its ends
  // with it, in one step.
  const auto& sized = respond(session, typed);
  ASSERT_EQ(sized.results()->Get(0)->error(), schema::ErrorCode::None);
  EXPECT_EQ(line(sized, walls[1]), (Line{5380, 0, 5380, 4000}));
  EXPECT_EQ(line(sized, walls[0]), (Line{0, 0, 5380, 0}));
  EXPECT_EQ(sized.changes()->size(), 3U);
  EXPECT_EQ(sized.log()->size(), 1U);

  const auto& undone = respond(session, Request{}.undo().bytes());
  EXPECT_EQ(line(undone, walls[1]), (Line{6000, 0, 6000, 4000}));

  // Walls that do not run side by side have no clear size between them.
  const auto& refused =
      respond(session, Request{}.set_dimension(schema::TextKind::Clear, walls[1], 5000).bytes());
  EXPECT_EQ(refused.results()->Get(0)->error(), schema::ErrorCode::NotFound);
}

TEST(Session, ADoorIsHungAndOpensAsTheBuilderSays) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_wall(session, level);
  const auto& placed =
      respond(session, Request{}.place(level, {3000, 0}, 900, schema::OpeningKind::Door).bytes());
  const ElementId door = to_id(placed.results()->Get(0)->created());
  const schema::Opening* drawn = changed_opening(placed);
  ASSERT_NE(drawn, nullptr);
  EXPECT_EQ(drawn->hinge(), schema::Hinge::Near);
  EXPECT_EQ(drawn->swing(), schema::Swing::Left);

  const auto& turned = respond(session, Request{}
                                            .set_hinge(door, schema::Hinge::Far)
                                            .set_swing(door, schema::Swing::Right)
                                            .draw(level)
                                            .bytes());
  ASSERT_EQ(turned.results()->Get(0)->error(), schema::ErrorCode::None);
  const schema::Opening* hung = changed_opening(turned);
  ASSERT_NE(hung, nullptr);
  EXPECT_EQ(hung->hinge(), schema::Hinge::Far);
  EXPECT_EQ(hung->swing(), schema::Swing::Right);
  EXPECT_EQ(turned.log()->Get(0)->kind(), schema::CommandKind::SetOpeningHinge);
  EXPECT_EQ(turned.log()->Get(1)->kind(), schema::CommandKind::SetOpeningSwing);
  // The leaf now hangs on the far edge, 3 450 mm along, and stands out of the right face.
  const schema::DrawnArc* swing = turned.results()->Get(2)->drawing()->arcs()->Get(0);
  EXPECT_DOUBLE_EQ(swing->centre()->x(), 3.45);
  EXPECT_DOUBLE_EQ(swing->centre()->y(), -0.19);

  // An app built against a newer schema may send a swing this engine does not know.
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  const auto no_such_swing = static_cast<schema::Swing>(9);
  const auto& refused = respond(session, Request{}.set_swing(door, no_such_swing).bytes());
  EXPECT_EQ(refused.results()->Get(0)->error(), schema::ErrorCode::CorruptData);
}

TEST(Session, OpeningsOfOneSizeShareAMarkAndASizeKeepsItsNumber) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_room(session, level);
  // Two windows of 1 500, a door, then a window of 1 200.
  const auto& placed =
      respond(session, Request{}
                           .place(level, {1500, 0})
                           .place(level, {4500, 0})
                           .place(level, {6000, 2000}, 900, schema::OpeningKind::Door)
                           .place(level, {3000, 4000}, 1200)
                           .draw(level)
                           .bytes());
  const schema::Drawing* drawing = placed.results()->Get(4)->drawing();
  ASSERT_NE(drawing, nullptr);
  // The two first windows share ОК-1 and the narrower one is ОК-2; the door is the first of
  // its kind.
  EXPECT_EQ(sizes(*drawing, schema::TextKind::WindowMark), (std::vector<std::int64_t>{1, 1, 2}));
  EXPECT_EQ(sizes(*drawing, schema::TextKind::DoorMark), std::vector<std::int64_t>{1});

  // With the first two gone the 1 200 window is the oldest of the windows: it is ОК-1 now.
  const ElementId first = to_id(placed.results()->Get(0)->created());
  const ElementId second = to_id(placed.results()->Get(1)->created());
  const auto& fewer =
      respond(session, Request{}.remove_opening(first).remove_opening(second).draw(level).bytes());
  EXPECT_EQ(sizes(*fewer.results()->Get(2)->drawing(), schema::TextKind::WindowMark),
            std::vector<std::int64_t>{1});
}

TEST(Session, ASelectedOpeningHasItsChainAndGoesToTheMiddleOfItsSide) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_room(session, level);
  const auto& placed = respond(session, Request{}.place(level, {1500, 0}).bytes());
  const ElementId window = to_id(placed.results()->Get(0)->created());

  // Selected, the window shows the side it is in: 560 mm from the room's corner, then 3 560.
  const auto& asked = respond(session, Request{}.draw_selected(level, window).bytes());
  const schema::Drawing* drawing = asked.results()->Get(0)->drawing();
  EXPECT_EQ(sizes(*drawing, schema::TextKind::Pier), (std::vector<std::int64_t>{560, 3560}));
  EXPECT_EQ(sizes(*drawing, schema::TextKind::OpeningWidth), std::vector<std::int64_t>{1500});
  EXPECT_TRUE(texts(*drawing, schema::TextKind::Clear).empty());

  // Centred, it stands 2 060 mm from each corner of the room: 2 250 along the wall.
  const auto& centred =
      respond(session, Request{}.center(window).draw_selected(level, window).bytes());
  ASSERT_EQ(centred.results()->Get(0)->error(), schema::ErrorCode::None);
  EXPECT_EQ(changed_opening(centred)->offset(), 2250);
  EXPECT_EQ(centred.log()->Get(0)->kind(), schema::CommandKind::MoveOpening);
  EXPECT_EQ(sizes(*centred.results()->Get(1)->drawing(), schema::TextKind::Pier),
            (std::vector<std::int64_t>{2060, 2060}));

  const auto& missing = respond(session, Request{}.center(ElementId{Uuid{7, 7}}).bytes());
  EXPECT_EQ(missing.results()->Get(0)->error(), schema::ErrorCode::NotFound);
}

TEST(Session, ASheetHasEveryRoomsSizesAndTheSchedules) {
  Session session{kDevice};
  const ElementId level = add_level(session);
  add_room(session, level);
  const auto& built =
      respond(session, Request{}
                           .name_room(level, {3000, 2000}, schema::RoomKind::Kitchen)
                           .place(level, {1500, 0})
                           .place(level, {4500, 0})
                           .place(level, {6000, 2000}, 900, schema::OpeningKind::Door)
                           .draw_sheet(level)
                           .bytes());
  const ElementId kitchen = to_id(built.results()->Get(0)->created());
  const schema::Drawing* sheet = built.results()->Get(4)->drawing();
  ASSERT_NE(sheet, nullptr);

  // Nothing is selected, yet the room has its sizes; its name stays.
  EXPECT_EQ(sizes(*sheet, schema::TextKind::Clear), (std::vector<std::int64_t>{5620, 3620}));
  EXPECT_EQ(texts(*sheet, schema::TextKind::RoomName).size(), 1U);
  EXPECT_FALSE(sizes(*sheet, schema::TextKind::Pier).empty());

  // The room schedule and the opening schedule come with the drawing.
  ASSERT_EQ(sheet->rooms()->size(), 1U);
  EXPECT_EQ(to_id(sheet->rooms()->Get(0)->room()), kitchen);
  EXPECT_EQ(sheet->rooms()->Get(0)->kind(), schema::RoomKind::Kitchen);
  EXPECT_NEAR(sheet->rooms()->Get(0)->area(), 5.62 * 3.62, 1e-9);
  ASSERT_EQ(sheet->openings()->size(), 2U);
  const schema::ListedOpening* windows = sheet->openings()->Get(0);
  EXPECT_EQ(windows->kind(), schema::TextKind::WindowMark);
  EXPECT_EQ(windows->number(), 1);
  EXPECT_EQ(windows->width(), 1500);
  EXPECT_EQ(windows->height(), 1500);
  EXPECT_EQ(windows->count(), 2);
  EXPECT_EQ(sheet->openings()->Get(1)->kind(), schema::TextKind::DoorMark);
  EXPECT_EQ(sheet->openings()->Get(1)->count(), 1);
}

}  // namespace
}  // namespace otves::bridge
