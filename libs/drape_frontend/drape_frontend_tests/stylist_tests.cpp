#include "testing/testing.hpp"

#include "drape_frontend/area_pattern.hpp"
#include "drape_frontend/stylist.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/feature.hpp"
#include "indexer/map_object.hpp"
#include "indexer/map_style_reader.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace stylist_tests
{
using df::AreaPattern;
using Path = std::vector<std::string>;

UNIT_TEST(Stylist_IsHatching)
{
  classificator::Load();
  auto const & cl = classif();

  auto const & checker = df::IsHatchingTerritoryChecker::Instance();

  TEST_EQUAL(checker.GetHatch(cl.GetTypeByPath({"boundary", "protected_area", "1"})), AreaPattern::Hatch45d, ());
  TEST_EQUAL(checker.GetHatch(cl.GetTypeByPath({"boundary", "protected_area", "2"})), AreaPattern::None, ());
  TEST_EQUAL(checker.GetHatch(cl.GetTypeByPath({"boundary", "protected_area"})), AreaPattern::None, ());

  TEST_EQUAL(checker.GetHatch(cl.GetTypeByPath({"boundary", "national_park"})), AreaPattern::Hatch45d, ());

  TEST_EQUAL(checker.GetHatch(cl.GetTypeByPath({"landuse", "military", "danger_area"})), AreaPattern::Hatch45d, ());

  TEST_EQUAL(checker.GetHatch(cl.GetTypeByPath({"amenity", "prison"})), AreaPattern::Hatch45d, ());

  TEST_EQUAL(checker.GetHatch(cl.GetTypeByPath({"natural", "wetland", "bog"})), AreaPattern::HatchDash, ());
}

UNIT_TEST(Stylist_IsAreaPattern)
{
  classificator::Load();
  auto const & cl = classif();

  // Constructing the singleton resolves every checker type via GetTypeByPath, which CHECKs the type
  // exists - so an invalid path (e.g. the 3-level natural=beach=sand mistaken for 2-level) fails here.
  auto const & checker = df::IsAreaPatternChecker::Instance();

  // Stipple: sandy / desert surfaces. Sand beaches (natural=beach + surface=sand) match via their parent.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach"})), AreaPattern::Stipple, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach", "sand"})), AreaPattern::Stipple, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "desert"})), AreaPattern::Stipple, ());

  // Speckle: rocky surfaces.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "scree"})), AreaPattern::Speckle, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "bare_rock"})), AreaPattern::Speckle, ());

  // Grid: planted landuse.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "orchard"})), AreaPattern::Grid, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "vineyard"})), AreaPattern::Grid, ());

  // Intermittent water is a modifier: its stipple is drawn on the fill of any type.
  for (uint32_t const type :
       {cl.GetTypeByPath({"natural", "water", "intermittent"}), cl.GetTypeByPath({"landuse", "basin", "intermittent"})})
  {
    TEST_EQUAL(checker.GetModifierPattern(type), AreaPattern::Stipple, (type));
    TEST_EQUAL(checker.GetPattern(type), AreaPattern::None, (type));
  }
  TEST_EQUAL(checker.GetModifierPattern(cl.GetTypeByPath({"natural", "beach"})), AreaPattern::None, ());

  // Permanent water and unrelated area types get no pattern.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "water"})), AreaPattern::None, ());
  TEST_EQUAL(checker.GetModifierPattern(cl.GetTypeByPath({"natural", "water"})), AreaPattern::None, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "basin"})), AreaPattern::None, ());
}

// An area feature without an mwm, with its types in the given order.
class AreaObject : public osm::MapObject
{
public:
  explicit AreaObject(std::vector<uint32_t> const & types)
  {
    m_geomType = feature::GeomType::Area;
    // Runtime selectors of some rules read the limit rect.
    m_triangles = {{0, 0}, {1, 0}, {0, 1}};
    m_types = feature::TypesHolder(feature::GeomType::Area);
    for (uint32_t t : types)
      m_types.Add(t);
  }
};

drule::AreaRule const * GetAreaRule(Path const & path, int zoom)
{
  drule::KeysT keys;
  classif().GetObject(classif().GetTypeByPath(path))->GetSuitable(zoom, feature::GeomType::Area, keys);
  for (auto const & k : keys)
    if (k.m_type == drule::area)
      return drule::GetCurrentRules().Find(k)->GetArea();
  return nullptr;
}

// Checks that Stylist retains the area rules of the expected fill and hatching types with their patterns, in both
// orders of the feature types.
void TestAreaRules(std::vector<Path> const & paths, Path const & fill, AreaPattern fillPattern,
                   Path const & hatching = {}, AreaPattern hatchingPattern = AreaPattern::None, int zoom = 16)
{
  auto const * const fillRule = fill.empty() ? nullptr : GetAreaRule(fill, zoom);
  auto const * const hatchingRule = hatching.empty() ? nullptr : GetAreaRule(hatching, zoom);
  TEST(fill.empty() || fillRule, (fill, zoom));
  TEST(hatching.empty() || hatchingRule, (hatching, zoom));

  std::vector<uint32_t> types;
  for (auto const & path : paths)
    types.push_back(classif().GetTypeByPath(path));

  for (int i = 0; i < 2; ++i)
  {
    auto const ft = FeatureType::CreateFromMapObject(AreaObject(types));
    feature::TypesHolder const holder(*ft);
    df::Stylist const s(*ft, zoom, 0 /* deviceLang */, false /* forceOutdoorStyle */);
    TEST_EQUAL(s.m_areaRule, fillRule, (holder));
    TEST_EQUAL(s.m_areaPattern, fillPattern, (holder));
    TEST_EQUAL(s.m_hatchingRule, hatchingRule, (holder));
    TEST_EQUAL(s.m_hatchingPattern, hatchingPattern, (holder));
    std::reverse(types.begin(), types.end());
  }
}

// The expected rules follow the area priorities and zooms of the default style.
UNIT_TEST(Stylist_AreaPatternsFollowRetainedRules)
{
  GetStyleReader().SetCurrentStyle(MapStyleDefaultLight);
  classificator::Load();

  // A fill and a hatch are both retained, each with the pattern of its own type.
  TestAreaRules({{"natural", "water"}, {"leisure", "nature_reserve"}}, {"natural", "water"}, AreaPattern::None,
                {"leisure", "nature_reserve"}, AreaPattern::Hatch45d);

  // Of two hatches the bog outranks the nature reserve and keeps its dash hatch, also when the reserve comes first,
  // as in maps.
  TestAreaRules({{"leisure", "nature_reserve"}, {"natural", "wetland", "bog"}}, {}, AreaPattern::None,
                {"natural", "wetland", "bog"}, AreaPattern::HatchDash);

  // A surface pattern is drawn only on the fill of its own type.
  TestAreaRules({{"natural", "beach"}, {"natural", "water"}}, {"natural", "water"}, AreaPattern::None);
  TestAreaRules({{"natural", "bare_rock"}, {"natural", "water"}}, {"natural", "water"}, AreaPattern::None);
  TestAreaRules({{"natural", "scree"}, {"natural", "scrub"}}, {"natural", "scrub"}, AreaPattern::None);
  TestAreaRules({{"natural", "beach"}, {"natural", "scree"}}, {"natural", "scree"}, AreaPattern::Speckle);
  // A beach mapped as a beach resort, which has the same fill, keeps its dots.
  TestAreaRules({{"natural", "beach"}, {"leisure", "beach_resort"}}, {"natural", "beach"}, AreaPattern::Stipple);

  // A modifier pattern is drawn on any retained fill: an intermittent ditch keeps its own fill with dots, and
  // intermittent water on bare rock gets dots instead of rocks.
  TestAreaRules({{"natural", "water", "ditch"}, {"natural", "water", "intermittent"}}, {"natural", "water", "ditch"},
                AreaPattern::Stipple);
  TestAreaRules({{"natural", "bare_rock"}, {"natural", "water", "intermittent"}}, {"natural", "water", "intermittent"},
                AreaPattern::Stipple);
  // A modifier goes on the fill only: a nature reserve keeps its hatch over dotted intermittent water.
  TestAreaRules({{"leisure", "nature_reserve"}, {"natural", "water", "intermittent"}},
                {"natural", "water", "intermittent"}, AreaPattern::Stipple, {"leisure", "nature_reserve"},
                AreaPattern::Hatch45d);
  // A modifier needs an area rule of its own type: intermittent basins are drawn from z12, so at z11 scree keeps its
  // rocks.
  TestAreaRules({{"natural", "scree"}, {"landuse", "basin", "intermittent"}}, {"natural", "scree"},
                AreaPattern::Speckle, {}, AreaPattern::None, 11 /* zoom */);
}

// MakeUnique retains the area rule with the highest priority and picks an arbitrary one of equal priorities, so area
// types with different patterns must not share a priority.
UNIT_TEST(Stylist_AreaPatternsHaveOwnPriorities)
{
  for (auto const style : {MapStyleDefaultLight, MapStyleOutdoorsLight, MapStyleVehicleLight})
  {
    GetStyleReader().SetCurrentStyle(style);
    classificator::Load();
    auto const & cl = classif();
    auto const & hatchingChecker = df::IsHatchingTerritoryChecker::Instance();
    auto const & patternChecker = df::IsAreaPatternChecker::Instance();
    // Hatches and fills are retained separately.
    std::map<std::pair<bool, int>, std::pair<AreaPattern, uint32_t>> firstTypes;
    cl.ForEachTree([&](ClassifObject const * obj, uint32_t type)
    {
      auto const hatch = hatchingChecker.GetHatch(type);
      auto const pattern = hatch != AreaPattern::None ? hatch : patternChecker.GetPattern(type);
      for (auto const & k : obj->GetDrawRules())
      {
        if (k.m_type != drule::area)
          continue;
        auto const & [firstPattern, firstType] =
            firstTypes.try_emplace({hatch != AreaPattern::None, k.m_priority}, pattern, type).first->second;
        TEST_EQUAL(firstPattern, pattern,
                   (style, k.m_priority, cl.GetReadableObjectName(firstType), cl.GetReadableObjectName(type)));
      }
    });
  }
}
}  // namespace stylist_tests
