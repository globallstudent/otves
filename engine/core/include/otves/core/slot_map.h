#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace otves {

// Generational handle into a SlotMap. A handle to an erased value never resolves again, even
// after its slot is reused.
struct Handle {
  std::uint32_t index = 0;
  std::uint32_t generation = 0;

  constexpr auto operator<=>(const Handle&) const = default;
};

// Values of one type in one contiguous array, addressed by generational handles. Iteration
// follows slot order, which depends only on the sequence of inserts and erases.
template <typename T>
class SlotMap {
 public:
  [[nodiscard]] Handle insert(T value) {
    if (!free_.empty()) {
      const std::uint32_t index = free_.back();
      free_.pop_back();
      Slot& slot = slots_[index];
      slot.value = std::move(value);
      ++size_;
      return Handle{.index = index, .generation = slot.generation};
    }
    slots_.push_back(Slot{.value = std::move(value), .generation = 0});
    ++size_;
    return Handle{.index = static_cast<std::uint32_t>(slots_.size() - 1), .generation = 0};
  }

  [[nodiscard]] T* get(Handle handle) noexcept {
    Slot* slot = live_slot(handle);
    return slot != nullptr && slot->value ? &*slot->value : nullptr;
  }

  [[nodiscard]] const T* get(Handle handle) const noexcept {
    const Slot* slot = live_slot(handle);
    return slot != nullptr && slot->value ? &*slot->value : nullptr;
  }

  bool erase(Handle handle) {
    Slot* slot = live_slot(handle);
    if (slot == nullptr) {
      return false;
    }
    slot->value.reset();
    ++slot->generation;
    free_.push_back(handle.index);
    --size_;
    return true;
  }

  [[nodiscard]] std::size_t size() const noexcept { return size_; }

  template <typename Fn>
  void for_each(const Fn& fn) const {
    for (const Slot& slot : slots_) {
      if (slot.value) {
        fn(*slot.value);
      }
    }
  }

 private:
  struct Slot {
    std::optional<T> value;
    std::uint32_t generation = 0;
  };

  [[nodiscard]] Slot* live_slot(Handle handle) noexcept {
    if (handle.index >= slots_.size()) {
      return nullptr;
    }
    Slot& slot = slots_[handle.index];
    return slot.value && slot.generation == handle.generation ? &slot : nullptr;
  }

  [[nodiscard]] const Slot* live_slot(Handle handle) const noexcept {
    if (handle.index >= slots_.size()) {
      return nullptr;
    }
    const Slot& slot = slots_[handle.index];
    return slot.value && slot.generation == handle.generation ? &slot : nullptr;
  }

  std::vector<Slot> slots_;
  std::vector<std::uint32_t> free_;
  std::size_t size_ = 0;
};

}  // namespace otves
