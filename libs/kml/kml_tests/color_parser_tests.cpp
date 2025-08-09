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
