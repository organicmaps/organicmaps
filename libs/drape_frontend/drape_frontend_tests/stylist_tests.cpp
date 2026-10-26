#include "testing/testing.hpp"

#include "drape_frontend/stylist.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_object.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "drape/hatching_decl.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <map>
#include <vector>

namespace
{
auto constexpr kStyles = []
{
  std::array<MapStyle, MapStyleCount - 1> styles{};
  for (size_t i = 0, s = 0; s < MapStyleCount; ++s)
    if (auto const style = static_cast<MapStyle>(s); style != MapStyleMerged)
      styles[i++] = style;
  return styles;
}();

class WaterArea : public osm::MapObject
{
public:
  explicit WaterArea(std::vector<uint32_t> const & types)
  {
    m_geomType = feature::GeomType::Area;
    m_triangles = {{0.0, 0.0}, {0.01, 0.0}, {0.0, 0.01}};
    for (auto type : types)
      m_types.Add(type);
  }
};

class SampleFeature : public osm::MapObject
{
public:
  SampleFeature(uint32_t type, feature::GeomType geomType)
  {
    m_geomType = geomType;
    m_points = {{0.0, 0.0}, {0.01, 0.01}};
    m_triangles = {{0.0, 0.0}, {0.01, 0.0}, {0.0, 0.01}};
    m_types.Add(type);
    m_name.Add("en", "Sample");
    m_houseNumber = "12";
  }
};

class StyleSnapshotFixture
{
public:
  StyleSnapshotFixture() : m_originalStyle(GetStyleReader().GetCurrentStyle())
  {
    classificator::Load();
    for (auto const style : kStyles)
      classificator::EnsureStyleLoaded(style);
  }

  ~StyleSnapshotFixture() { GetStyleReader().SetCurrentStyle(m_originalStyle); }

private:
  MapStyle const m_originalStyle;
};

void TestSameRules(df::Stylist const & actual, df::Stylist const & expected)
{
  TEST_EQUAL(actual.m_symbolRule, expected.m_symbolRule, ());
  TEST_EQUAL(actual.m_captionRule, expected.m_captionRule, ());
  TEST_EQUAL(actual.m_houseNumberRule, expected.m_houseNumberRule, ());
  TEST_EQUAL(actual.m_pathtextRule, expected.m_pathtextRule, ());
  TEST_EQUAL(actual.m_shieldRule, expected.m_shieldRule, ());
  TEST_EQUAL(actual.m_areaRule, expected.m_areaRule, ());
  TEST_EQUAL(actual.m_hatchingRule, expected.m_hatchingRule, ());
  TEST_EQUAL(actual.m_areaPattern, expected.m_areaPattern, ());
  TEST_EQUAL(actual.m_hatchingPattern, expected.m_hatchingPattern, ());
  TEST(std::equal(actual.m_lineRules.begin(), actual.m_lineRules.end(), expected.m_lineRules.begin(),
                  expected.m_lineRules.end()),
       ());
}

void TestAreaPatterns(MapStyle style, uint8_t zoom, std::initializer_list<uint32_t> types, uint32_t fillType,
                      std::string_view fillPattern, uint32_t hatchType = 0, std::string_view hatchPattern = {})
{
  std::vector<uint32_t> ordered(types);
  for (bool reverse : {false, true})
  {
    if (reverse)
      std::reverse(ordered.begin(), ordered.end());
    auto feature = FeatureType::CreateFromMapObject(WaterArea(ordered));
    df::Stylist stylist(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
    drule::AreaRule const * expectedFill = nullptr;
    drule::AreaRule const * expectedHatch = nullptr;
    if (fillType != 0)
    {
      auto single = FeatureType::CreateFromMapObject(WaterArea({fillType}));
      df::Stylist singleStyle(*single, zoom, StringUtf8Multilang::kEnglishCode, style);
      expectedFill = singleStyle.m_areaRule;
      TEST(expectedFill != nullptr, (style, zoom));
    }
    if (hatchType != 0)
    {
      auto single = FeatureType::CreateFromMapObject(WaterArea({hatchType}));
      df::Stylist singleStyle(*single, zoom, StringUtf8Multilang::kEnglishCode, style);
      expectedHatch = singleStyle.m_hatchingRule;
      TEST(expectedHatch != nullptr, (style, zoom));
    }
    TEST_EQUAL(stylist.m_areaRule, expectedFill, (style, zoom, reverse));
    TEST_EQUAL(stylist.m_areaPattern, fillPattern, (style, zoom, reverse));
    TEST_EQUAL(stylist.m_hatchingRule, expectedHatch, (style, zoom, reverse));
    TEST_EQUAL(stylist.m_hatchingPattern, hatchPattern, (style, zoom, reverse));
  }
}
}  // namespace

UNIT_TEST(Stylist_IsHatching)
{
  classificator::Load();
  auto const & cl = classif();

  auto const & checker = df::IsHatchingTerritoryChecker::Instance();

  TEST(checker(cl.GetTypeByPath({"boundary", "protected_area", "1"})), ());
  TEST(!checker(cl.GetTypeByPath({"boundary", "protected_area", "2"})), ());
  TEST(!checker(cl.GetTypeByPath({"boundary", "protected_area"})), ());

  TEST(checker(cl.GetTypeByPath({"boundary", "national_park"})), ());

  TEST(checker(cl.GetTypeByPath({"landuse", "military", "danger_area"})), ());

  TEST(checker(cl.GetTypeByPath({"amenity", "prison"})), ());
}

UNIT_TEST(Stylist_IsAreaPattern)
{
  classificator::Load();
  auto const & cl = classif();

  // Resolving the singleton's type paths asserts in Debug if a path is absent from the classificator.
  auto const & checker = df::IsAreaPatternChecker::Instance();

  // Stipple: sandy / desert surfaces and intermittent water. natural=sand is a beach subtype, matched via the parent.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach", "sand"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach", "gravel"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "desert"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "water", "intermittent"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "basin", "intermittent"})), dp::kStipplePattern, ());

  // Speckle: rocky and stony surfaces, quarries.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "scree"})), dp::kSpecklePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "bare_rock"})), dp::kSpecklePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "shingle"})), dp::kSpecklePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "quarry"})), dp::kSpecklePattern, ());

  // Grid: planted landuse.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "orchard"})), dp::kGridPattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "vineyard"})), dp::kGridPattern, ());

  // Permanent water and unrelated area types get no pattern.
  TEST(checker.GetPattern(cl.GetTypeByPath({"natural", "water"})).empty(), ());
  TEST(checker.GetPattern(cl.GetTypeByPath({"landuse", "basin"})).empty(), ());
  // Mine sites share the quarry fill, but they outline whole mines, whose pits are mapped as quarries.
  TEST(checker.GetPattern(cl.GetTypeByPath({"landuse", "industrial", "mine"})).empty(), ());
}

UNIT_TEST(Stylist_AreaPatternsFollowSelectedRules)
{
  auto & reader = GetStyleReader();
  for (size_t s = 0; s < MapStyleCount; ++s)
  {
    auto const style = static_cast<MapStyle>(s);
    if (style == MapStyleMerged)
      continue;
    reader.SetCurrentStyle(style);
    classificator::Load();
    auto const & cl = classif();
    auto const quarry = cl.GetTypeByPath({"landuse", "quarry"});
    auto const mine = cl.GetTypeByPath({"landuse", "industrial", "mine"});
    auto const shingle = cl.GetTypeByPath({"natural", "shingle"});
    auto const lake = cl.GetTypeByPath({"natural", "water", "lake"});
    auto const intermittent = cl.GetTypeByPath({"natural", "water", "intermittent"});
    auto const intermittentBasin = cl.GetTypeByPath({"landuse", "basin", "intermittent"});
    auto const ditch = cl.GetTypeByPath({"natural", "water", "ditch"});
    auto const park = cl.GetTypeByPath({"leisure", "park"});
    auto const scrub = cl.GetTypeByPath({"natural", "scrub"});
    auto const reserve = cl.GetTypeByPath({"leisure", "nature_reserve"});
    auto const bog = cl.GetTypeByPath({"natural", "wetland", "bog"});
    auto const beach = cl.GetTypeByPath({"natural", "beach"});
    auto const resort = cl.GetTypeByPath({"leisure", "beach_resort"});
    auto const orchard = cl.GetTypeByPath({"landuse", "orchard"});
    auto const vineyard = cl.GetTypeByPath({"landuse", "vineyard"});
    bool const vehicle = style == MapStyleVehicleLight || style == MapStyleVehicleDark;

    // A surface pattern belongs to its selected fill, including at zooms where the surface is invisible.
    TestAreaPatterns(style, 16, {quarry, lake}, lake, {});
    TestAreaPatterns(style, 16, {quarry, scrub}, scrub, {});
    TestAreaPatterns(style, 12, {quarry, lake}, lake, {});
    TestAreaPatterns(style, 11, {shingle, park}, park, {});
    TestAreaPatterns(style, 16, {shingle, park}, vehicle ? park : shingle,
                     vehicle ? std::string_view{} : dp::kSpecklePattern);
    TestAreaPatterns(style, 16, {quarry, mine}, quarry, dp::kSpecklePattern);

    // Style priorities keep patterns on shared fills and settle ties between patterned surface types.
    TestAreaPatterns(style, 16, {beach, resort}, beach, dp::kStipplePattern);
    TestAreaPatterns(style, 16, {orchard, vineyard}, orchard, dp::kGridPattern);

    // A surviving intermittent rule marks the fill, while an invisible modifier supplies no pattern.
    TestAreaPatterns(style, 16, {quarry, lake, intermittent}, intermittent, dp::kStipplePattern);
    TestAreaPatterns(style, 16, {shingle, intermittent}, intermittent, dp::kStipplePattern);
    TestAreaPatterns(style, 14, {ditch, intermittent}, ditch, dp::kStipplePattern);
    TestAreaPatterns(style, 11, {ditch, intermittent}, intermittent, dp::kStipplePattern);
    TestAreaPatterns(style, 11, {park, intermittentBasin}, park, {});

    // Hatch selection is independent of fill modifiers and follows the selected hatch rule's source.
    TestAreaPatterns(style, 16, {lake, intermittent, reserve}, intermittent, dp::kStipplePattern, reserve,
                     dp::k45dHatching);
    TestAreaPatterns(style, 16, {park, reserve}, park, {}, reserve, dp::k45dHatching);
    TestAreaPatterns(style, 16, {bog, reserve}, 0, {}, bog, dp::kDashHatching);
  }
  reader.SetCurrentStyle(kDefaultMapStyle);
  classificator::Load();
}

UNIT_TEST(Stylist_AreaPatternsHaveDistinctPriorities)
{
  auto & reader = GetStyleReader();
  for (size_t s = 0; s < MapStyleCount; ++s)
  {
    auto const style = static_cast<MapStyle>(s);
    if (style == MapStyleMerged)
      continue;
    reader.SetCurrentStyle(style);
    classificator::Load();
    auto const & patterns = df::IsAreaPatternChecker::Instance();
    auto const & hatches = df::IsHatchingTerritoryChecker::Instance();
    std::map<int, std::string_view> priorities;
    classif().ForEachTree([&](ClassifObject const * object, uint32_t type)
    {
      if (hatches(type) || patterns.IsIntermittentWater(type))
        return;
      auto const pattern = patterns.GetPattern(type);
      for (int zoom = 0; zoom <= scales::GetUpperStyleScale(); ++zoom)
      {
        drule::KeysT keys;
        object->GetSuitable(zoom, feature::GeomType::Area, keys);
        for (auto const & key : keys)
        {
          if (key.m_type != drule::area)
            continue;
          auto const [entry, inserted] = priorities.emplace(key.m_priority, pattern);
          if (!inserted)
            TEST_EQUAL(entry->second, pattern, (style, zoom, classif().GetReadableObjectName(type), key.m_priority));
        }
      }
    });
  }
  reader.SetCurrentStyle(kDefaultMapStyle);
  classificator::Load();
}

UNIT_TEST(Stylist_IntermittentWaterAreaPriority)
{
  auto & reader = GetStyleReader();
  for (size_t s = 0; s < MapStyleCount; ++s)
  {
    auto const style = static_cast<MapStyle>(s);
    if (style == MapStyleMerged)
      continue;
    reader.SetCurrentStyle(style);
    classificator::Load();
    auto const & cl = classif();
    auto const lake = cl.GetTypeByPath({"natural", "water", "lake"});
    auto const intermittent = cl.GetTypeByPath({"natural", "water", "intermittent"});
    auto const ditch = cl.GetTypeByPath({"natural", "water", "ditch"});
    auto const river = cl.GetTypeByPath({"waterway", "river"});
    auto const basin = cl.GetTypeByPath({"landuse", "basin"});
    auto const intermittentBasin = cl.GetTypeByPath({"landuse", "basin", "intermittent"});
    auto const areaPriority = [style](std::initializer_list<uint32_t> types, uint8_t zoom)
    {
      auto feature = FeatureType::CreateFromMapObject(WaterArea(types));
      df::Stylist stylist(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
      CHECK(stylist.m_areaRule != nullptr, ());
      return stylist.m_areaRule->priority;
    };

    auto const permanentDepthPriority = areaPriority({lake}, 14);
    auto const intermittentDepthPriority = areaPriority({lake, intermittent}, 14);
    auto const ditchDepthPriority = areaPriority({ditch, intermittent}, 14);
    drule::KeysT lineKeys;
    feature::GetDrawRule({river}, 14, feature::GeomType::Line, lineKeys);
    auto const line =
        std::find_if(lineKeys.begin(), lineKeys.end(), [](drule::Key const & k) { return k.m_type == drule::line; });
    CHECK(line != lineKeys.end(), ());
    // BG-top scales style priority by 0.1; gaps above 8 stay distinct even with a 16-bit depth buffer.
    TEST_GREATER(intermittentDepthPriority - line->m_priority, 8, (style));
    TEST_GREATER(permanentDepthPriority - intermittentDepthPriority, 8, (style));
    TEST_GREATER(ditchDepthPriority - permanentDepthPriority, 8, (style));
    TEST_EQUAL(areaPriority({ditch, intermittent}, 11), intermittentDepthPriority, (style));
    TEST_LESS(areaPriority({intermittentBasin}, 14), areaPriority({basin}, 14), (style));
  }
  reader.SetCurrentStyle(kDefaultMapStyle);
  classificator::Load();
}

UNIT_CLASS_TEST(StyleSnapshotFixture, Stylist_ExplicitStyleIgnoresGlobalSwitches)
{
  using feature::GeomType;
  struct Sample
  {
    uint32_t m_type;
    GeomType m_geomType;
  };
  auto const & cl = classif(MapStyleDefaultLight);
  std::array const samples = {Sample{cl.GetTypeByPath({"highway", "motorway"}), GeomType::Line},
                              Sample{cl.GetTypeByPath({"highway", "path"}), GeomType::Line},
                              Sample{cl.GetTypeByPath({"amenity", "cafe"}), GeomType::Point},
                              Sample{cl.GetTypeByPath({"building"}), GeomType::Area},
                              Sample{cl.GetTypeByPath({"natural", "water", "lake"}), GeomType::Area},
                              Sample{cl.GetTypeByPath({"natural", "water", "intermittent"}), GeomType::Area},
                              Sample{cl.GetTypeByPath({"landuse", "quarry"}), GeomType::Area},
                              Sample{cl.GetTypeByPath({"leisure", "nature_reserve"}), GeomType::Area}};

  for (auto const style : kStyles)
  {
    for (auto const & sample : samples)
    {
      auto feature = FeatureType::CreateFromMapObject(SampleFeature(sample.m_type, sample.m_geomType));
      bool hasRules = false;
      for (uint8_t zoom = 1; zoom <= 19; ++zoom)
      {
        GetStyleReader().SetCurrentStyle(style);
        df::Stylist const expected(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
        hasRules = hasRules || expected.m_symbolRule || expected.m_areaRule || expected.m_hatchingRule ||
                   !expected.m_lineRules.empty();
        for (auto const current : kStyles)
        {
          GetStyleReader().SetCurrentStyle(current);
          df::Stylist const actual(*feature, zoom, StringUtf8Multilang::kEnglishCode, style);
          TestSameRules(actual, expected);
        }
      }
      TEST(hasRules, (style, sample.m_type));
    }
  }
}
