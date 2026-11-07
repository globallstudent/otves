#pragma once

#include <cmath>
#include <compare>
#include <cstdint>

#include <otves/core/assert.h>

namespace otves {

inline constexpr double kMmPerMetre = 1000.0;

// A length typed or measured on site, in whole millimetres.
// Integer storage keeps measured values exact (6 400 stays 6 400 through any
// number of edits); geometry converts to double metres with to_metres().
class Mm {
 public:
  constexpr Mm() = default;
  constexpr explicit Mm(std::int64_t value) : value_(value) {}

  [[nodiscard]] constexpr std::int64_t value() const { return value_; }

  constexpr auto operator<=>(const Mm&) const = default;

  constexpr Mm& operator+=(Mm rhs) {
    value_ += rhs.value_;
    return *this;
  }
  constexpr Mm& operator-=(Mm rhs) {
    value_ -= rhs.value_;
    return *this;
  }

  [[nodiscard]] friend constexpr Mm operator+(Mm lhs, Mm rhs) { return lhs += rhs; }
  [[nodiscard]] friend constexpr Mm operator-(Mm lhs, Mm rhs) { return lhs -= rhs; }
  [[nodiscard]] friend constexpr Mm operator-(Mm length) { return Mm{-length.value_}; }
  [[nodiscard]] friend constexpr Mm operator*(Mm length, std::int64_t factor) {
    return Mm{length.value_ * factor};
  }
  [[nodiscard]] friend constexpr Mm operator*(std::int64_t factor, Mm length) {
    return length * factor;
  }

 private:
  std::int64_t value_ = 0;
};

[[nodiscard]] constexpr double to_metres(Mm length) {
  return static_cast<double>(length.value()) / kMmPerMetre;
}

// Rounds a computed length back to the millimetre grid used for display and storage.
[[nodiscard]] inline Mm round_to_mm(double metres) {
  OTVES_ASSERT(std::isfinite(metres));
  return Mm{std::llround(metres * kMmPerMetre)};
}

namespace literals {

[[nodiscard]] consteval Mm operator""_mm(unsigned long long value) {
  return Mm{static_cast<std::int64_t>(value)};
}

}  // namespace literals
}  // namespace otves
