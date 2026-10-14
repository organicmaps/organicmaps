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
