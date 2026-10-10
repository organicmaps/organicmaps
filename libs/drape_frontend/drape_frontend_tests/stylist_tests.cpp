#include "testing/testing.hpp"

#include "drape_frontend/stylist.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_object.hpp"
#include "indexer/map_style_reader.hpp"

#include "drape/hatching_decl.hpp"

#include "platform/distance.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include "base/scope_guard.hpp"

#include <algorithm>
#include <initializer_list>

namespace
{
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

class NamedLine : public osm::MapObject
{
public:
  NamedLine(uint32_t type, std::string_view name)
  {
    m_geomType = feature::GeomType::Line;
    m_points = {{0.0, 0.0}, {0.01, 0.01}};
    m_types.Add(type);
    m_name.Add(StringUtf8Multilang::kDefaultCode, name);
    m_featureID = FeatureID(MwmSet::MwmId(std::make_shared<MwmInfo>()), 0);
  }
};
}  // namespace

UNIT_TEST(Stylist_ContourAltitudeUnits)
{
  classificator::Load();
  auto const & cl = classif();

  std::string oldUnits;
  bool const hadUnits = settings::Get(settings::kAltitudeUnits, oldUnits);
  SCOPE_GUARD(restoreUnits, [&]
  {
    if (hadUnits)
      settings::Set(settings::kAltitudeUnits, oldUnits);
    else
      settings::Delete(settings::kAltitudeUnits);
  });

  auto const caption = [](FeatureType & f)
  {
    df::CaptionDescription description;
    description.Init(f, StringUtf8Multilang::kEnglishCode, 18, feature::GeomType::Line, false);
    TEST(description.GetAuxText().empty(), ());
    return description.GetMainText();
  };

  for (auto const subtype : {"step_10", "step_50", "step_100", "step_500", "step_1000", "zero"})
  {
    auto const type = cl.GetTypeByPath({"isoline", subtype});
    for (auto const name : {"100", "0", "-50", ""})
    {
      auto f = FeatureType::CreateFromMapObject(NamedLine(type, name));
      settings::Set(settings::kAltitudeUnits, measurement_utils::Units::Metric);
      auto const metric = std::string(name).empty() ? "" : std::string(name) + platform::kNarrowNonBreakingSpace + "m";
      TEST_EQUAL(caption(*f), metric, (subtype, name));

      settings::Set(settings::kAltitudeUnits, measurement_utils::Units::Imperial);
      std::string const feet = std::string(name) == "100" ? "328" : std::string(name) == "-50" ? "-164" : name;
      auto const imperial = feet.empty() ? "" : feet + platform::kNarrowNonBreakingSpace + "ft";
      TEST_EQUAL(caption(*f), imperial, (subtype, name));
      // Switching units reuses the same map data without modifying its stored height.
      TEST_EQUAL(f->GetName(StringUtf8Multilang::kDefaultCode), name, ());
      settings::Set(settings::kAltitudeUnits, measurement_utils::Units::Metric);
      TEST_EQUAL(caption(*f), metric, (subtype, name));
    }
  }

  // A numeric road name must never be interpreted as an elevation.
  auto road = FeatureType::CreateFromMapObject(NamedLine(cl.GetTypeByPath({"highway", "residential"}), "100"));
  settings::Set(settings::kAltitudeUnits, measurement_utils::Units::Imperial);
  TEST_EQUAL(caption(*road), "100", ());
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
    auto const areaPriority = [](std::initializer_list<uint32_t> types, uint8_t zoom)
    {
      auto feature = FeatureType::CreateFromMapObject(WaterArea(types));
      df::Stylist stylist(*feature, zoom, StringUtf8Multilang::kEnglishCode, false);
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
