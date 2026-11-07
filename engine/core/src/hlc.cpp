#include <algorithm>
#include <cstdint>

#include <otves/core/hlc.h>

namespace otves {
namespace {

constexpr unsigned kCounterBits = 16;

}  // namespace

Hlc HybridClock::tick(std::int64_t unix_ms) noexcept {
  const std::uint64_t physical =
      unix_ms > 0 ? static_cast<std::uint64_t>(unix_ms) << kCounterBits : 0;
  // A counter overflow carries into the millisecond bits: logical time runs slightly ahead.
  last_ = std::max(physical, last_ + 1);
  return Hlc{.value = last_};
}

}  // namespace otves
