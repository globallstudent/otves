#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <variant>

#include <otves/core/element.h>
#include <otves/core/error.h>

namespace otves {

// What a job is estimated under (docs/adr/0009). A job holds at most one; each value is kUnset
// until the builder chooses, and then the norms' default applies.
struct Conditions {
  static constexpr std::int64_t kUnset = -1;

  std::int64_t seismic_intensity = kUnset;  // design intensity, points
  std::int64_t cement_grade = kUnset;       // 400 for ПЦ400
  std::int64_t mortar_grade = kUnset;       // 100 for М100
  std::int64_t unit_waste = kUnset;         // allowance on masonry units, per mille
  std::int64_t mortar_waste = kUnset;       // allowance on mortar, per mille

  constexpr bool operator==(const Conditions&) const = default;
};

// Checks values that come from the user; never asserts. Whether a grade exists is the norms'
// to say: an unknown one fails the rule that reads it.
[[nodiscard]] std::expected<void, Error> validate(const Conditions& conditions);

enum class ConditionsField : std::uint8_t {
  kSeismicIntensity,
  kCementGrade,
  kMortarGrade,
  kUnitWaste,
  kMortarWaste,
};

using ConditionsValue = std::variant<std::int64_t>;

[[nodiscard]] ConditionsValue get_field(const Conditions& conditions, ConditionsField field);
void set_field(Conditions& conditions, ConditionsField field, const ConditionsValue& value);

template <>
struct ElementTraits<Conditions> {
  using Field = ConditionsField;
  using Value = ConditionsValue;
  static constexpr std::string_view kName = "conditions";
};

}  // namespace otves
