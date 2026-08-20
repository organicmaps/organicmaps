#include "indexer/map_style.hpp"

#include "testing/testing.hpp"

#include <cstddef>

namespace map_style_tests
{
UNIT_TEST(MapStyleSettingsRoundTrip)
{
  for (size_t i = 0; i < MapStyleCount; ++i)
  {
    auto const style = static_cast<MapStyle>(i);
    if (style == MapStyleMerged)
      continue;
    TEST_EQUAL(MapStyleFromSettings(MapStyleToString(style)), style, ());
  }

  TEST_EQUAL(MapStyleFromSettings("MapStyleMerged"), kDefaultMapStyle, ());
  TEST_EQUAL(MapStyleFromSettings("unknown"), kDefaultMapStyle, ());
}

UNIT_TEST(MapStyleModeVariants)
{
  TEST_EQUAL(GetMapStyleForMode(MapStyleMode::Default, false), MapStyleDefaultLight, ());
  TEST_EQUAL(GetMapStyleForMode(MapStyleMode::Default, true), MapStyleDefaultDark, ());
  TEST_EQUAL(GetMapStyleForMode(MapStyleMode::Outdoors, false), MapStyleOutdoorsLight, ());
  TEST_EQUAL(GetMapStyleForMode(MapStyleMode::Outdoors, true), MapStyleOutdoorsDark, ());
  TEST_EQUAL(GetMapStyleForMode(MapStyleMode::Cycling, false), MapStyleCyclingLight, ());
  TEST_EQUAL(GetMapStyleForMode(MapStyleMode::Cycling, true), MapStyleCyclingDark, ());

  TEST(GetMapStyleMode(MapStyleDefaultLight) == MapStyleMode::Default, ());
  TEST(GetMapStyleMode(MapStyleVehicleDark) == MapStyleMode::Default, ());
  TEST(GetMapStyleMode(MapStyleMerged) == MapStyleMode::Default, ());
  TEST(GetMapStyleMode(MapStyleOutdoorsLight) == MapStyleMode::Outdoors, ());
  TEST(GetMapStyleMode(MapStyleOutdoorsDark) == MapStyleMode::Outdoors, ());
  TEST(GetMapStyleMode(MapStyleCyclingLight) == MapStyleMode::Cycling, ());
  TEST(GetMapStyleMode(MapStyleCyclingDark) == MapStyleMode::Cycling, ());
}
}  // namespace map_style_tests
