#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <flatbuffers/flatbuffers.h>

#include <otves/bridge/session.h>
#include <otves/core/command.h>
#include <otves/core/document.h>
#include <otves/core/error.h>
#include <otves/core/op.h>
#include <otves/core/uuid.h>
#include <otves/geom/editing.h>
#include <otves/takeoff/rules.h>

#include "convert.h"
#include "derived.h"
#include "engine_generated.h"
#include "oplog.h"
#include "placement.h"

namespace otves::bridge {
namespace {

constexpr std::uint32_t kVersion = 1;
constexpr const char* kIdentifier = "OTVS";

// The engine's clock and entropy on a device: wall-clock milliseconds and a generator seeded
// once from the system.
DocumentEnvironment device_environment(DeviceId device) {
  const auto generator = std::make_shared<std::mt19937_64>(std::random_device{}());
  return {.device = device,
          .unix_ms =
              [] {
                const auto now = std::chrono::system_clock::now().time_since_epoch();
                return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
              },
          .random_u64 = [generator] { return (*generator)(); }};
}

}  // namespace

struct Session::Impl {
  explicit Impl(DeviceId device) : document(device_environment(device)) {}

  using Outcome = std::expected<std::optional<ElementId>, Error>;

  Outcome execute(const schema::CommandEntry& entry) {
    switch (entry.command_type()) {
      case schema::Command::BeginGesture:
        document.begin_gesture();
        return std::nullopt;
      case schema::Command::EndGesture:
        static_cast<void>(document.end_gesture());
        return std::nullopt;
      case schema::Command::Undo:
        return document.undo().transform([] { return std::optional<ElementId>{}; });
      case schema::Command::Redo:
        return document.redo().transform([] { return std::optional<ElementId>{}; });
      case schema::Command::LoadContent:
        return load(entry.command_as_LoadContent());
      case schema::Command::OpenModel:
        return decode(entry.command_as_OpenModel())
            .and_then([this](const Snapshot& snapshot) { return open(snapshot); })
            .transform([] { return std::optional<ElementId>{}; });
      case schema::Command::PlaceOpening:
        return decode(entry.command_as_PlaceOpening())
            .and_then([this](const OpeningPlacement& placement) {
              return place_opening(document.model(), placement);
            })
            .and_then([this](const AddOpening& add) { return document.execute(add); });
      case schema::Command::SetWallLength:
        return decode(entry.command_as_SetWallLength())
            .and_then([this](const Resize& set) { return resize(document.model(), set); })
            .and_then([this](const std::vector<Command>& steps) { return together(steps); });
      case schema::Command::AddSketch:
        return decode(entry.command_as_AddSketch())
            .and_then([this](const Stroke& stroke) { return sketch(document.model(), stroke); })
            .and_then([this](const std::vector<Command>& walls) { return together(walls); });
      default:
        return decode(entry).and_then(
            [this](const Command& command) { return document.execute(command); });
    }
  }

  // Several commands as one undo step: a gesture of their own, or part of the app's when it has
  // one open. resize() and sketch() have checked that each succeeds.
  Outcome together(std::span<const Command> commands) {
    const bool own = !document.in_gesture();
    if (own) {
      document.begin_gesture();
    }
    Outcome outcome = std::nullopt;
    for (const Command& command : commands) {
      outcome = document.execute(command);
      if (!outcome) {
        break;
      }
    }
    if (own) {
      static_cast<void>(document.end_gesture());
    }
    return outcome.transform(
        [](const std::optional<ElementId>&) -> std::optional<ElementId> { return std::nullopt; });
  }

  // The log starts over with the opened job, so groups not yet reported are kept aside first.
  std::expected<void, Error> open(const Snapshot& snapshot) {
    const std::span<const OpGroup> log = document.log();
    std::vector<OpGroup> waiting{log.begin() + static_cast<std::ptrdiff_t>(reported), log.end()};
    auto opened = document.open(snapshot);
    if (opened) {
      unreported.insert(unreported.end(), waiting.begin(), waiting.end());
      reported = 0;
    }
    return opened;
  }

  // Compiles first, so a broken bundle keeps the content that ran before it.
  Outcome load(const schema::LoadContent* content) {
    const auto scripts = [](const auto* list) {
      std::vector<takeoff::Script> out;
      if (list != nullptr) {
        for (const schema::Script* script : *list) {
          out.push_back({.name = script->name() != nullptr ? script->name()->str() : "",
                         .source = script->source() != nullptr ? script->source()->str() : ""});
        }
      }
      return out;
    };
    const std::vector<takeoff::Script> norms =
        scripts(content != nullptr ? content->norms() : nullptr);
    const std::vector<takeoff::Script> laws =
        scripts(content != nullptr ? content->rules() : nullptr);
    auto compiled = takeoff::RuleSet::compile({.norms = norms, .rules = laws});
    if (!compiled) {
      return std::unexpected{std::move(compiled).error()};
    }
    rules.emplace(*std::move(compiled));
    content_changed = true;
    return std::nullopt;
  }

  flatbuffers::Offset<schema::CommandResult> result(const Outcome& outcome) {
    if (!outcome) {
      return error(outcome.error().code, outcome.error().message);
    }
    if (!outcome->has_value()) {
      return schema::CreateCommandResult(builder);
    }
    const schema::Uuid created = to_schema(**outcome);
    return schema::CreateCommandResult(builder, schema::ErrorCode::None, 0, &created);
  }

  // A pick reads the model and answers with the element it found.
  flatbuffers::Offset<schema::CommandResult> picked(const schema::CommandEntry& entry) {
    const auto found = decode(entry.command_as_Pick()).and_then([this](const Tap& tap) {
      return pick(document.model(), tap);
    });
    if (!found) {
      return error(found.error().code, found.error().message);
    }
    if (!found->has_value()) {
      return schema::CreateCommandResult(builder);
    }
    const schema::Uuid id = to_schema(**found);
    return schema::CreateCommandResult(builder, schema::ErrorCode::None, 0, nullptr, &id);
  }

  // An aim reads the model and answers with where the wall would end.
  flatbuffers::Offset<schema::CommandResult> aimed(const schema::CommandEntry& entry) {
    const Result<geom::Aimed> answer =
        entry.command_type() == schema::Command::Aim
            ? decode(entry.command_as_Aim()).and_then([this](const Sight& sight) {
                return aim(document.model(), sight);
              })
            : decode(entry.command_as_AimMeasured()).and_then([this](const MeasuredSight& sight) {
                return aim(document.model(), sight);
              });
    if (!answer) {
      return error(answer.error().code, answer.error().message);
    }
    std::vector<schema::Guide> guides;
    guides.reserve(answer->guides.size());
    for (const geom::Guide& guide : answer->guides) {
      guides.emplace_back(to_schema(guide.from), to_schema(guide.to));
    }
    const schema::PointMm point = to_schema(answer->point);
    const auto result = schema::CreateAimed(builder, &point, answer->length.value(),
                                            builder.CreateVectorOfStructs(guides), answer->meets);
    return schema::CreateCommandResult(builder, schema::ErrorCode::None, 0, nullptr, nullptr,
                                       result);
  }

  // A drawing reads the model and answers with a level as a display list.
  flatbuffers::Offset<schema::CommandResult> drawn(const schema::CommandEntry& entry) {
    const auto drawing = decode(entry.command_as_Draw()).and_then([this](const View& view) {
      return draw(document.model(), view);
    });
    if (!drawing) {
      return error(drawing.error().code, drawing.error().message);
    }
    const auto list = encode_drawing(builder, *drawing);
    return schema::CreateCommandResult(builder, schema::ErrorCode::None, 0, nullptr, nullptr, 0,
                                       list);
  }

  // A query changes nothing; every other entry is a command.
  flatbuffers::Offset<schema::CommandResult> answer(const schema::CommandEntry& entry) {
    switch (entry.command_type()) {
      case schema::Command::Pick:
        return picked(entry);
      case schema::Command::Aim:
      case schema::Command::AimMeasured:
        return aimed(entry);
      case schema::Command::Draw:
        return drawn(entry);
      default:
        return result(execute(entry));
    }
  }

  flatbuffers::Offset<schema::CommandResult> error(ErrorCode code, const std::string& message) {
    return error(to_schema(code), message);
  }

  flatbuffers::Offset<schema::CommandResult> error(schema::ErrorCode code,
                                                   const std::string& message) {
    const auto text = builder.CreateString(message);
    return schema::CreateCommandResult(builder, code, text);
  }

  void run(std::span<const std::uint8_t> request) {
    std::vector<flatbuffers::Offset<schema::CommandResult>> results;
    flatbuffers::Verifier verifier(request.data(), request.size());
    if (!verifier.VerifyBuffer<schema::Request>(kIdentifier)) {
      results.push_back(error(ErrorCode::kCorruptData, "request failed verification"));
    } else if (const auto* parsed = flatbuffers::GetRoot<schema::Request>(request.data());
               parsed->version() != kVersion) {
      results.push_back(error(ErrorCode::kUnsupportedVersion,
                              "request version " + std::to_string(parsed->version())));
    } else if (parsed->commands() != nullptr) {
      results.reserve(parsed->commands()->size());
      for (const schema::CommandEntry* entry : *parsed->commands()) {
        results.push_back(answer(*entry));
      }
    }
    finish(results);
  }

  void finish(const std::vector<flatbuffers::Offset<schema::CommandResult>>& results) {
    std::vector<flatbuffers::Offset<schema::ElementChange>> changes;
    for (const ElementId id : document.take_dirty()) {
      changes.push_back(encode_change(builder, document.model(), id));
    }
    OutlinesOffset outlines;
    OpeningOutlinesOffset openings;
    TakeoffOffsets takeoff;
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<schema::Issue>>> issues;
    if (!changes.empty() || content_changed) {
      IssueOffsets found;
      outlines = encode_outlines(builder, document.model(), found);
      openings = encode_openings(builder, document.model());
      takeoff = encode_takeoff(builder, document.model(), rules, found);
      issues = builder.CreateVector(found);
      content_changed = false;
    }
    const std::span<const OpGroup> log = document.log().subspan(reported);
    std::vector<flatbuffers::Offset<schema::LogEntry>> entries;
    entries.reserve(unreported.size() + log.size());
    for (const OpGroup& group : unreported) {
      entries.push_back(encode_log_entry(builder, group));
    }
    for (const OpGroup& group : log) {
      entries.push_back(encode_log_entry(builder, group));
    }
    const auto response = schema::CreateResponse(
        builder, kVersion, builder.CreateVector(results), builder.CreateVector(changes),
        document.can_undo(), document.can_redo(), outlines, takeoff.lines, takeoff.error,
        takeoff.message, openings, builder.CreateVector(entries), issues);
    builder.Finish(response, kIdentifier);
    // Only a finished response counts as reported; after a failure the next one repeats them.
    unreported.clear();
    reported = document.log().size();
  }

  Document document;
  std::size_t reported = 0;         // log groups already sent to the app
  std::vector<OpGroup> unreported;  // groups of a log that an open replaced, not yet sent
  std::optional<takeoff::RuleSet> rules;
  bool content_changed = false;
  flatbuffers::FlatBufferBuilder builder;
};

Session::Session(DeviceId device) : impl_(std::make_unique<Impl>(device)) {}

Session::~Session() = default;

std::span<const std::uint8_t> Session::handle(std::span<const std::uint8_t> request) noexcept {
  flatbuffers::FlatBufferBuilder& builder = impl_->builder;
  // Commands that ran before a failure stay applied; their changes still reach the app.
  const auto fail = [this, &builder](const char* why) noexcept -> bool {
    try {
      builder.Clear();
      const std::vector results{impl_->error(schema::ErrorCode::Internal, std::string{why})};
      impl_->finish(results);
      return true;
    } catch (...) {
      return false;
    }
  };
  try {
    builder.Clear();
    impl_->run(request);
  } catch (const std::exception& failure) {
    if (!fail(failure.what())) {
      return {};
    }
  } catch (...) {
    if (!fail("unknown failure")) {
      return {};
    }
  }
  return {builder.GetBufferPointer(), builder.GetSize()};
}

}  // namespace otves::bridge
