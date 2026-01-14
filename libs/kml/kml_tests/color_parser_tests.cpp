#include "testing/testing.hpp"

#include "kml/color_parser.hpp"
#include "kml/serdes_geojson.hpp"

UNIT_TEST(ColorParser_Smoke)
{
  auto const magenta = kml::ParseGarminColor("Magenta");
  TEST(magenta, ());
  TEST_EQUAL(magenta, kml::ParseOSMColor("magenta"), ());
  TEST_EQUAL(magenta, kml::ParseHexColor("ff00ff"), ());
  TEST_EQUAL(magenta, kml::ParseHexColor("#ff00ff"), ());
  TEST_EQUAL(magenta, kml::ParseOSMColor("#f0f"), ());

  TEST(!kml::ParseGarminColor("xxyyzz"), ());
  TEST(!kml::ParseOSMColor("#xxyyzz"), ());

  // Current implementation gives assert with default 0 channel value. I didn't change this.
  // TEST(!kml::ParseHexColor("#xxyyzz"), ());
}

UNIT_TEST(ColorData_Test)
{
  kml::ColorData const defaultColor;
  TEST_EQUAL(kml::geojson::ToGeoJsonColor(defaultColor), "red", ());

  auto const greenHex = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::None, .m_rgba = 0x00FF00FF};
  TEST_EQUAL(kml::geojson::ToGeoJsonColor(greenHex), "#00FF00", ());

  auto const pinkPred = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::Pink, .m_rgba = 0};
  TEST_EQUAL(kml::geojson::ToGeoJsonColor(pinkPred), "pink", ());

  auto const deepOrange = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::DeepOrange, .m_rgba = 0xFF00AAFF};
  TEST_EQUAL(kml::geojson::ToGeoJsonColor(deepOrange), "tomato", ());
}
