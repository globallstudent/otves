#pragma once

#include <variant>

#include <otves/core/assert.h>

namespace otves {

// Copies the alternative T out of a field value. The caller picks T from the field, so a
// mismatch is a programming error.
template <typename T, typename... Alternatives>
void assign(T& target, const std::variant<Alternatives...>& value) {
  const T* typed = std::get_if<T>(&value);
  OTVES_ASSERT(typed != nullptr);
  if (typed != nullptr) {
    target = *typed;
  }
}

}  // namespace otves
