#include <variant>

#include <otves/core/op.h>
#include <otves/core/overloaded.h>
#include <otves/core/uuid.h>

namespace otves {

Op inverse(const Op& op) {
  return std::visit(
      Overloaded{
          [](const CreateWall& create) -> Op {
            return DeleteWall{.id = create.id, .wall = create.wall};
          },
          [](const DeleteWall& remove) -> Op {
            return CreateWall{.id = remove.id, .wall = remove.wall};
          },
          [](const SetWallField& set) -> Op {
            return SetWallField{
                .id = set.id, .field = set.field, .before = set.after, .after = set.before};
          },
      },
      op);
}

ElementId target(const Op& op) {
  return std::visit([](const auto& typed) { return typed.id; }, op);
}

}  // namespace otves
