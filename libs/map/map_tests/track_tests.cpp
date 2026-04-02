#include "testing/testing.hpp"

#include "map/elevation_info.hpp"
#include "map/track.hpp"
#include "map/track_statistics.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "kml/types.hpp"

namespace track_tests
{
using namespace geometry;
using namespace location;

double constexpr kEqualPointsEps = 1e-9;

GpsInfo const BuildGpsInfo(double latitude, double longitude, double altitude, double timestamp = 0)
{
  GpsInfo gpsInfo;
  gpsInfo.m_latitude = latitude;
  gpsInfo.m_longitude = longitude;
  gpsInfo.m_altitude = altitude;
  gpsInfo.m_timestamp = timestamp;
  return gpsInfo;
}

Track MakeTrack(std::initializer_list<std::initializer_list<PointWithAltitude>> const & lines)
{
  kml::TrackData data;
  data.m_layers.push_back(kml::TrackLayer());
  for (auto const & line : lines)
  {
    data.m_geometry.m_lines.emplace_back(line.begin(), line.end());
    data.m_geometry.m_timestamps.emplace_back();
  }
  return Track(std::move(data));
}

// ===================== ElevationInfo tests =====================

UNIT_TEST(ElevationInfo_EmptyMultiGeometry)
{
  ElevationInfo ei;
  auto const & lines = ei.GetLines();
  TEST_EQUAL(0, lines.size(), ());
}

UNIT_TEST(ElevationInfo_FromMultiGeometry)
{
  using PWA = PointWithAltitude;
  kml::MultiGeometry geometry;
  PWA const point1({0.0, 0.0}, 100);
  PWA const point2({1.0, 1.0}, 150);
  PWA const point3({2.0, 2.0}, 50);
  geometry.AddLine({point1, point2, point3});
  ElevationInfo ei(geometry.m_lines);

  auto const & lines = ei.GetLines();
  TEST_EQUAL(1, lines.size(), ());

  auto const & line = lines[0];
  TEST_EQUAL(3, line.size(), ());

  // Each line starts from distance 0.
  TEST_EQUAL(line[0].m_distance, 0, ());
  TEST_EQUAL(line[0].m_altitude, 100, ());

  double d1 = mercator::DistanceOnEarth(point1, point2);
  TEST_EQUAL(line[1].m_distance, d1, ());
  TEST_EQUAL(line[1].m_altitude, 150, ());

  double d2 = d1 + mercator::DistanceOnEarth(point2, point3);
  TEST_EQUAL(line[2].m_distance, d2, ());
  TEST_EQUAL(line[2].m_altitude, 50, ());
}

UNIT_TEST(ElevationInfo_MultipleLines_IndependentDistances)
{
  using PWA = PointWithAltitude;
  kml::MultiGeometry geometry;
  geometry.AddLine({PWA({0.0, 0.0}, 100), PWA({1.0, 1.0}, 150)});
  geometry.AddLine({PWA({2.0, 2.0}, 50), PWA({3.0, 3.0}, 75)});
  ElevationInfo ei(geometry.m_lines);

  auto const & lines = ei.GetLines();
  TEST_EQUAL(2, lines.size(), ());
  TEST_EQUAL(2, lines[0].size(), ());
  TEST_EQUAL(2, lines[1].size(), ());

  // Each line starts from 0.
  TEST_EQUAL(lines[0][0].m_distance, 0, ());
  TEST_EQUAL(lines[1][0].m_distance, 0, ());
}

UNIT_TEST(ElevationInfo_BuildWithGpsPoints)
{
  GpsTrackElevation gpsElevation;
  gpsElevation.AddGpsPoints({
      BuildGpsInfo(0.0, 0.0, 0),
      BuildGpsInfo(1.0, 1.0, 50),
      BuildGpsInfo(2.0, 2.0, 100),
  });
  gpsElevation.AddGpsPoints({BuildGpsInfo(3.0, 3.0, -50)});
  gpsElevation.AddGpsPoints({BuildGpsInfo(4.0, 4.0, 0)});
  gpsElevation.AddGpsPoints({});

  auto const & lines = gpsElevation.GetLines();
  TEST_EQUAL(5, gpsElevation.GetSize(), ());
  TEST_EQUAL(1, lines.size(), ());
}

// ===================== Track::GetPoint tests =====================

UNIT_TEST(Track_GetPoint_SingleLine_FirstPoint)
{
  auto const p1 = PointWithAltitude({0.0, 0.0}, 100);
  auto const p2 = PointWithAltitude({1.0, 0.0}, 200);
  Track track = MakeTrack({{p1, p2}});

  TEST_ALMOST_EQUAL_ABS(track.GetPoint(0.0), p1.GetPoint(), kEqualPointsEps, ());
}

UNIT_TEST(Track_GetPoint_SingleLine_LastPoint)
{
  auto const p1 = PointWithAltitude({0.0, 0.0}, 100);
  auto const p2 = PointWithAltitude({1.0, 0.0}, 200);
  Track track = MakeTrack({{p1, p2}});

  double const totalLen = mercator::DistanceOnEarth(p1.GetPoint(), p2.GetPoint());
  TEST_ALMOST_EQUAL_ABS(track.GetPoint(totalLen), p2.GetPoint(), kEqualPointsEps, ());
}

UNIT_TEST(Track_GetPoint_SingleLine_MidSegment)
{
  auto const p1 = PointWithAltitude({0.0, 0.0}, 100);
  auto const p2 = PointWithAltitude({2.0, 0.0}, 200);
  Track track = MakeTrack({{p1, p2}});

  double const totalLen = mercator::DistanceOnEarth(p1.GetPoint(), p2.GetPoint());
  TEST_ALMOST_EQUAL_ABS(track.GetPoint(totalLen / 2), m2::PointD(1.0, 0.0), 0.01, ());
}

UNIT_TEST(Track_GetPoint_ExactIntermediatePoint)
{
  using PWA = PointWithAltitude;
  PWA const p1({0.0, 0.0}, 100);
  PWA const p2({1.0, 0.0}, 150);
  PWA const p3({2.0, 0.0}, 200);
  Track track = MakeTrack({{p1, p2, p3}});

  double const d = mercator::DistanceOnEarth(p1.GetPoint(), p2.GetPoint());
  TEST_ALMOST_EQUAL_ABS(track.GetPoint(d), p2.GetPoint(), kEqualPointsEps, ());
}

UNIT_TEST(Track_GetPoint_MultiLine)
{
  using PWA = PointWithAltitude;
  PWA const p1({0.0, 0.0}, 100);
  PWA const p2({1.0, 0.0}, 150);
  PWA const p3({2.0, 0.0}, 50);
  PWA const p4({3.0, 0.0}, 75);
  Track track = MakeTrack({{p1, p2}, {p3, p4}});

  double const len1 = mercator::DistanceOnEarth(p1.GetPoint(), p2.GetPoint());
  double const len2 = mercator::DistanceOnEarth(p3.GetPoint(), p4.GetPoint());

  TEST_ALMOST_EQUAL_ABS(track.GetPoint(len1), p2.GetPoint(), kEqualPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(track.GetPoint(len1 + len2), p4.GetPoint(), kEqualPointsEps, ());
}

UNIT_TEST(Track_GetPoint_BeyondEnd)
{
  auto const p1 = PointWithAltitude({0.0, 0.0}, 100);
  auto const p2 = PointWithAltitude({1.0, 0.0}, 200);
  Track track = MakeTrack({{p1, p2}});

  double const totalLen = mercator::DistanceOnEarth(p1.GetPoint(), p2.GetPoint());
  TEST_ALMOST_EQUAL_ABS(track.GetPoint(totalLen * 2), p2.GetPoint(), kEqualPointsEps, ());
}

UNIT_TEST(Track_GetPoint_NegativeDistance)
{
  auto const p1 = PointWithAltitude({0.0, 0.0}, 100);
  auto const p2 = PointWithAltitude({1.0, 0.0}, 200);
  Track track = MakeTrack({{p1, p2}});

  TEST_ALMOST_EQUAL_ABS(track.GetPoint(-1.0), p1.GetPoint(), kEqualPointsEps, ());
}

// ===================== TrackStatistics tests =====================

UNIT_TEST(TrackStatistics_EmptyMultiGeometry)
{
  TrackStatistics ts;
  TEST_EQUAL(0, ts.m_ascent, ());
  TEST_EQUAL(0, ts.m_descent, ());
  TEST_EQUAL(ts.m_minElevation, kDefaultAltitudeMeters, ());
  TEST_EQUAL(ts.m_maxElevation, kDefaultAltitudeMeters, ());
}

UNIT_TEST(TrackStatistics_FromMultiGeometry)
{
  using PWA = PointWithAltitude;
  kml::MultiGeometry geometry;
  PWA const point1({0.0, 0.0}, 100);
  PWA const point2({1.0, 1.0}, 150);
  PWA const point3({2.0, 2.0}, 50);
  geometry.AddLine({point1, point2, point3});
  geometry.AddTimestamps({0, 1, 2});

  auto const ts = TrackStatistics(geometry);
  TEST_EQUAL(ts.m_minElevation, 50, ());
  TEST_EQUAL(ts.m_maxElevation, 150, ());
  TEST_EQUAL(ts.m_ascent, 50, ());    // Ascent from 100 -> 150
  TEST_EQUAL(ts.m_descent, 100, ());  // Descent from 150 -> 50
  TEST_EQUAL(ts.m_duration, 2, ());

  double distance = 0;
  distance += mercator::DistanceOnEarth(point1, point2);
  distance += mercator::DistanceOnEarth(point2, point3);
  TEST_EQUAL(ts.m_length, distance, ());
}

UNIT_TEST(TrackStatistics_NoAltitudeAndTimestampPoints)
{
  using PWA = PointWithAltitude;
  kml::MultiGeometry geometry;
  geometry.AddLine({PWA({0.0, 0.0}), PWA({1.0, 1.0}), PWA({2.0, 2.0})});

  auto const ts = TrackStatistics(geometry);

  TEST_EQUAL(ts.m_minElevation, kDefaultAltitudeMeters, ());
  TEST_EQUAL(ts.m_maxElevation, kDefaultAltitudeMeters, ());
  TEST_EQUAL(ts.m_ascent, 0, ());
  TEST_EQUAL(ts.m_descent, 0, ());
  TEST_EQUAL(ts.m_duration, 0, ());
}

UNIT_TEST(TrackStatistics_MultipleLines)
{
  using PWA = PointWithAltitude;
  kml::MultiGeometry geometry;
  geometry.AddLine({PWA({0.0, 0.0}, 100), PWA({1.0, 1.0}, 150), PWA({1.0, 1.0}, 140)});
  geometry.AddTimestamps({0, 1, 2});
  geometry.AddLine({PWA({2.0, 2.0}, 50), PWA({3.0, 3.0}, 75), PWA({3.0, 3.0}, 60)});
  geometry.AddTimestamps({0, 0, 0});
  geometry.AddLine({PWA({4.0, 4.0}, 200), PWA({5.0, 5.0}, 250)});
  geometry.AddTimestamps({4, 5});
  auto const ts = TrackStatistics(geometry);

  TEST_EQUAL(ts.m_minElevation, 50, ());
  TEST_EQUAL(ts.m_maxElevation, 250, ());
  TEST_EQUAL(ts.m_ascent, 125, ());  // Ascent from 100 -> 150, 50 -> 75, 200 -> 250
  TEST_EQUAL(ts.m_descent, 25, ());  // Descent from 150 -> 140, 75 -> 60
  TEST_EQUAL(ts.m_duration, 3, ());
}

UNIT_TEST(TrackStatistics_WithGpsPoints)
{
  std::vector<GpsInfo> const arrData[] = {
      {BuildGpsInfo(0.0, 0.0, 0, 0), BuildGpsInfo(1.0, 1.0, 50, 1), BuildGpsInfo(2.0, 2.0, 100, 2)},
      {BuildGpsInfo(3.0, 3.0, -50, 5)},
      {BuildGpsInfo(4.0, 4.0, 0, 10)}};
  TrackStatistics ts;
  for (auto const & pointsList : arrData)
    for (auto const & point : pointsList)
      ts.AddGpsInfoPoint(point);

  TEST_EQUAL(ts.m_minElevation, -50, ());
  TEST_EQUAL(ts.m_maxElevation, 100, ());
  TEST_EQUAL(ts.m_ascent, 150, ());   // Ascent from 0 -> 50, 50 -> 100, -50 -> 0
  TEST_EQUAL(ts.m_descent, 150, ());  // Descent from 100 -> -50
  TEST_EQUAL(ts.m_duration, 10, ());
}

UNIT_TEST(TrackStatistics_PositiveAndNegativeAltitudes)
{
  using PWA = PointWithAltitude;
  kml::MultiGeometry geometry;
  geometry.AddLine({PWA({0.0, 0.0}, -10), PWA({1.0, 1.0}, 20), PWA({2.0, 2.0}, -5), PWA({3.0, 3.0}, 15)});
  auto const ts = TrackStatistics(geometry);

  TEST_EQUAL(ts.m_minElevation, -10, ());
  TEST_EQUAL(ts.m_maxElevation, 20, ());
  TEST_EQUAL(ts.m_ascent, 50, ());   // Ascent from -10 -> 20 and -5 -> 15
  TEST_EQUAL(ts.m_descent, 25, ());  // Descent from 20 -> -5
}

UNIT_TEST(TrackStatistics_SmallAltitudeDelta)
{
  GpsInfo const arrPoints[] = {BuildGpsInfo(0.0, 0.0, 0),   BuildGpsInfo(1.0, 1.0, 0.2), BuildGpsInfo(2.0, 2.0, 0.4),
                               BuildGpsInfo(3.0, 3.0, 0.6), BuildGpsInfo(4.0, 4.0, 0.8), BuildGpsInfo(5.0, 5.0, 1.0)};

  TrackStatistics ts;
  for (auto const & point : arrPoints)
    ts.AddGpsInfoPoint(point);

  TEST_EQUAL(ts.m_minElevation, 0, ());
  TEST_EQUAL(ts.m_maxElevation, 1.0, ());
  TEST_EQUAL(ts.m_ascent, 1.0, ());
  TEST_EQUAL(ts.m_descent, 0, ());
}

UNIT_TEST(TrackStatistics_MixedMultiGeometryAndGpsPoints)
{
  using PWA = PointWithAltitude;
  kml::MultiGeometry geometry;
  geometry.AddLine({PWA({0.0, 0.0}, 100), PWA({1.0, 1.0}, 150), PWA({2.0, 2.0}, 50)});
  geometry.AddTimestamps({5, 6, 7});

  auto ts = TrackStatistics(geometry);

  std::vector<GpsInfo> const points = {BuildGpsInfo(3.0, 3.0, 60, 8), BuildGpsInfo(4.0, 4.0, 160, 10),
                                       BuildGpsInfo(4.0, 4.0, 20, 15)};

  for (auto const & point : points)
    ts.AddGpsInfoPoint(point);

  TEST_EQUAL(ts.m_minElevation, 20, ());
  TEST_EQUAL(ts.m_maxElevation, 160, ());
  TEST_EQUAL(ts.m_ascent, 160, ());   // 50 + 10 + 100
  TEST_EQUAL(ts.m_descent, 240, ());  // 100 + 140
  TEST_EQUAL(ts.m_duration, 10, ());  // 10
}
}  // namespace track_tests
