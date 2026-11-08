#include <variant>

#include <otves/core/op.h>
#include <otves/core/overloaded.h>
#include <otves/core/uuid.h>

namespace otves {

Op inverse(const Op& op) {
  return std::visit(
      Overloaded{
          []<typename T>(const Create<T>& create) -> Op {
            return Delete<T>{.id = create.id, .element = create.element};
          },
          []<typename T>(const Delete<T>& remove) -> Op {
            return Create<T>{.id = remove.id, .element = remove.element};
          },
          []<typename T>(const SetField<T>& set) -> Op {
            return SetField<T>{
                .id = set.id, .field = set.field, .before = set.after, .after = set.before};
          },
      },
      op);
}

ElementId target(const Op& op) {
  return std::visit([](const auto& typed) { return typed.id; }, op);
}

}  // namespace otves
