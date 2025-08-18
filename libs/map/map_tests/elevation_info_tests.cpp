#include "testing/testing.hpp"

#include "map/elevation_info.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "kml/types.hpp"

namespace elevation_info_tests
{
using namespace geometry;
using namespace location;

GpsInfo const BuildGpsInfo(double latitude, double longitude, double altitude)
{
  GpsInfo gpsInfo;
  gpsInfo.m_latitude = latitude;
  gpsInfo.m_longitude = longitude;
  gpsInfo.m_altitude = altitude;
  return gpsInfo;
}

UNIT_TEST(ElevationInfo_EmptyMultiGeometry)
{
  ElevationInfo ei;
  TEST_EQUAL(0, ei.GetSize(), ());
}

UNIT_TEST(ElevationInfo_FromMultiGeometry)
{
  kml::MultiGeometry geometry;
  auto const point1 = PointWithAltitude({0.0, 0.0}, 100);
  auto const point2 = PointWithAltitude({1.0, 1.0}, 150);
  auto const point3 = PointWithAltitude({2.0, 2.0}, 50);
  geometry.AddLine({point1, point2, point3});
  ElevationInfo ei(geometry.m_lines);

  TEST_EQUAL(3, ei.GetSize(), ());

  double distance = 0;
  TEST_EQUAL(ei.GetPoints()[0].m_distance, distance, ());
  distance += mercator::DistanceOnEarth(point1, point2);
  TEST_EQUAL(ei.GetPoints()[1].m_distance, distance, ());
  distance += mercator::DistanceOnEarth(point2, point3);
  TEST_EQUAL(ei.GetPoints()[2].m_distance, distance, ());
}

UNIT_TEST(ElevationInfo_MultipleLines)
{
  kml::MultiGeometry geometry;
  geometry.AddLine(
      {PointWithAltitude({0.0, 0.0}, 100), PointWithAltitude({1.0, 1.0}, 150), PointWithAltitude({1.0, 1.0}, 140)});
  geometry.AddLine(
      {PointWithAltitude({2.0, 2.0}, 50), PointWithAltitude({3.0, 3.0}, 75), PointWithAltitude({3.0, 3.0}, 60)});
  geometry.AddLine({PointWithAltitude({4.0, 4.0}, 200), PointWithAltitude({5.0, 5.0}, 250)});
  ElevationInfo ei(geometry.m_lines);

  TEST_EQUAL(8, ei.GetSize(), ());
}

UNIT_TEST(ElevationInfo_SegmentDistances)
{
  kml::MultiGeometry geometry;
  geometry.AddLine({PointWithAltitude({0.0, 0.0}), PointWithAltitude({1.0, 0.0})});
  geometry.AddLine({PointWithAltitude({2.0, 0.0}), PointWithAltitude({3.0, 0.0})});
  geometry.AddLine({PointWithAltitude({4.0, 0.0}), PointWithAltitude({5.0, 0.0})});

  ElevationInfo ei(geometry.m_lines);
  auto const & segmentDistances = ei.GetSegmentsDistances();
  auto const points = ei.GetPoints();

  TEST_EQUAL(segmentDistances.size(), 2, ());
  TEST_EQUAL(segmentDistances[0], ei.GetPoints()[2].m_distance, ());
  TEST_EQUAL(segmentDistances[1], ei.GetPoints()[4].m_distance, ());
}

UNIT_TEST(ElevationInfo_BuildWithGpsPoints)
{
  auto ei = ElevationInfo();
  ei.AddGpsPoints({
      BuildGpsInfo(0.0, 0.0, 0),
      BuildGpsInfo(1.0, 1.0, 50),
      BuildGpsInfo(2.0, 2.0, 100),
  });
  ei.AddGpsPoints({BuildGpsInfo(3.0, 3.0, -50)});
  ei.AddGpsPoints({BuildGpsInfo(4.0, 4.0, 0)});
  ei.AddGpsPoints({});

  TEST_EQUAL(5, ei.GetSize(), ());
  TEST_EQUAL(ei.GetSegmentsDistances().size(), 0, ());
}
}  // namespace elevation_info_tests
