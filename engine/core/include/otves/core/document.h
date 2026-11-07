#pragma once

#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <span>
#include <vector>

#include <otves/core/command.h>
#include <otves/core/error.h>
#include <otves/core/hlc.h>
#include <otves/core/model.h>
#include <otves/core/op.h>
#include <otves/core/uuid.h>

namespace otves {

// Clock and entropy come from outside, so tests are deterministic and the engine has no globals.
struct DocumentEnvironment {
  DeviceId device;
  std::function<std::int64_t()> unix_ms;
  std::function<std::uint64_t()> random_u64;
};

// Owns a model and its append-only operation log; the only way to change the model.
//
// A command becomes one group in the log. Between begin_gesture() and end_gesture(), all
// commands fold into a single group, so a drag records intent instead of every frame. Undo and
// redo append groups too: the log only grows, which is what persistence and sync read.
class Document {
 public:
  explicit Document(DocumentEnvironment environment);

  [[nodiscard]] const Model& model() const noexcept { return model_; }
  [[nodiscard]] std::span<const OpGroup> log() const noexcept { return log_; }

  // On error nothing changes.
  [[nodiscard]] std::expected<void, Error> execute(const Command& command);

  void begin_gesture();
  // Closes the gesture. Returns the new group, or nothing if the gesture changed nothing.
  std::optional<GroupId> end_gesture();
  [[nodiscard]] bool in_gesture() const noexcept { return gesture_.has_value(); }

  [[nodiscard]] bool can_undo() const noexcept { return !gesture_ && !undo_.empty(); }
  [[nodiscard]] bool can_redo() const noexcept { return !gesture_ && !redo_.empty(); }
  [[nodiscard]] std::expected<void, Error> undo();
  [[nodiscard]] std::expected<void, Error> redo();

  // Elements changed since the last call, sorted, for the geometry rebuild.
  [[nodiscard]] std::vector<ElementId> take_dirty();

 private:
  [[nodiscard]] std::expected<std::vector<Op>, Error> plan(const Command& command);
  void apply(const Op& op);
  void apply_all(const std::vector<Op>& ops);
  void append(CommandKind kind, GroupId origin, std::vector<Op> ops);
  [[nodiscard]] Uuid next_uuid() const;

  DocumentEnvironment environment_;
  HybridClock clock_;
  Model model_;
  std::vector<OpGroup> log_;
  std::vector<std::size_t> undo_;  // indices into log_ of groups that can be undone
  std::vector<std::size_t> redo_;
  std::optional<OpGroup> gesture_;
  std::vector<ElementId> dirty_;
};

}  // namespace otves
