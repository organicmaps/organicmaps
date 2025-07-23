#include "testing/testing.hpp"

#include "platform/location.hpp"

UNIT_TEST(IsLatValid)
{
  TEST(location::IsLatValid(35.), ());
  TEST(location::IsLatValid(-35.), ());
  TEST(!location::IsLatValid(0.), ());
  TEST(!location::IsLatValid(100.), ());
  TEST(!location::IsLatValid(-99.), ());
}

UNIT_TEST(IsLonValid)
{
  TEST(location::IsLonValid(135.), ());
  TEST(location::IsLonValid(-35.), ());
  TEST(!location::IsLonValid(0.), ());
  TEST(!location::IsLonValid(200.), ());
  TEST(!location::IsLonValid(-199.), ());
}

UNIT_TEST(AngleToBearing)
{
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(0.), 90., ());
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(30.), 60., ());
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(100.), 350., ());
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(370.), 80., ());
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(-370.), 100., ());
}

UNIT_TEST(BearingToAngle)
{
  TEST_ALMOST_EQUAL_ULPS(location::BearingToAngle(0.), 90., ());
  TEST_ALMOST_EQUAL_ULPS(location::BearingToAngle(30.), 60., ());
  TEST_ALMOST_EQUAL_ULPS(location::BearingToAngle(100.), 350., ());
  TEST_ALMOST_EQUAL_ULPS(location::BearingToAngle(370.), 80., ());
  TEST_ALMOST_EQUAL_ULPS(location::AngleToBearing(-370.), 100., ());
}
