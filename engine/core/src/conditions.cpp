#include <cstdint>
#include <expected>

#include <otves/core/assert.h>
#include <otves/core/conditions.h>
#include <otves/core/error.h>

#include "field.h"

namespace otves {
namespace {

// An allowance of a whole or more would at least double the order.
constexpr std::int64_t kMaxWaste = 999;

std::int64_t& member(Conditions& conditions, ConditionsField field) {
  switch (field) {
    case ConditionsField::kSeismicIntensity:
      return conditions.seismic_intensity;
    case ConditionsField::kCementGrade:
      return conditions.cement_grade;
    case ConditionsField::kMortarGrade:
      return conditions.mortar_grade;
    case ConditionsField::kUnitWaste:
      return conditions.unit_waste;
    case ConditionsField::kMortarWaste:
      return conditions.mortar_waste;
  }
  OTVES_ASSERT(false);
  return conditions.seismic_intensity;
}

}  // namespace

std::expected<void, Error> validate(const Conditions& conditions) {
  for (const std::int64_t value :
       {conditions.seismic_intensity, conditions.cement_grade, conditions.mortar_grade,
        conditions.unit_waste, conditions.mortar_waste}) {
    if (value < Conditions::kUnset) {
      return make_error(ErrorCode::kConditionOutOfRange, "a condition is below its unset value");
    }
  }
  if (conditions.unit_waste > kMaxWaste || conditions.mortar_waste > kMaxWaste) {
    return make_error(ErrorCode::kConditionOutOfRange,
                      "a waste allowance must be under 1000 per mille");
  }
  return {};
}

ConditionsValue get_field(const Conditions& conditions, ConditionsField field) {
  Conditions copy = conditions;
  return member(copy, field);
}

void set_field(Conditions& conditions, ConditionsField field, const ConditionsValue& value) {
  assign(member(conditions, field), value);
}

}  // namespace otves
