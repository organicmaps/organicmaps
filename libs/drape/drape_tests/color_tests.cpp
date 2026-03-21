#include "testing/testing.hpp"

#include "drape/color.hpp"

UNIT_TEST(Color_HSL)
{
  {
    dp::Color const black = dp::Color::Black();
    dp::HSL hsl = dp::Color2HSL(black);
    TEST(hsl.IsDark(), ());
    TEST_EQUAL(dp::HSL2Color(hsl), black, ());

    TEST(hsl.AdjustLightness(false), ());
    TEST_EQUAL(dp::HSL2Color(hsl), dp::Color(204, 204, 204), ());
  }

  {
    dp::Color const white = dp::Color::White();
    dp::HSL hsl = dp::Color2HSL(white);
    TEST(!hsl.IsDark(), ());
    TEST_EQUAL(dp::HSL2Color(hsl), white, ());

    TEST(hsl.AdjustLightness(true), ());
    TEST_EQUAL(dp::HSL2Color(hsl), dp::Color(51, 51, 51), ());
  }
}
