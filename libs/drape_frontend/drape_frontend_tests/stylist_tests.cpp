#include "testing/testing.hpp"

#include "drape_frontend/stylist.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_object.hpp"
#include "indexer/map_style_reader.hpp"

#include "drape/hatching_decl.hpp"

#include "platform/platform.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <string_view>

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
  SampleFeature(uint32_t type, feature::GeomType geomType) : SampleFeature({type}, geomType, true) {}

  SampleFeature(std::initializer_list<uint32_t> types, feature::GeomType geomType, bool named)
  {
    m_geomType = geomType;
    m_points = {{0.0, 0.0}, {0.01, 0.01}};
    m_triangles = {{0.0, 0.0}, {0.01, 0.0}, {0.0, 0.01}};
    for (auto type : types)
      m_types.Add(type);
    if (named)
      m_name.Add("en", "Sample");
    m_houseNumber = "12";
  }
};

class GuardedOverlayFixture
{
public:
  GuardedOverlayFixture() : m_originalStyle(GetStyleReader().GetCurrentStyle())
  {
    GetStyleReader().SetCurrentStyle(MapStyleDefaultLight);
    LoadTypes();
  }

  ~GuardedOverlayFixture()
  {
    GetStyleReader().SetCurrentStyle(m_originalStyle);
    classificator::Load();
  }

protected:
  void LoadTypes(bool shiftAddressType = false)
  {
    auto & platform = GetPlatform();
    std::string classificatorText;
    std::string typesText;
    platform.GetReader("classificator.txt")->ReadAsString(classificatorText);
    platform.GetReader("types.txt")->ReadAsString(typesText);
    if (shiftAddressType)
    {
      // Siblings are sorted by name, so inserting an earlier one changes the encoded address type.
      std::string_view constexpr kBuilding = "\n    building  +\n";
      auto const position = classificatorText.find(kBuilding);
      TEST_NOT_EQUAL(position, std::string::npos, ());
      classificatorText.insert(position + kBuilding.size(), "        0  -\n");
    }
    classificator::LoadTypes(classificatorText, typesText);
  }

  void LoadRules(drule::DrulesFormat const & format)
  {
    auto & cl = classif(MapStyleDefaultLight);
    drule::GetRules(MapStyleDefaultLight).LoadFromFormat(format, 0, cl);
    cl.SetLoaded(true);
  }

private:
  MapStyle const m_originalStyle;
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

UNIT_CLASS_TEST(GuardedOverlayFixture, Stylist_FallsBackWhenMainOverlayIsRejected)
{
  drule::Element primary;
  primary.scale = 15;
  primary.applyIf = {"name"};
  primary.symbol.emplace().name = "primary";
  primary.symbol->priority = 200;

  auto rejected = primary;
  rejected.symbol->name = "rejected";
  rejected.symbol->priority = 175;

  drule::Element fallback;
  fallback.scale = 15;
  fallback.symbol.emplace().name = "fallback";
  fallback.symbol->priority = 150;

  auto lower = fallback;
  lower.symbol->name = "lower";
  lower.symbol->priority = 100;
  auto earlier = lower;
  earlier.scale = 14;

  drule::DrulesFormat format;
  format.variants = {"light"};
  format.colors = {{0}};
  format.ruleCounts[drule::kRuleSymbols] = 5;
  format.types = {{"amenity-cafe", {primary}},
                  {"amenity-restaurant", {rejected}},
                  {"shop-supermarket", {earlier, lower}},
                  {"tourism-hotel", {fallback}}};
  LoadRules(format);

  auto const & cl = classif(MapStyleDefaultLight);
  auto const cafe = cl.GetTypeByPath({"amenity", "cafe"});
  auto const restaurant = cl.GetTypeByPath({"amenity", "restaurant"});
  auto const supermarket = cl.GetTypeByPath({"shop", "supermarket"});
  auto const hotel = cl.GetTypeByPath({"tourism", "hotel"});
  auto const types = {cafe, restaurant, supermarket, hotel};
  auto named = FeatureType::CreateFromMapObject(SampleFeature(types, feature::GeomType::Point, true));
  auto unnamed = FeatureType::CreateFromMapObject(SampleFeature(types, feature::GeomType::Point, false));
  auto constexpr kLang = StringUtf8Multilang::kEnglishCode;

  df::Stylist const namedAt15(*named, 15, kLang, MapStyleDefaultLight);
  TEST(namedAt15.m_symbolRule, ());
  TEST_EQUAL(namedAt15.m_symbolRule->name, "primary", ());
  df::Stylist const unnamedAt15(*unnamed, 15, kLang, MapStyleDefaultLight);
  TEST(unnamedAt15.m_symbolRule, ());
  TEST_EQUAL(unnamedAt15.m_symbolRule->name, "fallback", ());
  auto reordered = FeatureType::CreateFromMapObject(
      SampleFeature({cafe, hotel, supermarket, restaurant}, feature::GeomType::Point, false));
  df::Stylist const reorderedAt15(*reordered, 15, kLang, MapStyleDefaultLight);
  TEST(reorderedAt15.m_symbolRule, ());
  TEST_EQUAL(reorderedAt15.m_symbolRule->name, "fallback", ());
  df::Stylist const unnamedAt14(*unnamed, 14, kLang, MapStyleDefaultLight);
  TEST(!unnamedAt14.m_symbolRule, ());
}

UNIT_CLASS_TEST(GuardedOverlayFixture, Stylist_FiltersFallbackHouseNumberRules)
{
  drule::Element main;
  main.scale = 18;
  main.caption.emplace().primary.emplace().height = 14;
  main.caption->secondary.emplace().height = 10;
  auto unnamedRule = main;
  unnamedRule.applyIf = {"!name"};
  unnamedRule.caption->primary->height = 10;
  auto namedRule = unnamedRule;
  namedRule.applyIf = {"name"};
  namedRule.caption->primary->height = 12;

  drule::DrulesFormat format;
  format.variants = {"light"};
  format.colors = {{0}};
  format.ruleCounts[drule::kRuleCaptions] = 3;
  format.types = {{"amenity-cafe", {main}}, {"building-address", {unnamedRule, namedRule}}};
  LoadRules(format);

  auto const & cl = classif(MapStyleDefaultLight);
  auto const originalAddressType = cl.GetTypeByPath({"building", "address"});
  auto const cafe = cl.GetTypeByPath({"amenity", "cafe"});
  auto const checkHeight = [cafe](bool named, int height)
  {
    auto feature = FeatureType::CreateFromMapObject(SampleFeature({cafe}, feature::GeomType::Point, named));
    df::Stylist stylist(*feature, 18, StringUtf8Multilang::kEnglishCode, MapStyleDefaultLight);
    TEST(stylist.m_houseNumberRule, (named));
    TEST_EQUAL(stylist.m_houseNumberRule->primary->height, height, (named));
  };
  checkHeight(true, 12);
  checkHeight(false, 10);

  LoadTypes(true /* shiftAddressType */);
  LoadRules(format);
  TEST_NOT_EQUAL(originalAddressType, cl.GetTypeByPath({"building", "address"}), ());
  checkHeight(true, 12);
  checkHeight(false, 10);
}
}  // namespace
