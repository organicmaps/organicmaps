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

UNIT_TEST(ColorData_JSON_Serialization_Test)
{
  kml::ColorData const defaultColor;
  TEST_EQUAL(kml::geojson::ToGeoJsonColor(defaultColor), "red", ());

  auto const greenHex = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::None, .m_rgba = 0x00FF00FF};
  TEST_EQUAL(kml::geojson::ToGeoJsonColor(greenHex), "#00FF00", ());

  // Convert each value of kml::PredefinedColor to JSON and back.
  // Make sure we get the same color after parsing.
  for (auto predefColor : kml::kOrderedPredefinedColors)
  {
    if (predefColor == kml::PredefinedColor::None)
      continue;
    kml::ColorData colorData(predefColor);
    std::string jsonColor = kml::geojson::ToGeoJsonColor(colorData);
    TEST(!jsonColor.empty(), ());

    kml::ColorData parsedColor;
    kml::geojson::ParseGeoJsonColor(jsonColor, parsedColor);
    TEST_EQUAL(colorData, parsedColor, ());
  }
}
