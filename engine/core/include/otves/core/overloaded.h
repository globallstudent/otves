#pragma once

namespace otves {

// Builds a std::visit visitor from lambdas: std::visit(Overloaded{[](A) {}, [](B) {}}, value).
// The idiom needs every base's operator(), hence the multiple inheritance.
template <typename... Fn>
// NOLINTNEXTLINE(misc-multiple-inheritance)
struct Overloaded : Fn... {
  using Fn::operator()...;
};

}  // namespace otves
