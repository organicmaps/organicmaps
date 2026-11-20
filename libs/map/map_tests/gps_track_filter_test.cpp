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

UNIT_TEST(GpsTrackFilter_InaccurateFixKeepsNextFixes)
{
  Points const walk = MakeWalk(6);
  // A coarse fix, e.g. from Wi-Fi after GPS is lost indoors, between accurate ones.
  auto coarse = Make(102.5, 0.0, 10000.0 * kDegPerMeter, 15.0);
  coarse.m_horizontalAccuracy = 1000;
  GpsTrackFilter filter;
  Points out;
  filter.Process({walk[0], walk[1], walk[2], coarse, walk[3], walk[4], walk[5]}, out);
  TEST_EQUAL(out.size(), 6, ());
  TEST_EQUAL(out[3].m_timestamp, walk[3].m_timestamp, ());
}

UNIT_TEST(GpsTrackFilter_FinalizeSkipsInaccurateFix)
{
  GpsTrackFilter filter;
  Points out;
  filter.Process(MakeWalk(3), out);
  TEST_EQUAL(out.size(), 3, ());

  // A coarse fix, e.g. from Wi-Fi after GPS is lost indoors, is not force-appended either.
  auto coarse = Make(103.0, 0.0, 10000.0 * kDegPerMeter, 15.0);
  coarse.m_horizontalAccuracy = 1000;
  out.clear();
  filter.Process({coarse}, out);
  TEST(out.empty(), ());
  filter.Finalize(out);
  TEST(out.empty(), ());
}
}  // namespace gps_track_filter_test
