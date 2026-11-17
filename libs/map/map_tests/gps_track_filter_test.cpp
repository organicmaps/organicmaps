#include "testing/testing.hpp"

#include "map/gps_track_filter.hpp"

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
  filter.Finalize(out);
  TEST_EQUAL(out.size(), 4, ());
  TEST_EQUAL(out[2].m_timestamp, 1051, ());
  TEST_EQUAL(out[3].m_timestamp, 1052, ());
}

UNIT_TEST(GpsTrackFilter_FinalizeSkipsRepeatedFix)
{
  GpsTrackFilter filter;
  Points out;
  filter.Process(MakeWalk(5), out);
  TEST_EQUAL(out.size(), 5, ());

  // The last position reported again after a pause is neither accepted nor force-appended:
  // it would only add a zero-length segment to the saved track.
  out.clear();
  filter.Process({Make(160.0, 0.0, 15.0 * 4 * kDegPerMeter, 0.0)}, out);
  TEST(out.empty(), ());
  filter.Finalize(out);
  TEST(out.empty(), ());
}

UNIT_TEST(GpsTrackFilter_FinalizeAppendsLastFix)
{
  GpsTrackFilter filter;
  Points out;
  filter.Process(MakeWalk(5), out);
  TEST_EQUAL(out.size(), 5, ());

  // A fix too close to be accepted still ends the track where the user stopped.
  auto const last = Make(160.0, 0.0, (15.0 * 4 + 5.0) * kDegPerMeter, 0.0);
  out.clear();
  filter.Process({last}, out);
  TEST(out.empty(), ());
  filter.Finalize(out);
  TEST_EQUAL(out.size(), 1, ());
  TEST_EQUAL(out[0].m_timestamp, last.m_timestamp, ());
  TEST_EQUAL(out[0].m_longitude, last.m_longitude, ());
}
}  // namespace gps_track_filter_test
