#include "testing/testing.hpp"

#include "drape/color.hpp"

using namespace dp;

UNIT_TEST(Color_HSL)
{
  {
    Color const black = Color::Black();
    HSL hsl = Color2HSL(black);
    TEST(hsl.IsDark(), ());
    TEST_EQUAL(HSL2Color(hsl), black, ());

    TEST(hsl.AdjustLightness(false), ());
    TEST_EQUAL(HSL2Color(hsl), Color(204, 204, 204), ());
  }

  {
    Color const white = Color::White();
    HSL hsl = Color2HSL(white);
    TEST(!hsl.IsDark(), ());
    TEST_EQUAL(HSL2Color(hsl), white, ());

    TEST(hsl.AdjustLightness(true), ());
    TEST_EQUAL(HSL2Color(hsl), Color(51, 51, 51), ());
  }
}
