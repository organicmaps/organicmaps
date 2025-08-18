#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "indexer/classificator.hpp"

#include "platform/local_country_file.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

UNIT_TEST(Framework_ForEachFeatureAtPoint_And_Others)
{
  using namespace std;

  Framework frm(FrameworkParams(false /* m_enableDiffs */));
  frm.DeregisterAllMaps();
  frm.RegisterMap(platform::LocalCountryFile::MakeForTesting("minsk-pass"));

  // May vary according to the new minsk-pass data.
  vector<char const *> types = {
      "highway|footway|",   "hwtag|yesbicycle|",    "psurface|paved_good|",

      "highway|service|",   "psurface|paved_good|",

      "amenity|parking|",

      "barrier|lift_gate|",
  };
  frm.ForEachFeatureAtPoint([&](FeatureType & ft)
  {
    ft.ForEachType([&types](uint32_t type)
    {
      string const strType = classif().GetFullObjectName(type);
      auto found = find(types.begin(), types.end(), strType);
      TEST(found != types.end(), (strType));
      types.erase(found);
    });
  }, mercator::FromLatLon(53.8826576, 27.5378385));
  TEST_EQUAL(0, types.size(), (types));

  ftypes::IsBuildingChecker const & isBuilding = ftypes::IsBuildingChecker::Instance();
  {
    // Restaurant in the building.
    auto const id = frm.GetFeatureAtPoint(mercator::FromLatLon(53.89395, 27.567365));
    TEST(id.IsValid(), ());
    frm.GetDataSource().ReadFeature([&](FeatureType & ft)
    {
      TEST_EQUAL("Родны Кут", ft.GetName(StringUtf8Multilang::kDefaultCode), ());
      TEST(!isBuilding(ft), ());
    }, id);
  }

  {
    // Same building as above, very close to the restaurant.
    auto const id = frm.GetFeatureAtPoint(mercator::FromLatLon(53.893603, 27.567032));
    TEST(id.IsValid(), ());
    frm.GetDataSource().ReadFeature([&](FeatureType & ft) { TEST(isBuilding(ft), ()); }, id);
  }
}
