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

UNIT_TEST(LinearExtrapolation)
{
  GpsInfo const point1 = {EAppleNative,
                          0.0 /* m_timestamp in seconds */,
                          1.0 /* m_latitude */,
                          1.0 /* m_longitude */,
                          10.0 /* m_horizontalAccuracy */,
                          1.0 /* m_altitude */,
                          10.0 /* m_verticalAccuracy */,
                          0.0 /* m_bearing */,
                          10.0 /* m_speed */};
  GpsInfo const point2 = {EAppleNative,
                          1.0 /* m_timestamp in seconds */,
                          1.01 /* m_latitude */,
                          1.01 /* m_longitude */,
                          11.0 /* m_horizontalAccuracy */,
                          2.0 /* m_altitude */,
                          10.0 /* m_verticalAccuracy */,
                          1.0 /* m_bearing */,
                          12.0 /* m_speed */};
  // 0 ms after |point2|.
  TestGpsInfo(LinearExtrapolation(point1, point2, 0 /* timeAfterPoint2Ms */), point2);

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
    TestGpsInfo(LinearExtrapolation(point1, point2, 100 /* timeAfterPoint2Ms */), expected);
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
    TestGpsInfo(LinearExtrapolation(point1, point2, 200 /* timeAfterPoint2Ms */), expected);
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
    TestGpsInfo(LinearExtrapolation(point1, point2, 1000 /* timeAfterPoint2Ms */), expected);
  }
}
}  // namespace
