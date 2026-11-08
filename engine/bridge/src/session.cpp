#include <chrono>
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
#include <otves/core/uuid.h>
#include <otves/takeoff/rules.h>

#include "convert.h"
#include "derived.h"
#include "engine_generated.h"

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
      default:
        return decode(entry).and_then(
            [this](const Command& command) { return document.execute(command); });
    }
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
        results.push_back(result(execute(*entry)));
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
    TakeoffOffsets takeoff;
    if (!changes.empty() || content_changed) {
      outlines = encode_outlines(builder, document.model());
      takeoff = encode_takeoff(builder, document.model(), rules);
      content_changed = false;
    }
    const auto response = schema::CreateResponse(builder, kVersion, builder.CreateVector(results),
                                                 builder.CreateVector(changes), document.can_undo(),
                                                 document.can_redo(), outlines, takeoff.lines,
                                                 takeoff.error, takeoff.message);
    builder.Finish(response, kIdentifier);
  }

  Document document;
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
