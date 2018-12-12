#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "indexer/classificator.hpp"

#include "platform/local_country_file.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/mercator.hpp"

#include "std/algorithm.hpp"
#include "std/vector.hpp"

UNIT_TEST(Framework_ForEachFeatureAtPoint_And_Others)
{
  Framework frm(FrameworkParams(false /* m_enableLocalAds */, false /* m_enableDiffs */));
  frm.DeregisterAllMaps();
  frm.RegisterMap(platform::LocalCountryFile::MakeForTesting("minsk-pass"));

  vector<char const *> types =
  {
    "highway|footway|",
    "hwtag|yesfoot|",
    "hwtag|yesbicycle|",
    "highway|service|parking_aisle|",
    "amenity|parking|",
    "barrier|lift_gate|"
  };
  frm.ForEachFeatureAtPoint([&](FeatureType & ft)
  {
    ft.ForEachType([&types](uint32_t type)
    {
      string const strType = classif().GetFullObjectName(type);
      auto found = find(types.begin(), types.end(), strType);
      TEST(found != types.end(), ());
      types.erase(found);
    });
  }, MercatorBounds::FromLatLon(53.882663, 27.537788));
  TEST_EQUAL(0, types.size(), ());

  ftypes::IsBuildingChecker const & isBuilding = ftypes::IsBuildingChecker::Instance();
  {
    // Restaurant in the building.
    auto const feature = frm.GetFeatureAtPoint(MercatorBounds::FromLatLon(53.89395, 27.567365));
    string name;
    TEST(feature->GetName(StringUtf8Multilang::kDefaultCode, name), ());
    TEST_EQUAL("Родны Кут", name, ());
    TEST(!isBuilding(*feature), ());
  }

  {
    // Same building as above, very close to the restaurant.
    auto const feature = frm.GetFeatureAtPoint(MercatorBounds::FromLatLon(53.893603, 27.567032));
    TEST(feature, ());
    TEST(isBuilding(*feature), ());
  }
}
