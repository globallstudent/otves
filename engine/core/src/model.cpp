#include <otves/core/assert.h>
#include <otves/core/model.h>
#include <otves/core/slot_map.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>

namespace otves {

const Wall* Model::find_wall(ElementId id) const noexcept {
  const auto found = index_.find(id);
  if (found == index_.end()) {
    return nullptr;
  }
  const Entry* entry = walls_.get(found->second);
  return entry != nullptr ? &entry->wall : nullptr;
}

Wall* Model::find_wall(ElementId id) noexcept {
  const auto found = index_.find(id);
  if (found == index_.end()) {
    return nullptr;
  }
  Entry* entry = walls_.get(found->second);
  return entry != nullptr ? &entry->wall : nullptr;
}

void Model::insert_wall(ElementId id, const Wall& wall) {
  OTVES_ASSERT(!index_.contains(id));
  index_.insert_or_assign(id, walls_.insert(Entry{.id = id, .wall = wall}));
}

void Model::erase_wall(ElementId id) {
  const auto found = index_.find(id);
  OTVES_ASSERT(found != index_.end());
  if (found == index_.end()) {
    return;
  }
  walls_.erase(found->second);
  index_.erase(found);
}

}  // namespace otves
