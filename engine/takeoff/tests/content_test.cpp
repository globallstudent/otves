#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <otves/core/error.h>
#include <otves/core/opening.h>
#include <otves/core/point.h>
#include <otves/core/units.h>
#include <otves/core/uuid.h>
#include <otves/geom/wall_joins.h>
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/rules.h>
#include <otves/takeoff/wall_quantities.h>

namespace otves::takeoff {
namespace {

using namespace otves::literals;

Script load(const std::filesystem::path& file) {
  std::ifstream in{std::filesystem::path{OTVES_CONTENT_DIR} / file};
  EXPECT_TRUE(in.good()) << file;
  return {
      .name = file.stem().string(),
      .source = std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}}};
}

RuleSet masonry_rules() {
  const std::vector norms{load("norms/practice.luau"), load("norms/sp_82_101_98.luau")};
  const std::vector rules{load("rules/masonry.luau")};
  auto set = RuleSet::compile({.norms = norms, .rules = rules});
  EXPECT_TRUE(set.has_value()) << set.error().message;
  return *std::move(set);
}

// A room 6 400 × 4 000 between centre lines, 380 mm of 1НФ brick with 12/10 mm joints, 2 800 high.
struct Room {
  std::vector<WallQuantities> walls;
  Masonry masonry;
};

Room room(const std::vector<OpeningInput>& openings = {}) {
  const auto wall = [](std::int64_t n, PointMm start, PointMm end) {
    return geom::WallInput{
        .id = ElementId{Uuid::v7(n, 0, 0)},
        .wall = {.start = start, .end = end, .thickness = 380_mm, .height = 2800_mm}};
  };
  const std::vector walls{
      wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}), wall(2, {6400_mm, 0_mm}, {6400_mm, 4000_mm}),
      wall(3, {6400_mm, 4000_mm}, {0_mm, 4000_mm}), wall(4, {0_mm, 4000_mm}, {0_mm, 0_mm})};
  const MasonrySpec brick{.unit = {.length = 250_mm, .width = 120_mm, .height = 65_mm},
                          .joints = {.bed = 12_mm, .head = 10_mm}};
  Room result;
  result.walls = measure_walls(walls, openings, geom::join_walls(walls)).walls;
  result.masonry = count_masonry(result.walls, brick).value();
  return result;
}

double amount(const std::vector<QuantityLine>& lines, std::string_view item) {
  for (const QuantityLine& line : lines) {
    if (line.item == item) {
      return line.amount;
    }
  }
  ADD_FAILURE() << "no line " << item;
  return 0.0;
}

TEST(Content, MasonryRuleUsesPracticeDefaults) {
  const Room r = room();
  const auto lines = masonry_rules().run(r.walls, r.masonry, {});

  ASSERT_TRUE(lines.has_value()) << lines.error().message;
  ASSERT_EQ(lines->size(), 3U);
  // 58.24 m² of face, three wythes: 8 727.3 net units.
  EXPECT_EQ(amount(*lines, "masonry_unit"), std::ceil(r.masonry.units * 1.03));
  EXPECT_DOUBLE_EQ(amount(*lines, "mortar"), r.masonry.mortar * 1.07);
  // СП 82-101-98: 300 kg of М400 cement per m³ of М100 mortar.
  EXPECT_DOUBLE_EQ(amount(*lines, "mortar.cement"), r.masonry.mortar * 1.07 * 300);
}

TEST(Content, MasonryRuleFollowsJobSettings) {
  const Room r = room();
  const Settings settings{{"unit_waste", 0.05},
                          {"mortar_waste", 0.1},
                          {"cement", std::string{"PC500"}},
                          {"mortar_grade", std::string{"M150"}}};
  const auto lines = masonry_rules().run(r.walls, r.masonry, settings);

  ASSERT_TRUE(lines.has_value()) << lines.error().message;
  EXPECT_EQ(amount(*lines, "masonry_unit"), std::ceil(r.masonry.units * 1.05));
  EXPECT_DOUBLE_EQ(amount(*lines, "mortar.cement"), r.masonry.mortar * 1.1 * 330);
}

TEST(Content, MasonryRuleRejectsUnknownMixAndBadWaste) {
  const Room r = room();
  const RuleSet rules = masonry_rules();

  const auto no_norm = rules.run(
      r.walls, r.masonry, {{"cement", std::string{"PC500"}}, {"mortar_grade", std::string{"M50"}}});
  ASSERT_FALSE(no_norm.has_value());
  EXPECT_EQ(no_norm.error().code, ErrorCode::kRuleFailed);
  EXPECT_NE(no_norm.error().message.find("no cement norm for PC500 in mortar M50"),
            std::string::npos);

  const auto bad_waste = rules.run(r.walls, r.masonry, {{"unit_waste", 1.5}});
  ASSERT_FALSE(bad_waste.has_value());
  EXPECT_NE(bad_waste.error().message.find("unit_waste must be a fraction"), std::string::npos);
}

TEST(Content, WindowSavesBricksAndMortar) {
  const OpeningInput window{.id = ElementId{Uuid::v7(10, 0, 0)},
                            .opening = {.wall = ElementId{Uuid::v7(1, 0, 0)},
                                        .offset = 1200_mm,
                                        .width = 1500_mm,
                                        .height = 1500_mm,
                                        .sill = 900_mm,
                                        .kind = OpeningKind::kWindow}};
  const Room solid = room();
  const Room with_window = room({window});
  const auto lines = masonry_rules().run(with_window.walls, with_window.masonry, {});

  ASSERT_TRUE(lines.has_value()) << lines.error().message;
  // 2.25 m² less face, three wythes of 0.26 × 0.077 m cells.
  EXPECT_NEAR(solid.masonry.units - with_window.masonry.units, 2.25 * 3 / (0.26 * 0.077), 1e-6);
  EXPECT_EQ(amount(*lines, "masonry_unit"), std::ceil(with_window.masonry.units * 1.03));
  EXPECT_LT(with_window.masonry.mortar, solid.masonry.mortar);
}

}  // namespace
}  // namespace otves::takeoff
