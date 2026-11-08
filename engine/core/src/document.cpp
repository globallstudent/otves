#include <algorithm>
#include <cstddef>
#include <expected>
#include <optional>
#include <ranges>
#include <utility>
#include <variant>
#include <vector>

#include <otves/core/assert.h>
#include <otves/core/command.h>
#include <otves/core/document.h>
#include <otves/core/element.h>
#include <otves/core/error.h>
#include <otves/core/op.h>
#include <otves/core/overloaded.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

namespace otves {
namespace {

using Ops = std::vector<Op>;

// Plans a single-field change: validates the element as it would become and records
// before/after. Setting a field to its current value plans nothing.
template <typename T>
std::expected<Ops, Error> change(ElementId id, const T& current,
                                 typename ElementTraits<T>::Field field,
                                 const typename ElementTraits<T>::Value& value) {
  T changed = current;
  set_field(changed, field, value);
  if (auto valid = validate(changed); !valid) {
    return std::unexpected(std::move(valid.error()));
  }
  if (changed == current) {
    return Ops{};
  }
  return Ops{
      SetField<T>{.id = id, .field = field, .before = get_field(current, field), .after = value}};
}

// A drag sends a command every frame. Within a gesture each field keeps its first `before` and
// its latest `after`, so the log holds one change per field instead of one per frame.
template <typename T>
bool fold(OpGroup& gesture, const SetField<T>& set) {
  for (Op& existing : gesture.ops) {
    auto* other = std::get_if<SetField<T>>(&existing);
    if (other != nullptr && other->id == set.id && other->field == set.field) {
      other->after = set.after;
      return true;
    }
  }
  return false;
}

void fold_into(OpGroup& gesture, const Ops& ops) {
  for (const Op& op : ops) {
    const bool folded = std::visit(
        Overloaded{[&gesture]<typename T>(const SetField<T>& set) { return fold(gesture, set); },
                   [](const auto&) { return false; }},
        op);
    if (!folded) {
      gesture.ops.push_back(op);
    }
  }
}

bool is_no_op(const Op& op) {
  return std::visit(
      Overloaded{[]<typename T>(const SetField<T>& set) { return set.before == set.after; },
                 [](const auto&) { return false; }},
      op);
}

}  // namespace

Document::Document(DocumentEnvironment environment) : environment_(std::move(environment)) {
  OTVES_ASSERT(environment_.unix_ms && environment_.random_u64);
}

std::expected<void, Error> Document::execute(const Command& command) {
  auto ops = plan(command);
  if (!ops) {
    return std::unexpected(std::move(ops.error()));
  }
  if (ops->empty()) {
    return {};
  }
  apply_all(*ops);
  if (gesture_) {
    if (gesture_->ops.empty()) {
      gesture_->kind = kind_of(command);
    }
    fold_into(*gesture_, *ops);
    return {};
  }
  append(kind_of(command), GroupId{}, std::move(*ops));
  undo_.push_back(log_.size() - 1);
  redo_.clear();
  return {};
}

void Document::begin_gesture() {
  static_cast<void>(end_gesture());
  gesture_.emplace();
}

std::optional<GroupId> Document::end_gesture() {
  if (!gesture_) {
    return std::nullopt;
  }
  OpGroup group = std::move(*gesture_);
  gesture_.reset();
  std::erase_if(group.ops, is_no_op);
  if (group.ops.empty()) {
    return std::nullopt;
  }
  append(group.kind, GroupId{}, std::move(group.ops));
  undo_.push_back(log_.size() - 1);
  redo_.clear();
  return log_.back().id;
}

std::expected<void, Error> Document::undo() {
  if (gesture_) {
    return make_error(ErrorCode::kGestureOpen, "cannot undo during a gesture");
  }
  if (undo_.empty()) {
    return make_error(ErrorCode::kNothingToUndo, "nothing to undo");
  }
  const std::size_t index = undo_.back();
  undo_.pop_back();
  Ops ops;
  ops.reserve(log_[index].ops.size());
  for (const Op& op : std::views::reverse(log_[index].ops)) {
    ops.push_back(inverse(op));
  }
  apply_all(ops);
  const GroupId origin = log_[index].id;
  append(CommandKind::kUndo, origin, std::move(ops));
  redo_.push_back(index);
  return {};
}

std::expected<void, Error> Document::redo() {
  if (gesture_) {
    return make_error(ErrorCode::kGestureOpen, "cannot redo during a gesture");
  }
  if (redo_.empty()) {
    return make_error(ErrorCode::kNothingToRedo, "nothing to redo");
  }
  const std::size_t index = redo_.back();
  redo_.pop_back();
  Ops ops = log_[index].ops;
  apply_all(ops);
  const GroupId origin = log_[index].id;
  append(CommandKind::kRedo, origin, std::move(ops));
  undo_.push_back(index);
  return {};
}

std::vector<ElementId> Document::take_dirty() {
  std::ranges::sort(dirty_);
  const auto duplicates = std::ranges::unique(dirty_);
  dirty_.erase(duplicates.begin(), duplicates.end());
  return std::exchange(dirty_, {});
}

std::expected<Ops, Error> Document::plan(const Command& command) {
  const auto find = [this](ElementId id) -> std::expected<const Wall*, Error> {
    const Wall* wall = model_.find<Wall>(id);
    if (wall == nullptr) {
      return make_error(ErrorCode::kNotFound, "wall not found");
    }
    return wall;
  };
  const auto change_field = [&find](ElementId id, WallField field,
                                    const WallValue& value) -> std::expected<Ops, Error> {
    return find(id).and_then([&](const Wall* wall) { return change(id, *wall, field, value); });
  };

  return std::visit(Overloaded{
                        [this](const AddWall& add) -> std::expected<Ops, Error> {
                          if (auto valid = validate(add.wall); !valid) {
                            return std::unexpected(std::move(valid.error()));
                          }
                          return Ops{CreateWall{.id = ElementId{next_uuid()}, .element = add.wall}};
                        },
                        [&change_field](const MoveWallEnd& move) {
                          const WallField field =
                              move.end == WallEnd::kStart ? WallField::kStart : WallField::kEnd;
                          return change_field(move.wall, field, move.to);
                        },
                        [&change_field](const SetWallThickness& set) {
                          return change_field(set.wall, WallField::kThickness, set.thickness);
                        },
                        [&change_field](const SetWallHeight& set) {
                          return change_field(set.wall, WallField::kHeight, set.height);
                        },
                        [&change_field](const SetWallLocationLine& set) {
                          return change_field(set.wall, WallField::kLocationLine,
                                              set.location_line);
                        },
                        [&find](const RemoveWall& remove) -> std::expected<Ops, Error> {
                          return find(remove.wall).transform([&remove](const Wall* wall) {
                            return Ops{DeleteWall{.id = remove.wall, .element = *wall}};
                          });
                        },
                    },
                    command);
}

void Document::apply(const Op& op) {
  std::visit(
      Overloaded{
          [this]<typename T>(const Create<T>& create) { model_.insert(create.id, create.element); },
          [this]<typename T>(const Delete<T>& remove) { model_.erase<T>(remove.id); },
          [this]<typename T>(const SetField<T>& set) {
            T* element = model_.find<T>(set.id);
            OTVES_ASSERT(element != nullptr);
            if (element != nullptr) {
              set_field(*element, set.field, set.after);
            }
          },
      },
      op);
  dirty_.push_back(target(op));
}

void Document::apply_all(const std::vector<Op>& ops) {
  for (const Op& op : ops) {
    apply(op);
  }
}

void Document::append(CommandKind kind, GroupId origin, std::vector<Op> ops) {
  const GroupId id{next_uuid()};
  const Stamp stamp{.hlc = clock_.tick(environment_.unix_ms()), .device = environment_.device};
  log_.push_back(
      OpGroup{.id = id, .kind = kind, .stamp = stamp, .origin = origin, .ops = std::move(ops)});
}

Uuid Document::next_uuid() const {
  return Uuid::v7(environment_.unix_ms(), environment_.random_u64(), environment_.random_u64());
}

}  // namespace otves
