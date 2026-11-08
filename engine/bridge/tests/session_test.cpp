#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <flatbuffers/flatbuffers.h>
#include <gtest/gtest.h>

#include <otves/bridge/session.h>
#include <otves/core/uuid.h>

#include "engine_generated.h"

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

  Request& add_window(ElementId wall) {
    const schema::Uuid in = key(wall);
    const auto opening = schema::CreateOpening(builder_, &in, 1200, 1500, 1500, 900);
    return add(Command::AddOpening, schema::CreateAddOpening(builder_, opening).Union());
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
       {"losses", "masonry", "mortar", "practice", "rebar", "seismic"}) {
    norms.push_back(content_file("norms", name));
  }
  Request::Files rules;
  for (const std::string_view name : {"masonry", "lintels", "ring_beam", "junction_meshes"}) {
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
  const auto& built = respond(session, Request{}.add_wall(level, 6400).bytes());

  ASSERT_NE(built.takeoff(), nullptr);
  EXPECT_EQ(built.takeoff_error(), schema::ErrorCode::None);
  // 6.4 m × 2.8 m of 380 mm brick: about 2 686 units net, plus 3 % waste.
  EXPECT_GT(amount(built, "masonry_unit"), 2686);
  EXPECT_GT(amount(built, "concrete.b15"), 0.0);
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
  const auto& built = respond(session, Request{}.add_wall(level, 6400).bytes());
  EXPECT_GT(amount(built, "masonry_unit"), 0.0);
}

}  // namespace
}  // namespace otves::bridge
