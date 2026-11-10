#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <lua.h>

#include <otves/core/error.h>
#include <otves/core/overloaded.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/rules.h>
#include <otves/takeoff/wall_quantities.h>

#include "sandbox.h"

namespace otves::takeoff {
namespace {

constexpr std::size_t kMaxItemLength = 64;
constexpr int kWallFields = 10;
constexpr int kMasonryWallFields = 3;
constexpr int kUnitMasonryFields = 5;
constexpr int kLintelFields = 5;
constexpr int kJunctionFields = 3;

struct UnitName {
  std::string_view name;
  Unit unit = Unit::kPiece;
};

constexpr std::array<UnitName, 5> kUnits{{
    {.name = "pcs", .unit = Unit::kPiece},
    {.name = "m", .unit = Unit::kMetre},
    {.name = "m2", .unit = Unit::kSquareMetre},
    {.name = "m3", .unit = Unit::kCubicMetre},
    {.name = "kg", .unit = Unit::kKilogram},
}};

constexpr bool is_lower_or_digit(char c) {
  return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

constexpr bool is_identifier(std::string_view name) {
  const auto word = [](char c) {
    return is_lower_or_digit(c) || (c >= 'A' && c <= 'Z') || c == '_';
  };
  return !name.empty() && (name.front() < '0' || name.front() > '9') &&
         std::ranges::all_of(name, word);
}

constexpr bool is_item(std::string_view item) {
  const auto allowed = [](char c) { return is_lower_or_digit(c) || c == '_' || c == '.'; };
  return !item.empty() && item.size() <= kMaxItemLength && std::ranges::all_of(item, allowed);
}

Result<std::vector<CompiledScript>> compile_all(std::span<const Script> scripts,
                                                std::string_view kind) {
  std::vector<CompiledScript> compiled;
  compiled.reserve(scripts.size());
  for (const Script& script : scripts) {
    if (!is_identifier(script.name) ||
        std::ranges::contains(compiled, script.name, &CompiledScript::name)) {
      return make_error(ErrorCode::kRuleSyntax, std::string{kind} + " name '" + script.name +
                                                    "' is not a unique identifier");
    }
    auto bytecode = compile_luau(script);
    if (!bytecode) {
      return std::unexpected{std::move(bytecode).error()};
    }
    compiled.push_back({.name = script.name, .bytecode = *std::move(bytecode)});
  }
  return compiled;
}

void push_string(lua_State* state, std::string_view value) {
  lua_pushlstring(state, value.data(), value.size());
}

void set_field(lua_State* state, const char* key, double value) {
  lua_pushnumber(state, value);
  lua_rawsetfield(state, -2, key);
}

void set_field(lua_State* state, const char* key, std::string_view value) {
  push_string(state, value);
  lua_rawsetfield(state, -2, key);
}

// Ends a table under construction: read-only, then stored under `key` in the table below it.
void close_table(lua_State* state, const char* key) {
  lua_setreadonly(state, -1, 1);
  lua_rawsetfield(state, -2, key);
}

void close_element(lua_State* state, std::size_t index) {
  lua_setreadonly(state, -1, 1);
  lua_rawseti(state, -2, static_cast<int>(index + 1));
}

// input = { walls = { { id, thickness, footprint_area, length, face_area, volume, openings_area,
//                       ring_beam, footing_width, footing_depth } },
//           lintels = { { opening, wall, length, height, thickness } },
//           junctions = { { walls = { id }, height, meshes } },
//           masonry = { units, mortar, walls = { { id, units, mortar } },
//                       by_unit = { { length, width, height, units, mortar } } } },
// in metres, except unit sizes, which are whole millimetres.
void push_input(lua_State* state, const Measurement& measurement, const Masonry& masonry) {
  const std::span<const WallQuantities> walls = measurement.walls;
  const std::span<const Lintel> lintels = measurement.lintels;
  lua_createtable(state, 0, 4);

  lua_createtable(state, static_cast<int>(walls.size()), 0);
  for (std::size_t i = 0; i < walls.size(); ++i) {
    const WallQuantities& wall = walls[i];
    lua_createtable(state, 0, kWallFields);
    set_field(state, "id", wall.id.uuid().to_string());
    set_field(state, "thickness", to_metres(wall.thickness));
    set_field(state, "footprint_area", wall.footprint_area);
    set_field(state, "length", wall.length);
    set_field(state, "face_area", wall.face_area);
    set_field(state, "volume", wall.volume);
    set_field(state, "openings_area", wall.openings_area);
    set_field(state, "ring_beam", wall.ring_beam);
    set_field(state, "footing_width", wall.footing_width);
    set_field(state, "footing_depth", wall.footing_depth);
    close_element(state, i);
  }
  close_table(state, "walls");

  lua_createtable(state, static_cast<int>(lintels.size()), 0);
  for (std::size_t i = 0; i < lintels.size(); ++i) {
    const Lintel& lintel = lintels[i];
    lua_createtable(state, 0, kLintelFields);
    set_field(state, "opening", lintel.opening.uuid().to_string());
    set_field(state, "wall", lintel.wall.uuid().to_string());
    set_field(state, "length", lintel.length);
    set_field(state, "height", lintel.height);
    set_field(state, "thickness", to_metres(lintel.thickness));
    close_element(state, i);
  }
  close_table(state, "lintels");

  const std::span<const JunctionQuantities> junctions = measurement.junctions;
  lua_createtable(state, static_cast<int>(junctions.size()), 0);
  for (std::size_t i = 0; i < junctions.size(); ++i) {
    const JunctionQuantities& junction = junctions[i];
    lua_createtable(state, 0, kJunctionFields);
    lua_createtable(state, static_cast<int>(junction.walls.size()), 0);
    for (std::size_t w = 0; w < junction.walls.size(); ++w) {
      push_string(state, junction.walls[w].uuid().to_string());
      lua_rawseti(state, -2, static_cast<int>(w + 1));
    }
    close_table(state, "walls");
    set_field(state, "height", junction.height);
    lua_pushboolean(state, junction.meshes ? 1 : 0);
    lua_rawsetfield(state, -2, "meshes");
    close_element(state, i);
  }
  close_table(state, "junctions");

  lua_createtable(state, 0, 4);
  set_field(state, "units", masonry.units);
  set_field(state, "mortar", masonry.mortar);
  lua_createtable(state, static_cast<int>(masonry.walls.size()), 0);
  for (std::size_t i = 0; i < masonry.walls.size(); ++i) {
    const WallMasonry& wall = masonry.walls[i];
    lua_createtable(state, 0, kMasonryWallFields);
    set_field(state, "id", wall.id.uuid().to_string());
    set_field(state, "units", wall.units);
    set_field(state, "mortar", wall.mortar);
    close_element(state, i);
  }
  close_table(state, "walls");
  lua_createtable(state, static_cast<int>(masonry.by_unit.size()), 0);
  for (std::size_t i = 0; i < masonry.by_unit.size(); ++i) {
    const UnitMasonry& of_unit = masonry.by_unit[i];
    lua_createtable(state, 0, kUnitMasonryFields);
    set_field(state, "length", static_cast<double>(of_unit.unit.length.value()));
    set_field(state, "width", static_cast<double>(of_unit.unit.width.value()));
    set_field(state, "height", static_cast<double>(of_unit.unit.height.value()));
    set_field(state, "units", of_unit.units);
    set_field(state, "mortar", of_unit.mortar);
    close_element(state, i);
  }
  close_table(state, "by_unit");
  close_table(state, "masonry");

  lua_setreadonly(state, -1, 1);
}

void push_settings(lua_State* state, const Settings& settings) {
  lua_createtable(state, 0, static_cast<int>(settings.size()));
  for (const auto& [key, value] : settings) {
    push_string(state, key);
    std::visit(Overloaded{[state](double number) { lua_pushnumber(state, number); },
                          [state](const std::string& text) { push_string(state, text); }},
               value);
    lua_rawset(state, -3);
  }
  lua_setreadonly(state, -1, 1);
}

int count_keys(lua_State* state, int table) {
  int count = 0;
  lua_pushnil(state);
  while (lua_next(state, table) != 0) {
    ++count;
    lua_pop(state, 1);
  }
  return count;
}

std::optional<std::string> raw_string(lua_State* state, int table, const char* key) {
  std::optional<std::string> value;
  if (lua_rawgetfield(state, table, key) == LUA_TSTRING) {
    std::size_t length = 0;
    const char* text = lua_tolstring(state, -1, &length);
    value.emplace(text, length);
  }
  lua_pop(state, 1);
  return value;
}

std::optional<Unit> parse_unit(std::string_view name) {
  for (const UnitName& unit : kUnits) {
    if (unit.name == name) {
      return unit.unit;
    }
  }
  return std::nullopt;
}

std::optional<double> raw_number(lua_State* state, int table, const char* key) {
  std::optional<double> value;
  if (lua_rawgetfield(state, table, key) == LUA_TNUMBER) {
    value = lua_tonumber(state, -1);
  }
  lua_pop(state, 1);
  return value;
}

// Reads the line on top of the stack.
Result<QuantityLine> read_line(lua_State* state, std::string_view rule, int number) {
  const auto fail = [&](std::string_view what) {
    return make_error(
        ErrorCode::kRuleOutput,
        std::string{rule} + ": line " + std::to_string(number) + ": " + std::string{what});
  };
  if (!lua_istable(state, -1)) {
    return fail("is not a table");
  }
  const int table = lua_gettop(state);
  if (count_keys(state, table) != 3) {
    return fail("needs exactly item, unit and amount");
  }
  const std::optional<std::string> item = raw_string(state, table, "item");
  if (!item || !is_item(*item)) {
    return fail("item must be 1 to 64 characters of a-z, 0-9, '_' and '.'");
  }
  const std::optional<std::string> unit_name = raw_string(state, table, "unit");
  const std::optional<Unit> unit = unit_name ? parse_unit(*unit_name) : std::nullopt;
  if (!unit) {
    return fail("unit must be pcs, m, m2, m3 or kg");
  }
  const std::optional<double> amount = raw_number(state, table, "amount");
  if (!amount || !std::isfinite(*amount) || *amount < 0.0) {
    return fail("amount must be a finite number, zero or more");
  }
  return QuantityLine{.rule = std::string{rule}, .item = *item, .unit = *unit, .amount = *amount};
}

// Reads the array of lines on top of the stack, sorted by item and unit.
Result<std::vector<QuantityLine>> read_lines(lua_State* state, std::string_view rule) {
  const int table = lua_gettop(state);
  const int count = lua_istable(state, table) ? lua_objlen(state, table) : -1;
  if (count < 0 || count_keys(state, table) != count) {
    return make_error(ErrorCode::kRuleOutput,
                      std::string{rule} + ": run must return an array of lines");
  }
  std::vector<QuantityLine> lines;
  lines.reserve(static_cast<std::size_t>(count));
  for (int i = 1; i <= count; ++i) {
    lua_rawgeti(state, table, i);
    auto line = read_line(state, rule, i);
    lua_pop(state, 1);
    if (!line) {
      return std::unexpected{std::move(line).error()};
    }
    lines.push_back(*std::move(line));
  }
  const auto key = [](const QuantityLine& line) { return std::tie(line.item, line.unit); };
  std::ranges::sort(lines, {}, key);
  const auto twin = std::ranges::adjacent_find(lines, {}, key);
  if (twin != lines.end()) {
    return make_error(ErrorCode::kRuleOutput,
                      std::string{rule} + ": item '" + twin->item + "' appears twice in one unit");
  }
  return lines;
}

// Runs one rule module: its top level returns { run = function(input, norms, settings) }.
Result<std::vector<QuantityLine>> run_rule(Sandbox& sandbox, const CompiledScript& rule, int input,
                                           int norms, int settings) {
  lua_State* state = sandbox.state();
  if (auto loaded = sandbox.load(rule.name, rule.bytecode); !loaded) {
    return std::unexpected{std::move(loaded).error()};
  }
  const int module = lua_gettop(state);
  if (!lua_istable(state, module) || lua_rawgetfield(state, module, "run") != LUA_TFUNCTION) {
    lua_settop(state, module - 1);
    return make_error(ErrorCode::kRuleOutput,
                      rule.name + ": must return a table with a run function");
  }
  lua_pushvalue(state, input);
  lua_pushvalue(state, norms);
  lua_pushvalue(state, settings);
  if (auto called = sandbox.call(rule.name, 3); !called) {
    lua_settop(state, module - 1);
    return std::unexpected{std::move(called).error()};
  }
  auto lines = read_lines(state, rule.name);
  lua_settop(state, module - 1);
  return lines;
}

}  // namespace

Result<RuleSet> RuleSet::compile(const Scripts& scripts) {
  auto compiled_norms = compile_all(scripts.norms, "norm");
  if (!compiled_norms) {
    return std::unexpected{std::move(compiled_norms).error()};
  }
  auto compiled_rules = compile_all(scripts.rules, "rule");
  if (!compiled_rules) {
    return std::unexpected{std::move(compiled_rules).error()};
  }
  RuleSet set;
  set.norms_ = *std::move(compiled_norms);
  set.rules_ = *std::move(compiled_rules);
  return set;
}

Result<std::vector<QuantityLine>> RuleSet::run(const Measurement& measurement,
                                               const Masonry& masonry,
                                               const Settings& settings) const {
  Sandbox sandbox;
  lua_State* state = sandbox.state();
  std::vector<QuantityLine> lines;
  Result<void> outcome;
  std::function<void()> body = [&] {
    push_input(state, measurement, masonry);
    const int input = lua_gettop(state);
    push_settings(state, settings);
    const int settings_index = lua_gettop(state);

    lua_createtable(state, 0, static_cast<int>(norms_.size()));
    const int norms = lua_gettop(state);
    for (const CompiledScript& norm : norms_) {
      outcome = sandbox.load(norm.name, norm.bytecode).and_then([&] {
        return sandbox.freeze_data(norm.name);
      });
      if (!outcome) {
        return;
      }
      lua_rawsetfield(state, norms, norm.name.c_str());
    }
    lua_setreadonly(state, norms, 1);

    for (const CompiledScript& rule : rules_) {
      auto rule_lines = run_rule(sandbox, rule, input, norms, settings_index);
      if (!rule_lines) {
        outcome = std::unexpected{std::move(rule_lines).error()};
        return;
      }
      lines.insert(lines.end(), std::make_move_iterator(rule_lines->begin()),
                   std::make_move_iterator(rule_lines->end()));
    }
  };
  if (auto ran = sandbox.protect(body); !ran) {
    return std::unexpected{std::move(ran).error()};
  }
  if (!outcome) {
    return std::unexpected{std::move(outcome).error()};
  }
  return lines;
}

Result<Mm> RuleSet::length(std::string_view norm, std::span<const std::string_view> path) const {
  const auto found = std::ranges::find(norms_, norm, &CompiledScript::name);
  if (found == norms_.end()) {
    return make_error(ErrorCode::kNotFound, "norm '" + std::string{norm} + "' not found");
  }
  std::string where = found->name;
  for (const std::string_view key : path) {
    where += "." + std::string{key};
  }

  Sandbox sandbox;
  lua_State* state = sandbox.state();
  std::optional<double> value;
  Result<void> outcome;
  std::function<void()> body = [&] {
    outcome = sandbox.load(found->name, found->bytecode);
    if (!outcome) {
      return;
    }
    for (const std::string_view key : path) {
      if (!lua_istable(state, -1)) {
        return;
      }
      push_string(state, key);
      lua_rawget(state, -2);
      lua_remove(state, -2);
    }
    if (lua_type(state, -1) == LUA_TNUMBER) {
      value = lua_tonumber(state, -1);
    }
  };
  if (auto ran = sandbox.protect(body); !ran) {
    return std::unexpected{std::move(ran).error()};
  }
  if (!outcome) {
    return std::unexpected{std::move(outcome).error()};
  }
  const auto max = static_cast<double>(kMaxCoordinate.value());
  if (!value || *value < 0.0 || *value > max || *value != std::floor(*value)) {
    return make_error(ErrorCode::kRuleOutput, where + " must be a whole number of millimetres");
  }
  return Mm{static_cast<std::int64_t>(*value)};
}

Result<LintelSpec> lintel_spec(const RuleSet& rules) {
  const auto read = [&rules](std::string_view norm, std::string_view key) {
    const std::array<std::string_view, 2> path{"lintel", key};
    return rules.length(norm, path);
  };
  const Result<Mm> height = read("practice", "height");
  const Result<Mm> bearing = read("seismic", "bearing");
  const Result<Mm> short_bearing = read("seismic", "short_bearing");
  const Result<Mm> short_span = read("seismic", "short_span");
  for (const Result<Mm>* value : {&height, &bearing, &short_bearing, &short_span}) {
    if (!*value) {
      return std::unexpected{value->error()};
    }
  }
  const LintelSpec spec{.height = *height,
                        .bearing = *bearing,
                        .short_bearing = *short_bearing,
                        .short_span = *short_span};
  if (auto valid = validate(spec); !valid) {
    return std::unexpected{std::move(valid).error()};
  }
  return spec;
}

Result<SeismicAdvice> seismic_advice(const RuleSet& rules, std::optional<std::int64_t> chosen) {
  // Intensities are whole points; `length` reads any whole number.
  const std::array<std::string_view, 1> from_path{"detailing_from"};
  const std::array<std::string_view, 2> intensity_path{"defaults", "seismic_intensity"};
  const std::array<std::string_view, 2> thickness_path{"walls", "load_bearing_min_thickness"};
  const Result<Mm> from = rules.length("seismic", from_path);
  const Result<Mm> intensity = rules.length("practice", intensity_path);
  const Result<Mm> thickness = rules.length("practice", thickness_path);
  for (const Result<Mm>* value : {&from, &intensity, &thickness}) {
    if (!*value) {
      return std::unexpected{value->error()};
    }
  }
  return SeismicAdvice{.required = chosen.value_or(intensity->value()) >= from->value(),
                       .load_bearing = *thickness};
}

Result<Joints> masonry_joints(const RuleSet& rules) {
  const auto joint = [&rules](std::string_view key) {
    const std::array<std::string_view, 2> path{"joints", key};
    return rules.length("masonry", path);
  };
  const Result<Mm> bed = joint("bed");
  const Result<Mm> head = joint("head");
  for (const Result<Mm>* value : {&bed, &head}) {
    if (!*value) {
      return std::unexpected{value->error()};
    }
  }
  return Joints{.bed = *bed, .head = *head};
}

}  // namespace otves::takeoff
