#include "testing/testing.hpp"

#include "map/gps_track_filter.hpp"

#include "platform/settings.hpp"

#include "base/scope_guard.hpp"

#include <vector>

namespace gps_track_filter_test
{
using Points = std::vector<location::GpsInfo>;

location::GpsInfo Make(double timestamp, double lat, double lon, double speed)
{
  location::GpsInfo info;
  info.m_timestamp = timestamp;
  info.m_speed = speed;
  info.m_latitude = lat;
  info.m_longitude = lon;
  info.m_horizontalAccuracy = 5;
  info.m_verticalAccuracy = 1.0;
  info.m_source = location::EAndroidNative;
  return info;
}

// Degrees of longitude per meter at the equator.
double constexpr kDegPerMeter = 1.0 / 111320.0;

// Straight line to the east, 15 m every second at 15 m/s: every point passes the filter.
Points MakeWalk(size_t count)
{
  Points points;
  for (size_t i = 0; i < count; ++i)
    points.push_back(Make(100.0 + i, 0.0, 15.0 * i * kDegPerMeter, 15.0));
  return points;
}

UNIT_TEST(GpsTrackFilter_RepeatedFixKeepsNextFix)
{
  Points const walk = MakeWalk(4);
  GpsTrackFilter filter;
  Points out;
  filter.Process({walk[0], walk[1]}, out);
  // The same fix again, e.g. the cached location replayed to a new location observer.
  filter.Process({walk[1]}, out);
  filter.Process({walk[2], walk[3]}, out);
  TEST_EQUAL(out.size(), 4, ());
  TEST_EQUAL(out[2].m_timestamp, walk[2].m_timestamp, ());
}

UNIT_TEST(GpsTrackFilter_SameTimeDistinctFixKeepsDirection)
{
  auto const fix = [](double timestamp, double east, double north)
  { return Make(timestamp, north * kDegPerMeter, east * kDegPerMeter, 15.0); };
  GpsTrackFilter filter;
  Points out;
  // The fix at 30 m has the same time as the one before it, but it is not a replay: it must stay in the
  // history, or the turn at 54 m looks like a jump from 15 m.
  filter.Process({fix(1000, 0, 0), fix(1001, 15, 0), fix(1001, 30, 0), fix(1011, 54, 8)}, out);
  TEST_EQUAL(out.size(), 3, ());
  TEST_EQUAL(out[2].m_timestamp, 1011, ());
}

UNIT_TEST(GpsTrackFilter_ClockRollbackAfterRejectedFix)
{
  auto const fix = [](double timestamp, double meters) { return Make(timestamp, 0.0, meters * kDegPerMeter, 15.0); };
  GpsTrackFilter filter;
  Points out;
  // The fix at 20 m is rejected as too close. Then the clock goes back: the fix at 1050 is older than
  // it and is dropped, but the fixes after it must be recorded.
  filter.Process({fix(1000, 0), fix(1001, 15), fix(1100, 20), fix(1050, 35), fix(1051, 50), fix(1052, 65)}, out);
  TEST(!filter.GetDeferredPoint(), ());
  TEST_EQUAL(out.size(), 4, ());
  TEST_EQUAL(out[2].m_timestamp, 1051, ());
  TEST_EQUAL(out[3].m_timestamp, 1052, ());
}

UNIT_TEST(GpsTrackFilter_ReportsLastDeferredFix)
{
  GpsTrackFilter filter;
  Points out;
  auto points = MakeWalk(5);
  points.front().m_verticalAccuracy = -1;
  filter.Process(points, out);
  TEST_EQUAL(out.size(), 5, ());
  TEST(!out.front().HasAltitude(), ());

  // Both fixes are too close to sample. The later fix is the endpoint.
  auto const last = Make(160.0, 0.0, (15.0 * 4 + 5.0) * kDegPerMeter, 0.0);
  auto missingAltitude = Make(161.0, 0.0, (15.0 * 4 + 8.0) * kDegPerMeter, 0.0);
  missingAltitude.m_verticalAccuracy = -1.0;
  out.clear();
  filter.Process({last, missingAltitude}, out);
  TEST(out.empty(), ());
  auto const * deferred = filter.GetDeferredPoint();
  TEST(deferred, ());
  TEST_EQUAL(deferred->m_timestamp, missingAltitude.m_timestamp, ());
  TEST_EQUAL(deferred->m_longitude, missingAltitude.m_longitude, ());
  TEST(!deferred->HasAltitude(), ());
}

UNIT_TEST(GpsTrackFilter_ZeroAccuracyBypassesSampling)
{
  std::string_view constexpr kMinAccuracyKey = "GpsTrackingMinAccuracy";
  double originalAccuracy = 0;
  bool const hadSetting = settings::Get(kMinAccuracyKey, originalAccuracy);
  SCOPE_GUARD(restoreAccuracy, [&]
  {
    if (hadSetting)
      GpsTrackFilter::StoreMinHorizontalAccuracy(originalAccuracy);
    else
      settings::Delete(kMinAccuracyKey);
  });
  GpsTrackFilter::StoreMinHorizontalAccuracy(0);
  GpsTrackFilter filter;
  auto points = MakeWalk(2);
  points.front().m_speed = -1;
  points.front().m_verticalAccuracy = -1;
  Points out{Make(99, 0, 0, 0)};
  filter.Process(points, out);
  TEST(!filter.GetDeferredPoint(), ());
  TEST_EQUAL(out.size(), 3, ());
  TEST_EQUAL(out[0].m_timestamp, 99, ());
  TEST_EQUAL(out[1].m_timestamp, points.front().m_timestamp, ());
  TEST(!out[1].HasAltitude(), ());
  TEST_EQUAL(out[2].m_timestamp, points.back().m_timestamp, ());
}

}  // namespace gps_track_filter_test
