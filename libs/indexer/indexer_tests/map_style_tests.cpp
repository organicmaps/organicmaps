#include "testing/testing.hpp"

#include "indexer/map_style.hpp"

namespace map_style_tests
{
UNIT_TEST(GetMapStyleForFamily)
{
  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Default, false /* dark */), MapStyleDefaultLight, ());
  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Default, true), MapStyleDefaultDark, ());

  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Vehicle, false), MapStyleVehicleLight, ());
  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Vehicle, true), MapStyleVehicleDark, ());

  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Outdoors, false), MapStyleOutdoorsLight, ());
  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Outdoors, true), MapStyleOutdoorsDark, ());
}
}  // namespace map_style_tests
