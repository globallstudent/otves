#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <otves/core/assert.h>
#include <otves/core/uuid.h>

namespace otves {
namespace {

constexpr std::uint64_t kTimestampMask = 0xFFFF'FFFF'FFFFULL;  // 48 bits
constexpr unsigned kTimestampShift = 16;
constexpr std::uint64_t kVersion7 = 0x7000ULL;
constexpr unsigned kVersionShift = 12;
constexpr std::uint64_t kNibbleMask = 0xFULL;
constexpr std::uint64_t kRandomAMask = 0x0FFFULL;                    // 12 bits
constexpr std::uint64_t kVariantRfc9562 = 0x8000'0000'0000'0000ULL;  // bits 10
constexpr std::uint64_t kRandomBMask = 0x3FFF'FFFF'FFFF'FFFFULL;     // 62 bits
constexpr std::uint64_t kGoldenRatio64 = 0x9E37'79B9'7F4A'7C15ULL;

constexpr std::size_t kHexLength = 32;
constexpr std::size_t kBitsPerNibble = 4;
constexpr std::size_t kNibblesPerWord = 16;

constexpr std::array<char, 16> kHexDigits = {'0', '1', '2', '3', '4', '5', '6', '7',
                                             '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

}  // namespace

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): the random words are interchangeable
Uuid Uuid::v7(std::int64_t unix_ms, std::uint64_t random_a, std::uint64_t random_b) {
  OTVES_ASSERT(unix_ms >= 0);
  const auto timestamp = static_cast<std::uint64_t>(unix_ms) & kTimestampMask;
  return Uuid{(timestamp << kTimestampShift) | kVersion7 | (random_a & kRandomAMask),
              kVariantRfc9562 | (random_b & kRandomBMask)};
}

unsigned Uuid::version() const {
  return static_cast<unsigned>((high_ >> kVersionShift) & kNibbleMask);
}

std::string Uuid::to_string() const {
  std::string text;
  text.reserve(kHexLength + 4);
  for (std::size_t nibble = 0; nibble < kHexLength; ++nibble) {
    if (nibble == 8 || nibble == 12 || nibble == 16 || nibble == 20) {  // NOLINT(*-magic-numbers)
      text.push_back('-');
    }
    const std::uint64_t word = nibble < kNibblesPerWord ? high_ : low_;
    const std::size_t shift = (kNibblesPerWord - 1 - (nibble % kNibblesPerWord)) * kBitsPerNibble;
    text.push_back(kHexDigits.at(static_cast<std::size_t>((word >> shift) & kNibbleMask)));
  }
  return text;
}

std::size_t hash_value(Uuid uuid) noexcept {
  return static_cast<std::size_t>(uuid.high() ^ (uuid.low() * kGoldenRatio64));
}

}  // namespace otves
