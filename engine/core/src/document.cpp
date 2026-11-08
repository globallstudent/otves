#include <algorithm>
#include <cstddef>
#include <expected>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <otves/core/assert.h>
#include <otves/core/command.h>
#include <otves/core/document.h>
#include <otves/core/element.h>
#include <otves/core/error.h>
#include <otves/core/model.h>
#include <otves/core/op.h>
#include <otves/core/opening.h>
#include <otves/core/overloaded.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

namespace otves {
namespace {

using Ops = std::vector<Op>;

template <typename T>
using FieldChange = std::pair<typename ElementTraits<T>::Field, typename ElementTraits<T>::Value>;

template <typename T>
std::expected<const T*, Error> find_element(const Model& model, ElementId id) {
  const T* element = model.find<T>(id);
  if (element == nullptr) {
    return make_error(ErrorCode::kNotFound, std::string{ElementTraits<T>::kName} + " not found");
  }
  return element;
}

// Plans changes to fields of one element: validates the element as it would become and records
// before/after for each field that really changes.
template <typename T>
std::expected<Ops, Error> edit(
    const Model& model, ElementId id,
    std::type_identity_t<std::initializer_list<FieldChange<T>>> changes) {
  return find_element<T>(model, id).and_then([&](const T* current) -> std::expected<Ops, Error> {
    T changed = *current;
    for (const auto& [field, value] : changes) {
      set_field(changed, field, value);
    }
    if (auto valid = validate(changed); !valid) {
      return std::unexpected(std::move(valid.error()));
    }
    Ops ops;
    for (const auto& [field, value] : changes) {
      auto before = get_field(*current, field);
      if (before != value) {
        ops.push_back(
            SetField<T>{.id = id, .field = field, .before = std::move(before), .after = value});
      }
    }
    return ops;
  });
}

// The wall's openings are deleted first, so undo recreates the wall before them.
std::expected<Ops, Error> remove_wall(const Model& model, ElementId id) {
  return find_element<Wall>(model, id).transform([&model, id](const Wall* wall) {
    std::vector<DeleteOpening> hosted;
    model.for_each<Opening>([&hosted, id](ElementId opening_id, const Opening& opening) {
      if (opening.wall == id) {
        hosted.push_back({.id = opening_id, .element = opening});
      }
    });
    std::ranges::sort(hosted, {}, &DeleteOpening::id);
    Ops ops(hosted.begin(), hosted.end());
    ops.emplace_back(DeleteWall{.id = id, .element = *wall});
    return ops;
  });
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
  const Model& model = model_;
  return std::visit(
      Overloaded{
          [this](const AddWall& add) -> std::expected<Ops, Error> {
            if (auto valid = validate(add.wall); !valid) {
              return std::unexpected(std::move(valid.error()));
            }
            return Ops{CreateWall{.id = ElementId{next_uuid()}, .element = add.wall}};
          },
          [&model](const MoveWallEnd& move) {
            const WallField field =
                move.end == WallEnd::kStart ? WallField::kStart : WallField::kEnd;
            return edit<Wall>(model, move.wall, {{field, move.to}});
          },
          [&model](const SetWallThickness& set) {
            return edit<Wall>(model, set.wall, {{WallField::kThickness, set.thickness}});
          },
          [&model](const SetWallHeight& set) {
            return edit<Wall>(model, set.wall, {{WallField::kHeight, set.height}});
          },
          [&model](const SetWallLocationLine& set) {
            return edit<Wall>(model, set.wall, {{WallField::kLocationLine, set.location_line}});
          },
          [&model](const RemoveWall& remove) { return remove_wall(model, remove.wall); },
          [this](const AddOpening& add) -> std::expected<Ops, Error> {
            if (auto valid = validate(add.opening); !valid) {
              return std::unexpected(std::move(valid.error()));
            }
            return find_element<Wall>(model_, add.opening.wall)
                .transform([this, &add](const Wall*) {
                  return Ops{CreateOpening{.id = ElementId{next_uuid()}, .element = add.opening}};
                });
          },
          [&model](const MoveOpening& move) {
            return edit<Opening>(model, move.opening, {{OpeningField::kOffset, move.offset}});
          },
          [&model](const ResizeOpening& resize) {
            return edit<Opening>(
                model, resize.opening,
                {{OpeningField::kWidth, resize.width}, {OpeningField::kHeight, resize.height}});
          },
          [&model](const SetOpeningSill& set) {
            return edit<Opening>(model, set.opening, {{OpeningField::kSill, set.sill}});
          },
          [&model](const SetOpeningKind& set) {
            return edit<Opening>(model, set.opening, {{OpeningField::kKind, set.kind}});
          },
          [&model](const RemoveOpening& remove) {
            return find_element<Opening>(model, remove.opening)
                .transform([&remove](const Opening* opening) {
                  return Ops{DeleteOpening{.id = remove.opening, .element = *opening}};
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
