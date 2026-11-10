#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
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

// Everything in content/.
RuleSet content() {
  const std::vector norms{load("norms/losses.luau"), load("norms/masonry.luau"),
                          load("norms/mortar.luau"), load("norms/practice.luau"),
                          load("norms/rebar.luau"),  load("norms/seismic.luau")};
  const std::vector rules{load("rules/masonry.luau"), load("rules/lintels.luau"),
                          load("rules/ring_beam.luau"), load("rules/junction_meshes.luau")};
  auto set = RuleSet::compile({.norms = norms, .rules = rules});
  EXPECT_TRUE(set.has_value()) << set.error().message;
  return *std::move(set);
}

ElementId id(std::int64_t n) { return ElementId{Uuid::v7(n, 0, 0)}; }

OpeningInput window(std::int64_t n, Mm width) {
  return {.id = id(n),
          .opening = {.wall = id(1),
                      .offset = 1200_mm,
                      .width = width,
                      .height = 1500_mm,
                      .sill = 900_mm,
                      .kind = OpeningKind::kWindow}};
}

// A room 6 400 × 4 000 between centre lines, 380 mm of 1НФ brick with 12/10 mm joints, 2 800 high,
// with lintels from content. Its storey asks for a 150 mm ring beam and junction meshes.
struct Room {
  Measurement measurement;
  Masonry masonry;
};

geom::WallInput wall(std::int64_t n, PointMm start, PointMm end, Mm thickness = 380_mm) {
  return {.id = id(n),
          .wall = {.start = start, .end = end, .thickness = thickness, .height = 2800_mm}};
}

Room room(const RuleSet& rules, const std::vector<OpeningInput>& openings = {},
          const std::vector<geom::WallInput>& more_walls = {}) {
  std::vector walls{
      wall(1, {0_mm, 0_mm}, {6400_mm, 0_mm}), wall(2, {6400_mm, 0_mm}, {6400_mm, 4000_mm}),
      wall(3, {6400_mm, 4000_mm}, {0_mm, 4000_mm}), wall(4, {0_mm, 4000_mm}, {0_mm, 0_mm})};
  walls.insert(walls.end(), more_walls.begin(), more_walls.end());
  const MasonrySpec brick{.unit = {.length = 250_mm, .width = 120_mm, .height = 65_mm},
                          .joints = masonry_joints(rules).value()};
  Room result;
  result.measurement =
      measure_walls(walls, openings, lintel_spec(rules).value(), geom::join_walls(walls)).value();
  for (WallQuantities& measured : result.measurement.walls) {
    measured.ring_beam = 0.15;
  }
  for (JunctionQuantities& junction : result.measurement.junctions) {
    junction.meshes = true;
  }
  result.masonry = count_masonry(result.measurement.walls, brick).value();
  return result;
}

std::vector<QuantityLine> lines_of(const std::vector<QuantityLine>& lines, std::string_view rule) {
  std::vector<QuantityLine> picked;
  for (const QuantityLine& line : lines) {
    if (line.rule == rule) {
      picked.push_back(line);
    }
  }
  return picked;
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

TEST(Content, LintelSpecComesFromNorms) {
  const auto spec = lintel_spec(content());

  ASSERT_TRUE(spec.has_value()) << spec.error().message;
  // КМК 2.01.03-19 3.5.15 bearings, 2ПБ section height.
  EXPECT_EQ(spec->bearing, 350_mm);
  EXPECT_EQ(spec->short_bearing, 250_mm);
  EXPECT_EQ(spec->short_span, 1500_mm);
  EXPECT_EQ(spec->height, 140_mm);
}

TEST(Content, JointsComeFromNorms) {
  const auto joints = masonry_joints(content());

  ASSERT_TRUE(joints.has_value()) << joints.error().message;
  // СП 70.13330.2012 9.2.4.
  EXPECT_EQ(joints->bed, 12_mm);
  EXPECT_EQ(joints->head, 10_mm);
}

TEST(Content, SeismicAdviceComesFromNorms) {
  const auto advice = seismic_advice(content());

  ASSERT_TRUE(advice.has_value()) << advice.error().message;
  // 9 points by default, detailing from 7 (КМК 2.01.03-19); walls from 250 mm bear load.
  EXPECT_TRUE(advice->required);
  EXPECT_EQ(advice->load_bearing, 250_mm);
}

TEST(Content, OnlyWhatWasChosenIsCounted) {
  const RuleSet rules = content();
  Room r = room(rules);
  // No wall has a unit, and the storey asks for no ring beam and no meshes.
  const std::vector<std::optional<MasonryUnit>> none(r.measurement.walls.size());
  r.masonry = count_masonry(r.measurement.walls, none, masonry_joints(rules).value()).value();
  for (WallQuantities& measured : r.measurement.walls) {
    measured.ring_beam = 0.0;
  }
  for (JunctionQuantities& junction : r.measurement.junctions) {
    junction.meshes = false;
  }

  const auto all = rules.run(r.measurement, r.masonry, {});

  ASSERT_TRUE(all.has_value()) << all.error().message;
  EXPECT_TRUE(all->empty());
}

TEST(Content, EachUnitSizeGetsItsOwnLine) {
  const RuleSet rules = content();
  const Room r = room(rules, {}, {wall(5, {3200_mm, 0_mm}, {3200_mm, 4000_mm}, 190_mm)});
  // The outer walls in 1НФ brick, the partition in 390 × 190 × 188 blocks.
  std::vector<std::optional<MasonryUnit>> units(
      r.measurement.walls.size(), MasonryUnit{.length = 250_mm, .width = 120_mm, .height = 65_mm});
  units.back() = MasonryUnit{.length = 390_mm, .width = 190_mm, .height = 188_mm};
  const Masonry masonry =
      count_masonry(r.measurement.walls, units, masonry_joints(rules).value()).value();

  const auto all = rules.run(r.measurement, masonry, {});

  ASSERT_TRUE(all.has_value()) << all.error().message;
  const auto lines = lines_of(*all, "masonry");
  ASSERT_EQ(lines.size(), 4U);
  ASSERT_EQ(masonry.by_unit.size(), 2U);
  EXPECT_EQ(amount(lines, "masonry_unit.250x120x65"), std::ceil(masonry.by_unit[0].units * 1.03));
  EXPECT_EQ(amount(lines, "masonry_unit.390x190x188"), std::ceil(masonry.by_unit[1].units * 1.03));
  EXPECT_DOUBLE_EQ(amount(lines, "mortar"), masonry.mortar * 1.07);
}

TEST(Content, MasonryRuleUsesPracticeDefaults) {
  const RuleSet rules = content();
  const Room r = room(rules);
  const auto all = rules.run(r.measurement, r.masonry, {});

  ASSERT_TRUE(all.has_value()) << all.error().message;
  const auto lines = lines_of(*all, "masonry");
  ASSERT_EQ(lines.size(), 3U);
  // 58.24 m² of face, three wythes: 8 727.3 net units.
  EXPECT_EQ(amount(lines, "masonry_unit.250x120x65"), std::ceil(r.masonry.units * 1.03));
  EXPECT_DOUBLE_EQ(amount(lines, "mortar"), r.masonry.mortar * 1.07);
  // СП 82-101-98: 300 kg of М400 cement per m³ of М100 mortar.
  EXPECT_DOUBLE_EQ(amount(lines, "mortar.cement"), r.masonry.mortar * 1.07 * 300);
  EXPECT_TRUE(lines_of(*all, "lintels").empty());
}

TEST(Content, MasonryRuleFollowsJobSettings) {
  const RuleSet rules = content();
  const Room r = room(rules);
  const Settings settings{{"unit_waste", 0.05},
                          {"mortar_waste", 0.1},
                          {"cement", std::string{"PC500"}},
                          {"mortar_grade", std::string{"M150"}}};
  const auto all = rules.run(r.measurement, r.masonry, settings);

  ASSERT_TRUE(all.has_value()) << all.error().message;
  const auto lines = lines_of(*all, "masonry");
  EXPECT_EQ(amount(lines, "masonry_unit.250x120x65"), std::ceil(r.masonry.units * 1.05));
  EXPECT_DOUBLE_EQ(amount(lines, "mortar.cement"), r.masonry.mortar * 1.1 * 330);
}

TEST(Content, MasonryRuleRejectsUnknownMixAndBadWaste) {
  const RuleSet rules = content();
  const Room r = room(rules);

  const auto no_norm =
      rules.run(r.measurement, r.masonry,
                {{"cement", std::string{"PC500"}}, {"mortar_grade", std::string{"M50"}}});
  ASSERT_FALSE(no_norm.has_value());
  EXPECT_EQ(no_norm.error().code, ErrorCode::kRuleFailed);
  EXPECT_NE(no_norm.error().message.find("no cement norm for PC500 in mortar M50"),
            std::string::npos);

  const auto bad_waste = rules.run(r.measurement, r.masonry, {{"unit_waste", 1.5}});
  ASSERT_FALSE(bad_waste.has_value());
  EXPECT_NE(bad_waste.error().message.find("unit_waste must be a fraction"), std::string::npos);
}

TEST(Content, WindowAndLintelSaveBricksAndMortar) {
  const RuleSet rules = content();
  const Room solid = room(rules);
  const Room with_window = room(rules, {window(10, 1500_mm)});
  const auto all = rules.run(with_window.measurement, with_window.masonry, {});

  ASSERT_TRUE(all.has_value()) << all.error().message;
  // 2.25 m² of window and 2.0 × 0.14 m of lintel are not masonry: three wythes of 0.26 × 0.077 m
  // cells each.
  const double holes = 2.25 + (2.0 * 0.14);
  EXPECT_NEAR(solid.masonry.units - with_window.masonry.units, holes * 3 / (0.26 * 0.077), 1e-6);
  EXPECT_EQ(amount(lines_of(*all, "masonry"), "masonry_unit.250x120x65"),
            std::ceil(with_window.masonry.units * 1.03));
  EXPECT_LT(with_window.masonry.mortar, solid.masonry.mortar);
}

TEST(Content, LintelsAreBarsSideBySideByLength) {
  const RuleSet rules = content();
  // 1 500 mm window: 250 mm bearings, 2.0 m bars. 1 800 mm window: 350 mm bearings, 2.5 m bars.
  OpeningInput wide = window(11, 1800_mm);
  wide.opening.offset = 3700_mm;
  const Room r = room(rules, {window(10, 1500_mm), wide});
  const auto all = rules.run(r.measurement, r.masonry, {});

  ASSERT_TRUE(all.has_value()) << all.error().message;
  EXPECT_TRUE(r.measurement.issues.empty());
  // A 380 mm wall takes three 120 mm bars with 10 mm joints.
  const std::vector<QuantityLine> expected{
      {.rule = "lintels", .item = "lintel.bar.l20", .unit = Unit::kPiece, .amount = 3},
      {.rule = "lintels", .item = "lintel.bar.l25", .unit = Unit::kPiece, .amount = 3},
  };
  EXPECT_EQ(lines_of(*all, "lintels"), expected);
}

TEST(Content, RingBeamTopsTheLoadBearingWalls) {
  const RuleSet rules = content();
  // A 120 mm partition across the room is not load-bearing and gets no ring beam.
  const Room r = room(rules, {}, {wall(5, {3200_mm, 0_mm}, {3200_mm, 4000_mm}, 120_mm)});
  const auto all = rules.run(r.measurement, r.masonry, {});

  ASSERT_TRUE(all.has_value()) << all.error().message;
  const auto lines = lines_of(*all, "ring_beam");
  ASSERT_EQ(lines.size(), 2U);
  // 20.8 m of 380 mm wall, 150 mm high, B15 with 2.8 % placement loss (Сборник потерь).
  EXPECT_NEAR(amount(lines, "concrete.b15"), 20.8 * 0.38 * 0.15 * 1.028, 1e-9);
  // 9 points: 4Ø12 A500С at 0.888 kg/m (ГОСТ 34028-2016) plus 7 % for laps.
  EXPECT_NEAR(amount(lines, "rebar.a500c.d12"), 20.8 * 4 * 0.888 * 1.07, 1e-9);
}

TEST(Content, RingBeamFollowsIntensityHeightAndConcrete) {
  const RuleSet rules = content();
  Room r = room(rules);
  for (WallQuantities& measured : r.measurement.walls) {
    measured.ring_beam = 0.25;
  }
  const Settings settings{{"seismic_intensity", 8.0}, {"ring_beam_concrete", 12.5}};
  const auto all = rules.run(r.measurement, r.masonry, settings);

  ASSERT_TRUE(all.has_value()) << all.error().message;
  const auto lines = lines_of(*all, "ring_beam");
  EXPECT_NEAR(amount(lines, "concrete.b12_5"), 20.8 * 0.38 * 0.25 * 1.028, 1e-9);
  EXPECT_NEAR(amount(lines, "rebar.a500c.d10"), 20.8 * 4 * 0.617 * 1.07, 1e-9);

  // Below 7 points the norm asks for no beam; one that was chosen is detailed as at 7.
  const auto calm = rules.run(r.measurement, r.masonry, {{"seismic_intensity", 6.0}});
  ASSERT_TRUE(calm.has_value()) << calm.error().message;
  EXPECT_NEAR(amount(lines_of(*calm, "ring_beam"), "rebar.a500c.d10"), 20.8 * 4 * 0.617 * 1.07,
              1e-9);
}

TEST(Content, RingBeamRejectsValuesBelowTheNorm) {
  const RuleSet rules = content();
  const Room r = room(rules);
  Room thin = r;
  for (WallQuantities& measured : thin.measurement.walls) {
    measured.ring_beam = 0.12;
  }

  const auto low = rules.run(thin.measurement, thin.masonry, {});
  ASSERT_FALSE(low.has_value());
  EXPECT_NE(low.error().message.find("a ring beam must be at least 150 mm high"),
            std::string::npos);
  const auto weak = rules.run(r.measurement, r.masonry, {{"ring_beam_concrete", 10.0}});
  ASSERT_FALSE(weak.has_value());
  EXPECT_NE(weak.error().message.find("ring_beam_concrete must be at least 12.5"),
            std::string::npos);
}

TEST(Content, JunctionMeshesFollowIntensity) {
  const RuleSet rules = content();
  // Four corners of 2.8 m walls; the partition's T-junction with the bottom wall gets none.
  const Room r = room(rules, {}, {wall(5, {3200_mm, 0_mm}, {3200_mm, 4000_mm}, 120_mm)});

  const auto nine = rules.run(r.measurement, r.masonry, {});
  ASSERT_TRUE(nine.has_value()) << nine.error().message;
  // 9 points: every 500 mm below the top, 5 layers at each corner.
  EXPECT_EQ(amount(lines_of(*nine, "junction_meshes"), "mesh.junction"), 4 * 5);

  const auto eight = rules.run(r.measurement, r.masonry, {{"seismic_intensity", 8.0}});
  ASSERT_TRUE(eight.has_value()) << eight.error().message;
  // 8 points: every 700 mm; the layer at 2.8 m is the ring beam, so 3 layers.
  EXPECT_EQ(amount(lines_of(*eight, "junction_meshes"), "mesh.junction"), 4 * 3);

  const auto above_nine = rules.run(r.measurement, r.masonry, {{"seismic_intensity", 10.0}});
  ASSERT_FALSE(above_nine.has_value());
  EXPECT_NE(above_nine.error().message.find("КМК 3.5.1"), std::string::npos);
}

}  // namespace
}  // namespace otves::takeoff
