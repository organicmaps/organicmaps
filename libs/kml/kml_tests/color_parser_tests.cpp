#include "testing/testing.hpp"

#include "kml/color_parser.hpp"

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
  kml::ColorData const default_color;
  TEST_EQUAL(kml::toCssColor(default_color), "red", ());

  auto const green_hex = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::None, .m_rgba = 0x00ff00ff};
  TEST_EQUAL(kml::toCssColor(green_hex), "#00ff00", ());

  auto const pink_pred = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::Pink, .m_rgba = 0};
  TEST_EQUAL(kml::toCssColor(pink_pred), "pink", ());

  auto const deep_orange = kml::ColorData{.m_predefinedColor = kml::PredefinedColor::DeepOrange, .m_rgba = 0xFF00AAFF};
  TEST_EQUAL(kml::toCssColor(deep_orange), "DarkOrange", ());
}
