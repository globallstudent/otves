#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <otves/core/error.h>
#include <otves/core/units.h>
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {

// A Luau file from content/. A norm's name is its key in the `norms` table a rule receives, so it
// must be an identifier; a rule's name tags its lines and its error messages.
struct Script {
  std::string name;
  std::string source;
};

enum class Unit : std::uint8_t {
  kPiece,        // "pcs"
  kMetre,        // "m"
  kSquareMetre,  // "m2"
  kCubicMetre,   // "m3"
  kKilogram,     // "kg"
};

struct QuantityLine {
  std::string rule;
  std::string item;  // lowercase id such as "mortar.cement"; the app maps it to a catalog entry
  Unit unit = Unit::kPiece;
  double amount = 0.0;

  auto operator<=>(const QuantityLine&) const = default;
};

// Job settings a rule may read, such as "mortar_grade" = "M100". Absent keys read as nil.
using SettingValue = std::variant<double, std::string>;
using Settings = std::map<std::string, SettingValue, std::less<>>;

// Luau bytecode of one script, built by RuleSet::compile.
struct CompiledScript {
  std::string name;
  std::string bytecode;
};

// Norms reach every rule as the `norms` table; rules run in this order.
struct Scripts {
  std::span<const Script> norms;
  std::span<const Script> rules;
};

// Compiled norm and rule scripts. Immutable, so one set can serve several threads.
class RuleSet {
 public:
  // Fails with kRuleSyntax on the first script that does not compile, or on a name that is not an
  // identifier unique among the norms or among the rules.
  [[nodiscard]] static Result<RuleSet> compile(const Scripts& scripts);

  // Runs every rule in one fresh sandbox (docs/adr/0005). Lines come grouped by rule in the order
  // the rules were given, sorted by item and unit within a rule.
  [[nodiscard]] Result<std::vector<QuantityLine>> run(const Measurement& measurement,
                                                      const Masonry& masonry,
                                                      const Settings& settings) const;

  // A length in whole millimetres from a norm table, such as {"lintel", "bearing"} in `seismic`,
  // so engine inputs that come from norms stay out of code. Fails with kNotFound for an unknown
  // norm and kRuleOutput when the path does not end at a whole number from 0 to kMaxCoordinate.
  [[nodiscard]] Result<Mm> length(std::string_view norm,
                                  std::span<const std::string_view> path) const;

 private:
  std::vector<CompiledScript> norms_;
  std::vector<CompiledScript> rules_;
};

// Lintels from content: bearings from norms.seismic.lintel, the section height from
// norms.practice.lintel.
[[nodiscard]] Result<LintelSpec> lintel_spec(const RuleSet& rules);

// Mortar joints from norms.masonry.joints. The unit is the wall's own (docs/adr/0008).
[[nodiscard]] Result<Joints> masonry_joints(const RuleSet& rules);

}  // namespace otves::takeoff
