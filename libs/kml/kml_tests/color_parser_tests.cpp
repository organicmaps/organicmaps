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

  TEST(!kml::ParseHexColor("#xxyyzz"), ());
}

UNIT_TEST(ColorData_JSON_Serialization_Test)
{
  kml::ColorData const defaultColor;
  TEST_EQUAL(kml::geojson::ToGeoJsonColor(defaultColor), "red", ());

  auto const greenHex = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::None, .m_rgba = 0x00FF00FF};
  TEST_EQUAL(kml::geojson::ToGeoJsonColor(greenHex), "#00FF00", ());

  // Convert each value of kml::PredefinedColor to JSON and back.
  // Make sure we get the same color after parsing.
  for (auto const predefColor : kml::kOrderedPredefinedColors)
  {
    kml::ColorData const colorData(predefColor);
    auto const jsonColor = kml::geojson::ToGeoJsonColor(colorData);
    TEST(!jsonColor.empty(), ());

    auto const parsedColor = kml::geojson::ParseGeoJsonColor(jsonColor);
    if (predefColor == kml::PredefinedColor::None)
      TEST_EQUAL(kml::ColorData{.m_predefinedColor = kml::PredefinedColor::Red}, *parsedColor,
                 ("JSON serializes None to Red"));
    else
      TEST_EQUAL(colorData, *parsedColor, ());
  }
}
