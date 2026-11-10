#pragma once

#include <cstddef>
#include <tuple>
#include <unordered_map>

#include <otves/core/assert.h>
#include <otves/core/conditions.h>
#include <otves/core/level.h>
#include <otves/core/opening.h>
#include <otves/core/slot_map.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

namespace otves {

// Elements of one type in one contiguous array, found by id.
template <typename T>
class ElementStore {
 public:
  [[nodiscard]] const T* find(ElementId id) const noexcept {
    const auto found = index_.find(id);
    if (found == index_.end()) {
      return nullptr;
    }
    const Entry* entry = slots_.get(found->second);
    return entry != nullptr ? &entry->element : nullptr;
  }

  [[nodiscard]] T* find(ElementId id) noexcept {
    const auto found = index_.find(id);
    if (found == index_.end()) {
      return nullptr;
    }
    Entry* entry = slots_.get(found->second);
    return entry != nullptr ? &entry->element : nullptr;
  }

  [[nodiscard]] std::size_t size() const noexcept { return slots_.size(); }

  template <typename Fn>
  void for_each(const Fn& fn) const {
    slots_.for_each([&fn](const Entry& entry) { fn(entry.id, entry.element); });
  }

  void insert(ElementId id, const T& element) {
    OTVES_ASSERT(!index_.contains(id));
    index_.insert_or_assign(id, slots_.insert(Entry{.id = id, .element = element}));
  }

  void erase(ElementId id) {
    const auto found = index_.find(id);
    OTVES_ASSERT(found != index_.end());
    if (found == index_.end()) {
      return;
    }
    slots_.erase(found->second);
    index_.erase(found);
  }

 private:
  struct Entry {
    ElementId id;
    T element;
  };

  SlotMap<Entry> slots_;
  std::unordered_map<ElementId, Handle, IdHash> index_;
};

// The open model of one part. Only Document mutates it, by applying ops; everyone else reads it
// through a const reference.
class Model {
 public:
  template <typename T>
  [[nodiscard]] const T* find(ElementId id) const noexcept {
    return store<T>().find(id);
  }

  template <typename T>
  [[nodiscard]] T* find(ElementId id) noexcept {
    return store<T>().find(id);
  }

  template <typename T>
  [[nodiscard]] std::size_t count() const noexcept {
    return store<T>().size();
  }

  // Visits elements of one type in a deterministic order: it depends only on the sequence of
  // applied ops.
  template <typename T, typename Fn>
  void for_each(const Fn& fn) const {
    store<T>().for_each(fn);
  }

  // Precondition: no element of this type has this id.
  template <typename T>
  void insert(ElementId id, const T& element) {
    store<T>().insert(id, element);
  }

  // Precondition: the element exists.
  template <typename T>
  void erase(ElementId id) {
    store<T>().erase(id);
  }

 private:
  template <typename T>
  [[nodiscard]] ElementStore<T>& store() noexcept {
    return std::get<ElementStore<T>>(stores_);
  }

  template <typename T>
  [[nodiscard]] const ElementStore<T>& store() const noexcept {
    return std::get<ElementStore<T>>(stores_);
  }

  std::tuple<ElementStore<Level>, ElementStore<Wall>, ElementStore<Opening>,
             ElementStore<Conditions>>
      stores_;
};

}  // namespace otves
