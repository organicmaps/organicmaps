#include "testing/testing.hpp"

#include "drape_frontend/stylist.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_object.hpp"
#include "indexer/map_style_reader.hpp"

#include "drape/hatching_decl.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>

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
  explicit WaterArea(std::initializer_list<uint32_t> types)
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
  TEST(std::equal(actual.m_lineRules.begin(), actual.m_lineRules.end(), expected.m_lineRules.begin(),
                  expected.m_lineRules.end()),
       ());
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

  // Constructing the singleton resolves every checker type via GetTypeByPath, which CHECKs the type
  // exists - so an invalid path (e.g. the 3-level natural=beach=sand mistaken for 2-level) fails here.
  auto const & checker = df::IsAreaPatternChecker::Instance();

  // Stipple: sandy / desert surfaces and intermittent water. natural=sand is a beach subtype, matched via the parent.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach", "sand"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "desert"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "water", "intermittent"})), dp::kStipplePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "basin", "intermittent"})), dp::kStipplePattern, ());

  // Speckle: rocky surfaces.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "scree"})), dp::kSpecklePattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "bare_rock"})), dp::kSpecklePattern, ());

  // Grid: planted landuse.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "orchard"})), dp::kGridPattern, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "vineyard"})), dp::kGridPattern, ());

  // Permanent water and unrelated area types get no pattern.
  TEST(checker.GetPattern(cl.GetTypeByPath({"natural", "water"})).empty(), ());
  TEST(checker.GetPattern(cl.GetTypeByPath({"landuse", "basin"})).empty(), ());
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
                              Sample{cl.GetTypeByPath({"natural", "water", "lake"}), GeomType::Area}};

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
        hasRules = hasRules || expected.m_symbolRule || expected.m_areaRule || !expected.m_lineRules.empty();
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
