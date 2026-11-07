#pragma once

#include <cstddef>
#include <unordered_map>

#include <otves/core/slot_map.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

namespace otves {

// The open model of one part. Only Document mutates it, by applying ops; everyone else reads it
// through a const reference.
class Model {
 public:
  [[nodiscard]] const Wall* find_wall(ElementId id) const noexcept;
  [[nodiscard]] Wall* find_wall(ElementId id) noexcept;
  [[nodiscard]] std::size_t wall_count() const noexcept { return walls_.size(); }

  // Visits walls in a deterministic order: it depends only on the sequence of applied ops.
  template <typename Fn>
  void for_each_wall(const Fn& fn) const {
    walls_.for_each([&fn](const Entry& entry) { fn(entry.id, entry.wall); });
  }

  // Precondition: no wall with this id exists.
  void insert_wall(ElementId id, const Wall& wall);
  // Precondition: the wall exists.
  void erase_wall(ElementId id);

 private:
  struct Entry {
    ElementId id;
    Wall wall;
  };

  SlotMap<Entry> walls_;
  std::unordered_map<ElementId, Handle, IdHash> index_;
};

}  // namespace otves
