#include "testing/testing.hpp"

#include "map/elevation_info.hpp"
#include "map/track.hpp"
#include "map/track_statistics.hpp"

#include "kml/serdes_gpx.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"

#include <vector>

namespace track_tests
{
using namespace geometry;
using namespace location;

double constexpr kEqualPointsEps = 1e-9;

GpsInfo BuildGpsInfo(double latitude, double longitude, double altitude, double timestamp = 0)
{
  GpsInfo gpsInfo;
  gpsInfo.m_latitude = latitude;
  gpsInfo.m_longitude = longitude;
  gpsInfo.m_altitude = altitude;
  gpsInfo.m_verticalAccuracy = 1.0;  // Mark altitude as valid.
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

// ===================== UpdateSelectionInfo tests =====================

UNIT_TEST(Track_UpdateSelectionInfo_OnSegment)
{
  using PWA = PointWithAltitude;
  Track track = MakeTrack({{PWA({0.0, 0.0}, 0), PWA({10.0, 0.0}, 0)}});

  Track::TrackSelectionInfo info;
  info.m_squareDist = 1.0;  // allow up to distance 1
  track.UpdateSelectionInfo(m2::PointD(5.0, 0.0), info);

  TEST_EQUAL(info.m_trackId, track.GetData().m_id, ());
  TEST_ALMOST_EQUAL_ABS(info.m_trackPoint, m2::PointD(5.0, 0.0), kEqualPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_squareDist, 0.0, kEqualPointsEps, ());
}

UNIT_TEST(Track_UpdateSelectionInfo_PerpendicularFoot)
{
  using PWA = PointWithAltitude;
  Track track = MakeTrack({{PWA({0.0, 0.0}, 0), PWA({10.0, 0.0}, 0)}});

  // Tap above segment: closest point is the perpendicular foot.
  Track::TrackSelectionInfo info;
  info.m_squareDist = 1.0;  // 0.5^2 = 0.25 < 1, should accept
  track.UpdateSelectionInfo(m2::PointD(3.0, 0.5), info);

  TEST_EQUAL(info.m_trackId, track.GetData().m_id, ());
  TEST_ALMOST_EQUAL_ABS(info.m_trackPoint, m2::PointD(3.0, 0.0), kEqualPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_squareDist, 0.25, kEqualPointsEps, ());
}

// Tap farther than the threshold: info is left untouched.
UNIT_TEST(Track_UpdateSelectionInfo_BeyondThreshold)
{
  using PWA = PointWithAltitude;
  Track track = MakeTrack({{PWA({0.0, 0.0}, 0), PWA({10.0, 0.0}, 0)}});

  Track::TrackSelectionInfo info;
  info.m_squareDist = 0.04;  // only accept within 0.2
  track.UpdateSelectionInfo(m2::PointD(5.0, 1.0), info);

  TEST_EQUAL(info.m_trackId, kml::kInvalidTrackId, ());
  TEST_ALMOST_EQUAL_ABS(info.m_squareDist, 0.04, kEqualPointsEps, ());
}

// Endpoint fallback: tap beyond the segment, closest point is the endpoint.
UNIT_TEST(Track_UpdateSelectionInfo_BeyondSegmentEnd)
{
  using PWA = PointWithAltitude;
  Track track = MakeTrack({{PWA({0.0, 0.0}, 0), PWA({10.0, 0.0}, 0)}});

  Track::TrackSelectionInfo info;
  info.m_squareDist = 100.0;
  track.UpdateSelectionInfo(m2::PointD(12.0, 0.0), info);

  TEST_EQUAL(info.m_trackId, track.GetData().m_id, ());
  TEST_ALMOST_EQUAL_ABS(info.m_trackPoint, m2::PointD(10.0, 0.0), kEqualPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_squareDist, 4.0, kEqualPointsEps, ());
}

// Multi-segment track: best segment wins.
UNIT_TEST(Track_UpdateSelectionInfo_MultiSegmentClosestWins)
{
  using PWA = PointWithAltitude;
  // L-shape: (0,0) -> (10,0) -> (10,10)
  Track track = MakeTrack({{PWA({0.0, 0.0}, 0), PWA({10.0, 0.0}, 0), PWA({10.0, 10.0}, 0)}});

  // Tap nearer to the vertical leg.
  Track::TrackSelectionInfo info;
  info.m_squareDist = 100.0;
  track.UpdateSelectionInfo(m2::PointD(9.0, 5.0), info);

  TEST_EQUAL(info.m_trackId, track.GetData().m_id, ());
  TEST_ALMOST_EQUAL_ABS(info.m_trackPoint, m2::PointD(10.0, 5.0), kEqualPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_squareDist, 1.0, kEqualPointsEps, ());
}

// Multi-line track: best line wins regardless of ordering.
UNIT_TEST(Track_UpdateSelectionInfo_MultiLine)
{
  using PWA = PointWithAltitude;
  Track track = MakeTrack({{PWA({0.0, 0.0}, 0), PWA({10.0, 0.0}, 0)}, {PWA({0.0, 100.0}, 0), PWA({10.0, 100.0}, 0)}});

  Track::TrackSelectionInfo info;
  info.m_squareDist = 10000.0;
  track.UpdateSelectionInfo(m2::PointD(5.0, 99.0), info);

  TEST_EQUAL(info.m_trackId, track.GetData().m_id, ());
  TEST_ALMOST_EQUAL_ABS(info.m_trackPoint, m2::PointD(5.0, 100.0), kEqualPointsEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_squareDist, 1.0, kEqualPointsEps, ());
}

// Subsequent call tightens the threshold via info.m_squareDist carry-over.
UNIT_TEST(Track_UpdateSelectionInfo_ThresholdTightensAcrossCalls)
{
  using PWA = PointWithAltitude;
  Track farTrack = MakeTrack({{PWA({0.0, 5.0}, 0), PWA({10.0, 5.0}, 0)}});
  Track nearTrack = MakeTrack({{PWA({0.0, 1.0}, 0), PWA({10.0, 1.0}, 0)}});

  Track::TrackSelectionInfo info;
  info.m_squareDist = 100.0;
  // Tap at (5, 0): far track is at dist 5 (sq=25), near track is at dist 1 (sq=1).
  farTrack.UpdateSelectionInfo(m2::PointD(5.0, 0.0), info);
  TEST_EQUAL(info.m_trackId, farTrack.GetData().m_id, ());
  TEST_ALMOST_EQUAL_ABS(info.m_squareDist, 25.0, kEqualPointsEps, ());

  nearTrack.UpdateSelectionInfo(m2::PointD(5.0, 0.0), info);
  TEST_EQUAL(info.m_trackId, nearTrack.GetData().m_id, ());
  TEST_ALMOST_EQUAL_ABS(info.m_squareDist, 1.0, kEqualPointsEps, ());
}

// Farther track is rejected when a closer one already set a tighter threshold.
UNIT_TEST(Track_UpdateSelectionInfo_FartherTrackRejected)
{
  using PWA = PointWithAltitude;
  Track nearTrack = MakeTrack({{PWA({0.0, 1.0}, 0), PWA({10.0, 1.0}, 0)}});
  Track farTrack = MakeTrack({{PWA({0.0, 5.0}, 0), PWA({10.0, 5.0}, 0)}});

  Track::TrackSelectionInfo info;
  info.m_squareDist = 100.0;
  nearTrack.UpdateSelectionInfo(m2::PointD(5.0, 0.0), info);
  auto const nearId = info.m_trackId;
  TEST_EQUAL(nearId, nearTrack.GetData().m_id, ());

  // Far track is beyond the tightened threshold: must not replace nearTrack.
  farTrack.UpdateSelectionInfo(m2::PointD(5.0, 0.0), info);
  TEST_EQUAL(info.m_trackId, nearId, ());
  TEST_ALMOST_EQUAL_ABS(info.m_squareDist, 1.0, kEqualPointsEps, ());
}

// No explicit threshold: default (max) accepts everything.
UNIT_TEST(Track_UpdateSelectionInfo_DefaultThresholdAcceptsAny)
{
  using PWA = PointWithAltitude;
  Track track = MakeTrack({{PWA({0.0, 0.0}, 0), PWA({10.0, 0.0}, 0)}});

  Track::TrackSelectionInfo info;
  // Default info.m_squareDist = max. Tap very far from the track.
  track.UpdateSelectionInfo(m2::PointD(1e6, 1e6), info);
  TEST_EQUAL(info.m_trackId, track.GetData().m_id, ());
  TEST_ALMOST_EQUAL_ABS(info.m_trackPoint, m2::PointD(10.0, 0.0), kEqualPointsEps, ());
}

// ===================== TrackStatistics tests =====================

UNIT_TEST(TrackStatistics_Duration)
{
  kml::MultiGeometry geometry;
  geometry.AddLine({PointWithAltitude({0.0, 0.0}, 100), PointWithAltitude({1.0, 1.0}, 150)});
  geometry.AddTimestamps({0, 5});
  geometry.AddLine({PointWithAltitude({2.0, 2.0}, 50), PointWithAltitude({3.0, 3.0}, 75)});
  geometry.AddTimestamps({10, 13});

  TrackStatistics ts;
  ts.CalculateDuration(geometry);
  TEST_EQUAL(ts.m_duration, 8, ());  // (5 - 0) + (13 - 10)
}

UNIT_TEST(TrackStatistics_GpsPoints)
{
  GpsInfo const points[] = {BuildGpsInfo(0.0, 0.0, 0, 0), BuildGpsInfo(1.0, 1.0, 50, 1), BuildGpsInfo(2.0, 2.0, 100, 2),
                            BuildGpsInfo(3.0, 3.0, -50, 5), BuildGpsInfo(4.0, 4.0, 0, 10)};

  TrackStatistics ts;
  for (auto const & point : points)
    ts.AddGpsInfoPoint(point);

  TEST_EQUAL(ts.m_minElevation, -50, ());
  TEST_EQUAL(ts.m_maxElevation, 100, ());
  TEST_EQUAL(ts.m_ascent, 150, ());   // 0->50, 50->100, -50->0
  TEST_EQUAL(ts.m_descent, 150, ());  // 100->-50
  TEST_EQUAL(ts.m_duration, 10, ());
  TEST_GREATER(ts.m_length, 0, ());
}

// ===================== Elevation simplification tests =====================
namespace
{
ElevationInfo::AltitudesInfo CalcAltitudesInfo(std::string const & fileName, Altitude threshold)
{
  kml::FileData fileData;
  kml::DeserializerGpx(fileData).Deserialize(FileReader(GetPlatform().TestsDataPathForFile(fileName)));
  TEST_EQUAL(fileData.m_tracksData.size(), 1, ());

  ElevationInfo ei(fileData.m_tracksData[0].m_geometry.m_lines);
  ei.Simplify();
  return ei.CalculateAltitudesInfo(threshold);
}
}  // namespace

// https://github.com/organicmaps/organicmaps/issues/5087#issuecomment-4177262973
UNIT_TEST(Elevation_AscentDescent_Transalpes)
{
  auto const altInfo = CalcAltitudesInfo("test_data/gpx/transalpes-2026-preparation.gpx", 15);

  // Paper guide: ~1550m ascent, ~850m descent.
  double constexpr kEps = 0.04;
  TEST_ALMOST_EQUAL_ABS(double(altInfo.GetTotalAscent()), 1550.0, 1550.0 * kEps, ());
  TEST_ALMOST_EQUAL_ABS(double(altInfo.GetTotalDescent()), 850.0, 850.0 * kEps, ());
}

// https://github.com/organicmaps/organicmaps/issues/5087#issuecomment-3390365431
UNIT_TEST(Elevation_AscentDescent_AllenMountain)
{
  auto const altInfo = CalcAltitudesInfo("test_data/gpx/East River Trail to Allen Mountain.gpx", 4);

  // Komoot: 2850ft (868m) ascent, 375ft (114m) descent.
  TEST_ALMOST_EQUAL_ABS(double(altInfo.GetTotalAscent()), 868.0, 868.0 * 0.01, ());
  TEST_ALMOST_EQUAL_ABS(double(altInfo.GetTotalDescent()), 114.0, 114.0 * 0.1, ());
}

// https://github.com/organicmaps/organicmaps/issues/5454#issuecomment-3140515619
UNIT_TEST(Elevation_AscentDescent_Solden_Merano)
{
  auto const altInfo = CalcAltitudesInfo("test_data/gpx/Solden (Tirol) to Merano (South Tirol).gpx", 6);

  // Google: 1374m up and 2427m down.
  double constexpr kEps = 0.05;
  TEST_ALMOST_EQUAL_ABS(double(altInfo.GetTotalAscent()), 1374.0, 1374.0 * kEps, ());
  TEST_ALMOST_EQUAL_ABS(double(altInfo.GetTotalDescent()), 2427.0, 2427.0 * kEps, ());
}

// https://github.com/organicmaps/organicmaps/issues/5087#issuecomment-4009485791
UNIT_TEST(Elevation_AscentDescent_Peak4120)
{
  auto const altInfo = CalcAltitudesInfo("test_data/gpx/Peak 4120.20260209.gpx", 10);

  // User said: 1647ft (502m) up and down.
  double constexpr kEps = 0.07;
  TEST_ALMOST_EQUAL_ABS(double(altInfo.GetTotalAscent()), 502.0, 502.0 * kEps, ());
  TEST_ALMOST_EQUAL_ABS(double(altInfo.GetTotalDescent()), 502.0, 502.0 * kEps, ());
}

UNIT_TEST(Elevation_SmoothSlopeOutliers)
{
  kml::FileData fileData;
  kml::DeserializerGpx(fileData).Deserialize(
      FileReader(GetPlatform().TestsDataPathForFile("test_data/gpx/transalpes-2026-preparation.gpx")));

  auto const & lines = fileData.m_tracksData[0].m_geometry.m_lines;

  {
    ElevationInfo ei(lines);
    ei.Simplify();
    auto const info = ei.CalculateAltitudesInfo(10);
    LOG_SHORT(LINFO, ("maxSlope = NONE", "ascent =", info.GetTotalAscent(), "descent =", info.GetTotalDescent()));
  }

  for (double maxSlope : {50.0, 70.0, 100.0, 150.0, 200.0})
  {
    ElevationInfo ei(lines);
    ei.SmoothSlopeOutliers(maxSlope);
    ei.Simplify();
    auto const info = ei.CalculateAltitudesInfo(10);
    LOG_SHORT(LINFO, ("maxSlope =", maxSlope, "ascent =", info.GetTotalAscent(), "descent =", info.GetTotalDescent()));
  }
}
}  // namespace track_tests
