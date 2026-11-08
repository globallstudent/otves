#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/error.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/rules.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {
namespace {

using namespace otves::literals;

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

const std::vector<WallQuantities>& walls() {
  static const std::vector<WallQuantities> kWalls{
      {.id = id(1),
       .thickness = 380_mm,
       .footprint_area = 2.432,
       .length = 6.4,
       .face_area = 17.92,
       .volume = 6.8096},
      {.id = id(2),
       .thickness = 250_mm,
       .footprint_area = 1.0,
       .length = 4.0,
       .face_area = 11.2,
       .volume = 2.8},
  };
  return kWalls;
}

const Masonry& masonry() {
  static const Masonry kMasonry{.walls = {{.id = id(1), .units = 2685.5, .mortar = 1.6},
                                          {.id = id(2), .units = 1119, .mortar = 0.6}},
                                .issues = {},
                                .units = 3804.5,
                                .mortar = 2.2};
  return kMasonry;
}

Result<std::vector<QuantityLine>> run(const std::vector<Script>& norms,
                                      const std::vector<Script>& rules,
                                      const Settings& settings = {}) {
  auto set = RuleSet::compile({.norms = norms, .rules = rules});
  if (!set) {
    return std::unexpected{set.error()};
  }
  return set->run(walls(), masonry(), settings);
}

// A rule whose run body is `body`.
Script rule(std::string name, std::string_view body) {
  return {.name = std::move(name),
          .source =
              "return { run = function(input, norms, settings)\n" + std::string{body} + "\nend }"};
}

void expect_error(const Result<std::vector<QuantityLine>>& result, ErrorCode code,
                  std::string_view text) {
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, code);
  EXPECT_NE(result.error().message.find(text), std::string::npos) << result.error().message;
}

TEST(Rules, LinesAreSortedWithinARuleAndKeepRuleOrder) {
  const auto lines =
      run({}, {rule("second", R"(return {
                                  { item = "sand", unit = "m3", amount = 2 },
                                  { item = "cement", unit = "kg", amount = 1 } })"),
               rule("first", R"(return { { item = "brick", unit = "pcs", amount = 3 } })")});

  ASSERT_TRUE(lines.has_value()) << lines.error().message;
  const std::vector<QuantityLine> expected{
      {.rule = "second", .item = "cement", .unit = Unit::kKilogram, .amount = 1},
      {.rule = "second", .item = "sand", .unit = Unit::kCubicMetre, .amount = 2},
      {.rule = "first", .item = "brick", .unit = Unit::kPiece, .amount = 3},
  };
  EXPECT_EQ(*lines, expected);
}

TEST(Rules, InputSettingsAndNormsReachTheRule) {
  const std::vector<Script> norms{{.name = "table1", .source = "return { per_m3 = 300 }"}};
  const auto lines = run(norms, {rule("read", R"(return {
                            { item = "walls", unit = "pcs", amount = #input.walls },
                            { item = "length", unit = "m", amount = input.walls[1].length },
                            { item = "thickness", unit = "m", amount = input.walls[2].thickness },
                            { item = "units", unit = "pcs", amount = input.masonry.units },
                            { item = "wall_units", unit = "pcs", amount = input.masonry.walls[2].units },
                            { item = "norm", unit = "kg", amount = norms.table1.per_m3 },
                            { item = "waste", unit = "kg", amount = settings.waste },
                            { item = "grade", unit = "pcs", amount = if settings.grade == "M100" then 1 else 0 },
                          })")},
                         {{"waste", 0.05}, {"grade", std::string{"M100"}}});

  ASSERT_TRUE(lines.has_value()) << lines.error().message;
  const auto amount = [&](std::string_view item) {
    for (const QuantityLine& line : *lines) {
      if (line.item == item) {
        return line.amount;
      }
    }
    return -1.0;
  };
  EXPECT_EQ(amount("walls"), 2);
  EXPECT_EQ(amount("length"), 6.4);
  EXPECT_EQ(amount("thickness"), 0.25);
  EXPECT_EQ(amount("units"), 3804.5);
  EXPECT_EQ(amount("wall_units"), 1119);
  EXPECT_EQ(amount("norm"), 300);
  EXPECT_EQ(amount("waste"), 0.05);
  EXPECT_EQ(amount("grade"), 1);
}

TEST(Rules, SyntaxErrorNamesTheScriptAndLine) {
  const std::vector<Script> rules{{.name = "broken", .source = "return {\n run = function( end }"}};
  const auto set = RuleSet::compile({.norms = {}, .rules = rules});

  ASSERT_FALSE(set.has_value());
  EXPECT_EQ(set.error().code, ErrorCode::kRuleSyntax);
  EXPECT_TRUE(set.error().message.starts_with("broken:2:")) << set.error().message;
}

TEST(Rules, NamesMustBeUniqueIdentifiers) {
  const std::vector<Script> bad_name{{.name = "2nd", .source = "return {}"}};
  const std::vector<Script> twice{{.name = "a", .source = "return {}"},
                                  {.name = "a", .source = "return {}"}};

  EXPECT_EQ(RuleSet::compile({.norms = bad_name, .rules = {}}).error().code,
            ErrorCode::kRuleSyntax);
  EXPECT_EQ(RuleSet::compile({.norms = {}, .rules = twice}).error().code, ErrorCode::kRuleSyntax);
}

TEST(Rules, RuntimeErrorNamesTheScriptAndLine) {
  expect_error(run({}, {rule("failing", "error('no norm for M25')")}), ErrorCode::kRuleFailed,
               "failing:2: no norm for M25");
}

TEST(Rules, EndlessLoopRunsOutOfSteps) {
  expect_error(run({}, {rule("loop", "while true do end")}), ErrorCode::kRuleFailed,
               "loop:2: ran out of steps");
}

TEST(Rules, MemoryIsCapped) {
  expect_error(run({}, {rule("greedy", "return { string.rep('x', 64 * 1024 * 1024) }")}),
               ErrorCode::kRuleFailed, "not enough memory");
}

TEST(Rules, NondeterministicAndOutwardFunctionsAreGone) {
  const auto lines = run({}, {rule("probe", R"(
    for _, name in { "os", "io", "debug", "print", "pcall", "xpcall", "getfenv", "setfenv",
                     "require", "loadstring", "collectgarbage", "gcinfo", "newproxy",
                     "coroutine", "buffer", "bit32", "utf8", "vector", "integer" } do
      if _G[name] ~= nil then error(name .. " is reachable") end
    end
    for _, name in { "random", "randomseed", "sin", "cos", "exp", "log", "pow", "noise" } do
      if math[name] ~= nil then error("math." .. name .. " is reachable") end
    end
    return {})")});

  EXPECT_TRUE(lines.has_value()) << lines.error().message;
}

TEST(Rules, RemovedBuiltinsCannotBeCalledThroughFastPaths) {
  expect_error(
      run({}, {rule("trig", "return { { item = 'x', unit = 'pcs', amount = math.sin(1) } }")}),
      ErrorCode::kRuleFailed, "trig:2:");
}

TEST(Rules, NormsInputAndSettingsAreReadOnly) {
  const std::vector<Script> norms{{.name = "n", .source = "return { value = 1, nested = { 2 } }"}};
  const Settings settings{{"waste", 0.05}};

  for (const std::string_view write :
       {"norms.n.value = 2", "norms.n.nested[1] = 3", "norms.extra = 1", "input.masonry.units = 0",
        "input.walls[1].length = 0", "settings.waste = 0", "math.floor = nil"}) {
    expect_error(run(norms, {rule("writer", write)}, settings), ErrorCode::kRuleFailed, "readonly");
  }
}

TEST(Rules, GlobalsDoNotLeakBetweenRules) {
  const auto lines =
      run({}, {{.name = "first", .source = "leak = 1\nreturn { run = function() return {} end }"},
               rule("second", "if leak ~= nil then error('leaked') end\nreturn {}")});

  EXPECT_TRUE(lines.has_value()) << lines.error().message;
}

TEST(Rules, NormsHoldOnlyData) {
  for (const std::string_view source :
       {"return { f = function() end }", "return setmetatable({}, {})", "return { [{}] = 1 }"}) {
    const std::vector<Script> norms{{.name = "n", .source = std::string{source}}};
    expect_error(run(norms, {rule("r", "return {}")}), ErrorCode::kRuleOutput, "n: data");
  }
}

TEST(Rules, BadLinesAreRejected) {
  struct Case {
    std::string_view body;
    std::string_view message;
  };
  const std::vector<Case> cases{
      {"return 1", "array of lines"},
      {"return { a = 1 }", "array of lines"},
      {"return { 1 }", "line 1: is not a table"},
      {"return { { item = 'x', unit = 'pcs' } }", "exactly item, unit and amount"},
      {"return { { item = 'x', unit = 'pcs', amount = 1, note = 'n' } }", "exactly item"},
      {"return { { item = 'Brick', unit = 'pcs', amount = 1 } }", "item must be"},
      {"return { { item = 'x', unit = 'pieces', amount = 1 } }", "unit must be"},
      {"return { { item = 'x', unit = 'pcs', amount = -1 } }", "amount must be"},
      {"return { { item = 'x', unit = 'pcs', amount = 0 / 0 } }", "amount must be"},
      {"return { { item = 'x', unit = 'pcs', amount = '1' } }", "amount must be"},
      {"return { { item = 'x', unit = 'pcs', amount = 1 }, { item = 'x', unit = 'pcs', amount = 2 "
       "} }",
       "appears twice"},
  };
  for (const Case& c : cases) {
    expect_error(run({}, {rule("bad", c.body)}), ErrorCode::kRuleOutput, c.message);
  }
}

TEST(Rules, ModuleMustHaveARunFunction) {
  expect_error(run({}, {{.name = "empty", .source = "return {}"}}), ErrorCode::kRuleOutput,
               "run function");
}

TEST(Rules, SameInputGivesSameLines) {
  const std::vector<Script> rules{
      rule("mix",
           "local t = {}\nfor k, v in { b = 2, a = 1, c = 3 } do\n"
           "table.insert(t, { item = k, unit = 'kg', amount = v / 3 })\nend\nreturn t")};

  const auto first = run({}, rules);
  const auto second = run({}, rules);
  ASSERT_TRUE(first.has_value()) << first.error().message;
  EXPECT_EQ(*first, *second);
  EXPECT_EQ(first->front().item, "a");
}

}  // namespace
}  // namespace otves::takeoff
