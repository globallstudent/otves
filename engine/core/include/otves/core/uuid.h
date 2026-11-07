#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>

namespace otves {

// A 128-bit UUID. Engine ids are version 7 (RFC 9562): a 48-bit Unix millisecond timestamp
// followed by random bits, so ids created offline on two phones never collide and sort by time.
class Uuid {
 public:
  constexpr Uuid() = default;
  // High then low 64 bits, in RFC 9562 byte order.
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  constexpr Uuid(std::uint64_t high, std::uint64_t low) : high_(high), low_(low) {}

  // unix_ms must be non-negative; only the low 12 bits of random_a and 62 bits of random_b are
  // used.
  [[nodiscard]] static Uuid v7(std::int64_t unix_ms, std::uint64_t random_a,
                               std::uint64_t random_b);

  [[nodiscard]] constexpr std::uint64_t high() const { return high_; }
  [[nodiscard]] constexpr std::uint64_t low() const { return low_; }
  [[nodiscard]] constexpr bool is_nil() const { return high_ == 0 && low_ == 0; }
  [[nodiscard]] unsigned version() const;

  // Canonical 8-4-4-4-12 lowercase hex form.
  [[nodiscard]] std::string to_string() const;

  constexpr auto operator<=>(const Uuid&) const = default;

 private:
  std::uint64_t high_ = 0;
  std::uint64_t low_ = 0;
};

[[nodiscard]] std::size_t hash_value(Uuid uuid) noexcept;

// Typed wrapper, so an element id cannot be passed where a group id is expected.
template <typename Tag>
class Id {
 public:
  constexpr Id() = default;
  constexpr explicit Id(Uuid uuid) : uuid_(uuid) {}

  [[nodiscard]] constexpr Uuid uuid() const { return uuid_; }
  [[nodiscard]] constexpr bool is_nil() const { return uuid_.is_nil(); }

  constexpr auto operator<=>(const Id&) const = default;

 private:
  Uuid uuid_;
};

struct ElementTag;
struct GroupTag;
struct DeviceTag;

using ElementId = Id<ElementTag>;
using GroupId = Id<GroupTag>;
using DeviceId = Id<DeviceTag>;

struct IdHash {
  template <typename Tag>
  std::size_t operator()(Id<Tag> id) const noexcept {
    return hash_value(id.uuid());
  }
};

}  // namespace otves
