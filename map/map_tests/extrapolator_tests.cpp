#include "testing/testing.hpp"

#include "map/extrapolation/extrapolator.hpp"

#include "platform/location.hpp"

#include "base/math.hpp"

namespace
{
using namespace location;
using namespace extrapolation;

void TestGpsInfo(GpsInfo const & tested, GpsInfo const & expected)
{
  double constexpr kEpsilon = 1e-5;
  TEST_EQUAL(tested.m_source, expected.m_source, ());
  TEST(my::AlmostEqualAbs(tested.m_latitude, expected.m_latitude, kEpsilon), ());
  TEST(my::AlmostEqualAbs(tested.m_longitude, expected.m_longitude, kEpsilon), ());
  TEST(my::AlmostEqualAbs(tested.m_horizontalAccuracy, expected.m_horizontalAccuracy, kEpsilon), ());
  TEST(my::AlmostEqualAbs(tested.m_altitude, expected.m_altitude, kEpsilon), ());
  TEST(my::AlmostEqualAbs(tested.m_verticalAccuracy, expected.m_verticalAccuracy, kEpsilon), ());
  TEST(my::AlmostEqualAbs(tested.m_bearing, expected.m_bearing, kEpsilon), ());
  TEST(my::AlmostEqualAbs(tested.m_speed, expected.m_speed, kEpsilon), ());
}

GpsInfo GetGpsInfo(double timestampS, double lat, double lon)
{
  return GpsInfo{EAppleNative,
                 timestampS,
                 lat,
                 lon,
                 10.0 /* m_horizontalAccuracy */,
                 1.0 /* m_altitude */,
                 10.0 /* m_verticalAccuracy */,
                 0.0 /* m_bearing */,
                 10.0 /* m_speed */};
}

UNIT_TEST(LinearExtrapolation)
{
  GpsInfo const loc1 = {EAppleNative,
                        0.0 /* m_timestamp in seconds */,
                        1.0 /* m_latitude */,
                        1.0 /* m_longitude */,
                        10.0 /* m_horizontalAccuracy */,
                        1.0 /* m_altitude */,
                        10.0 /* m_verticalAccuracy */,
                        0.0 /* m_bearing */,
                        10.0 /* m_speed */};
  GpsInfo const loc2 = {EAppleNative,
                        1.0 /* m_timestamp in seconds */,
                        1.01 /* m_latitude */,
                        1.01 /* m_longitude */,
                        11.0 /* m_horizontalAccuracy */,
                        2.0 /* m_altitude */,
                        10.0 /* m_verticalAccuracy */,
                        1.0 /* m_bearing */,
                        12.0 /* m_speed */};
  // 0 ms after |point2|.
  TestGpsInfo(LinearExtrapolation(loc1, loc2, 0 /* timeAfterPoint2Ms */), loc2);

  // 100 ms after |point2|.
  {
    GpsInfo const expected = {EAppleNative,
                              1.1 /* m_timestamp */,
                              1.011 /* m_latitude */,
                              1.011 /* m_longitude */,
                              11.1 /* m_horizontalAccuracy */,
                              2.1 /* m_altitude */,
                              10.0 /* m_verticalAccuracy */,
                              1.0 /* m_bearing */,
                              12.2 /* m_speed */};
    TestGpsInfo(LinearExtrapolation(loc1, loc2, 100 /* timeAfterPoint2Ms */), expected);
  }

  // 200 ms after |point2|.
  {
    GpsInfo const expected = {EAppleNative,
                              1.2 /* m_timestamp */,
                              1.012 /* m_latitude */,
                              1.012 /* m_longitude */,
                              11.2 /* m_horizontalAccuracy */,
                              2.2 /* m_altitude */,
                              10.0 /* m_verticalAccuracy */,
                              1.0 /* m_bearing */,
                              12.4 /* m_speed */};
    TestGpsInfo(LinearExtrapolation(loc1, loc2, 200 /* timeAfterPoint2Ms */), expected);
  }

  // 1000 ms after |point2|.
  {
    GpsInfo const expected = {EAppleNative,
                              2.0 /* m_timestamp */,
                              1.02 /* m_latitude */,
                              1.02 /* m_longitude */,
                              12.0 /* m_horizontalAccuracy */,
                              3.0 /* m_altitude */,
                              10.0 /* m_verticalAccuracy */,
                              1.0 /* m_bearing */,
                              14.0 /* m_speed */};
    TestGpsInfo(LinearExtrapolation(loc1, loc2, 1000 /* timeAfterPoint2Ms */), expected);
  }
}

UNIT_TEST(AreCoordsGoodForExtrapolation)
{
  {
    GpsInfo loc1;
    GpsInfo loc2;
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Locations are not valid."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 10.0 /* lat */, 179.999999 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 10.0 /* lat */, -179.999999 /* lon */);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Crossing meridian 180."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 10.0 /* lat */, 179.999997 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 10.0 /* lat */, 179.999999 /* lon */);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Near meridian 180."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 10.0 /* lat */, 179.999997 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 10.0 /* lat */, 179.999998 /* lon */);
    TEST(AreCoordsGoodForExtrapolation(loc1, loc2), ("Near meridian 180 but ok."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 10.0 /* lat */, -179.999997 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 10.0 /* lat */, -179.999999  /* lon */);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Near meridian -180."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 89.9997 /* lat */, -10.0 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 89.9999 /* lat */, -10.0  /* lon */);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Close to North Pole."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 89.9997 /* lat */, -10.0 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 89.9998 /* lat */, -10.0  /* lon */);
    TEST(AreCoordsGoodForExtrapolation(loc1, loc2), ("Close to North Pole but ok."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, -89.9997 /* lat */, -10.0 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, -89.9999 /* lat */, -10.0  /* lon */);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Close to South Pole."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, -89.9997 /* lat */, -10.0 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, -89.9998 /* lat */, -10.0  /* lon */);
    TEST(AreCoordsGoodForExtrapolation(loc1, loc2), ("Close to South Pole but ok."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 10.0 /* lat */, -179.999997 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 10.0 /* lat */, -179.999998  /* lon */);
    TEST(AreCoordsGoodForExtrapolation(loc1, loc2), ("Near meridian -180 but ok."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 1.0 /* lat */, 10.0 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 2.0 /* lat */, 10.0 /* lon */);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Locations are too far."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 1.0 /* lat */, 1.0 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(1.0 /* timestamp */, 1.0 /* lat */, 1.00001 /* lon */);
    TEST(AreCoordsGoodForExtrapolation(loc1, loc2), ("Locations are close enough."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 1.0 /* lat */, 1.0 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(0.0 /* timestamp */, 1.0 /* lat */, 1.00001 /* lon */);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Time is the same."));
  }
  {
    GpsInfo const loc1 = GetGpsInfo(0.0 /* timestamp */, 1.0 /* lat */, 1.0 /* lon */);
    GpsInfo const loc2 = GetGpsInfo(3.0 /* timestamp */, 1.0 /* lat */, 1.00001 /* lon */);
    TEST(!AreCoordsGoodForExtrapolation(loc1, loc2), ("Too rare locations."));
  }
}
}  // namespace
