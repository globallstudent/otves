#pragma once

#include <compare>
#include <cstdint>

namespace otves {

// Hybrid logical clock stamp: Unix milliseconds in the high 48 bits, a counter in the low 16.
// Stamps from one clock grow strictly even when the wall clock stalls or steps back, which is
// what last-writer-wins merging needs.
struct Hlc {
  std::uint64_t value = 0;

  constexpr auto operator<=>(const Hlc&) const = default;
};

class HybridClock {
 public:
  [[nodiscard]] Hlc tick(std::int64_t unix_ms) noexcept;

 private:
  std::uint64_t last_ = 0;
};

}  // namespace otves
