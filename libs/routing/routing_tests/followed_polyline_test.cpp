#include "testing/testing.hpp"

#include "routing/base/followed_polyline.hpp"

#include "geometry/mercator.hpp"
#include "geometry/polyline2d.hpp"

namespace followed_polyline_test
{
using namespace routing;

namespace
{
static m2::PolylineD const kTestDirectedPolyline1(std::vector<m2::PointD>{{0.0, 0.0}, {3.0, 0.0}, {5.0, 0.0}});
static m2::PolylineD const kTestDirectedPolyline2(std::vector<m2::PointD>{{6.0, 0.0}, {7.0, 0.0}});
}  // namespace

UNIT_TEST(FollowedPolylineAppend)
{
  FollowedPolyline followedPolyline1(kTestDirectedPolyline1.Begin(), kTestDirectedPolyline1.End());
  FollowedPolyline const followedPolyline2(kTestDirectedPolyline2.Begin(), kTestDirectedPolyline2.End());

  TEST_EQUAL(followedPolyline1.GetPolyline(), kTestDirectedPolyline1, ());
  followedPolyline1.Append(followedPolyline2);
  TEST_EQUAL(followedPolyline1.GetPolyline().GetSize(), 5, ());

  m2::PolylineD polyline1 = kTestDirectedPolyline1;
  polyline1.Append(kTestDirectedPolyline2);
  TEST_EQUAL(followedPolyline1.GetPolyline(), polyline1, ());
}

UNIT_TEST(FollowedPolylinePop)
{
  FollowedPolyline followedPolyline(kTestDirectedPolyline1.Begin(), kTestDirectedPolyline1.End());

  TEST_EQUAL(followedPolyline.GetPolyline(), kTestDirectedPolyline1, ());
  TEST_EQUAL(followedPolyline.GetPolyline().GetSize(), 3, ());
  followedPolyline.PopBack();
  TEST_EQUAL(followedPolyline.GetPolyline().GetSize(), 2, ());
}

UNIT_TEST(FollowedPolylineInitializationFogTest)
{
  FollowedPolyline polyline(kTestDirectedPolyline1.Begin(), kTestDirectedPolyline1.End());
  TEST(polyline.IsValid(), ());
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  TEST_EQUAL(polyline.GetPolyline().GetSize(), 3, ());
}

UNIT_TEST(FollowedPolylineFollowingTestByProjection)
{
  FollowedPolyline polyline(kTestDirectedPolyline1.Begin(), kTestDirectedPolyline1.End());
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  polyline.UpdateProjection(mercator::RectByCenterXYAndSizeInMeters({0, 0}, 2));
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(0, 0), ());
  polyline.UpdateProjection(mercator::RectByCenterXYAndSizeInMeters({1, 0}, 2));
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(1, 0), ());
  polyline.UpdateProjection(mercator::RectByCenterXYAndSizeInMeters({4, 0}, 2));
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 1, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(4, 0), ());
  auto iter = polyline.UpdateProjection(mercator::RectByCenterXYAndSizeInMeters({5.0001, 0}, 1));
  TEST(!iter.IsValid(), ());
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 1, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(4, 0), ());
  polyline.UpdateProjection(mercator::RectByCenterXYAndSizeInMeters({5.0001, 0}, 2000));
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 1, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(5, 0), ());
}

UNIT_TEST(FollowedPolylineDistanceCalculationTest)
{
  // Test full length case.
  FollowedPolyline polyline(kTestDirectedPolyline1.Begin(), kTestDirectedPolyline1.End());
  double distance = polyline.GetDistanceM(polyline.Begin(), polyline.End());
  double masterDistance = mercator::DistanceOnEarth(kTestDirectedPolyline1.Front(), kTestDirectedPolyline1.Back());
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());
  distance = polyline.GetTotalDistanceMeters();
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());

  // Test partial length case.
  polyline.UpdateProjection(mercator::RectByCenterXYAndSizeInMeters({3, 0}, 2));
  distance = polyline.GetDistanceM(polyline.GetCurrentIter(), polyline.End());
  masterDistance = mercator::DistanceOnEarth(kTestDirectedPolyline1.GetPoint(1), kTestDirectedPolyline1.Back());
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());
  distance = polyline.GetDistanceToEndMeters();
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());

  // Test point in the middle case.
  polyline.UpdateProjection(mercator::RectByCenterXYAndSizeInMeters({4, 0}, 2));
  distance = polyline.GetDistanceM(polyline.GetCurrentIter(), polyline.End());
  masterDistance = mercator::DistanceOnEarth(m2::PointD(4, 0), kTestDirectedPolyline1.Back());
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());
  distance = polyline.GetDistanceToEndMeters();
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());
}

UNIT_TEST(FollowedPolylineDirectionTest)
{
  m2::PolylineD testPolyline(std::vector<m2::PointD>{{0, 0}, {1.00003, 0}, {1.00003, 1}});
  FollowedPolyline polyline(testPolyline.Begin(), testPolyline.End());
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  m2::PointD directionPoint;
  polyline.GetCurrentDirectionPoint(directionPoint, 20);
  TEST_EQUAL(directionPoint, testPolyline.GetPoint(1), ());
  polyline.UpdateProjection(mercator::RectByCenterXYAndSizeInMeters({1.0, 0}, 2));
  polyline.GetCurrentDirectionPoint(directionPoint, 0.0001);
  TEST_EQUAL(directionPoint, testPolyline.GetPoint(1), ());
  polyline.GetCurrentDirectionPoint(directionPoint, 20);
  TEST_EQUAL(directionPoint, testPolyline.GetPoint(2), ());
}

UNIT_TEST(FollowedPolylineGetDistanceFromBeginM)
{
  m2::PolylineD testPolyline(std::vector<m2::PointD>{{0, 0}, {1, 0}, {2, 0}, {3, 0}, {5, 0}, {6, 0}});
  FollowedPolyline polyline(testPolyline.Begin(), testPolyline.End());
  m2::PointD point(4, 0);
  polyline.UpdateProjection(mercator::RectByCenterXYAndSizeInMeters(point, 2));

  double const distance = polyline.GetDistanceFromStartMeters();
  double const masterDistance = mercator::DistanceOnEarth(kTestDirectedPolyline1.Front(), point);
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());
}

namespace
{
m2::PointD TrackPoint(double x, double y = 0.0)
{
  return {mercator::MetersToMercator(x), mercator::MetersToMercator(y)};
}

m2::RectD TrackFixRect(double x, double y = 0.0, double radiusM = 5.0)
{
  auto const latlon = mercator::ToLatLon(TrackPoint(x, y));
  return mercator::MetersToXY(latlon.m_lon, latlon.m_lat, radiusM);
}
}  // namespace

UNIT_TEST(FollowedPolyline_TrackWindowDoesNotSkipLoop)
{
  std::vector<m2::PointD> const points = {TrackPoint(0), TrackPoint(200), TrackPoint(200, 200), TrackPoint(0, 200),
                                          TrackPoint(0)};
  FollowedPolyline poly(points.begin(), points.end());
  for (size_t i = 0; i < 3; ++i)
  {
    TEST(poly.UpdateTrackMatchingProjection(
             {.m_positionRect = TrackFixRect(0), .m_minProgressM = 0.0, .m_maxProgressM = 1000.0, .m_toleranceM = 5.0}),
         ());
    TEST_EQUAL(poly.GetDistanceFromStartMeters(), 0.0, ("A shared finish coordinate is not an observed lap"));
  }
  TEST(!poly.UpdateTrackMatchingProjection({.m_positionRect = TrackFixRect(200, 200),
                                            .m_minProgressM = 0.0,
                                            .m_maxProgressM = 100.0,
                                            .m_toleranceM = 5.0}),
       ());
  TEST_EQUAL(poly.GetDistanceFromStartMeters(), 0.0, ("Rejected fixes leave progress unchanged"));
}

UNIT_TEST(FollowedPolyline_TrackTurnaroundRequiresObservedCorner)
{
  std::vector<m2::PointD> const points = {TrackPoint(0), TrackPoint(1000), TrackPoint(0)};
  FollowedPolyline poly(points.begin(), points.end());
  TEST(poly.UpdateTrackMatchingProjection({.m_positionRect = TrackFixRect(700),
                                           .m_minProgressM = 0.0,
                                           .m_maxProgressM = 2000.0,
                                           .m_direction = m2::PointD(1, 0),
                                           .m_toleranceM = 5.0}),
       ());
  TEST(poly.UpdateTrackMatchingProjection({.m_positionRect = TrackFixRect(400),
                                           .m_minProgressM = 0.0,
                                           .m_maxProgressM = 2000.0,
                                           .m_direction = m2::PointD(-1, 0),
                                           .m_toleranceM = 5.0}),
       ());
  TEST_ALMOST_EQUAL_ABS(poly.GetDistanceFromStartMeters(), 400.0, 1.0, ("Backtracking remains on the outbound visit"));
  TEST(poly.UpdateTrackMatchingProjection({.m_positionRect = TrackFixRect(1000),
                                           .m_minProgressM = 0.0,
                                           .m_maxProgressM = 2000.0,
                                           .m_direction = m2::PointD(1, 0),
                                           .m_toleranceM = 5.0}),
       ());
  TEST(poly.UpdateTrackMatchingProjection({.m_positionRect = TrackFixRect(900),
                                           .m_minProgressM = 0.0,
                                           .m_maxProgressM = 2000.0,
                                           .m_direction = m2::PointD(-1, 0),
                                           .m_toleranceM = 5.0}),
       ());
  TEST_ALMOST_EQUAL_ABS(poly.GetDistanceFromStartMeters(), 1100.0, 3.0,
                        ("An observed corner permits the return visit"));
}

UNIT_TEST(FollowedPolyline_TrackDenseForwardChainAndDuplicateVertices)
{
  std::vector<m2::PointD> points = {TrackPoint(0)};
  for (size_t i = 0; i <= 100; ++i)
    points.push_back(TrackPoint(i));
  FollowedPolyline poly(points.begin(), points.end());
  TEST(poly.UpdateTrackMatchingProjection({.m_positionRect = TrackFixRect(97),
                                           .m_minProgressM = 0.0,
                                           .m_maxProgressM = 120.0,
                                           .m_direction = m2::PointD(1, 0),
                                           .m_toleranceM = 5.0,
                                           .m_forwardDirection = m2::PointD(1, 0)}),
       ());
  TEST_ALMOST_EQUAL_ABS(poly.GetDistanceFromStartMeters(), 97.0, 1.0, ());
  double const distance = poly.GetDistanceFromStartMeters();
  TEST(poly.UpdateTrackMatchingProjection(
           {.m_positionRect = TrackFixRect(97), .m_minProgressM = 0.0, .m_maxProgressM = 120.0, .m_toleranceM = 5.0}),
       ());
  TEST_EQUAL(poly.GetDistanceFromStartMeters(), distance, ("An unchanged fix does not march through the source"));
}

UNIT_TEST(FollowedPolyline_TrackBacktrackingKeepsSpatialTolerance)
{
  std::vector<m2::PointD> points;
  for (int x = 0; x <= 1000; ++x)
    points.push_back(TrackPoint(x));
  FollowedPolyline poly(points.begin(), points.end());
  for (double const x : {0.0, 500.0, 400.0, 400.0})
  {
    TEST(poly.UpdateTrackMatchingProjection(
             {.m_positionRect = TrackFixRect(x, 0.0, 20.0), .m_maxProgressM = 1100.0, .m_toleranceM = 5.0}),
         (x));
    TEST_LESS_OR_EQUAL(mercator::DistanceOnEarth(poly.GetCurrentIter().m_pt, TrackPoint(x)), 5.0 + 1.0E-6,
                       ("An exact projection exists within the window", x));
  }
}

UNIT_TEST(FollowedPolyline_LastNonZeroSegmentCacheFollowsGeometry)
{
  std::vector<m2::PointD> const points = {TrackPoint(0), TrackPoint(10), TrackPoint(20), TrackPoint(20)};
  FollowedPolyline poly(points.begin(), points.end());
  double const firstLength = mercator::DistanceOnEarth(points[0], points[1]);
  TEST_ALMOST_EQUAL_ABS(poly.GetLastNonZeroSegmentStartMeters(), firstLength, 1.0E-6, ());
  std::vector<m2::PointD> const tail = {TrackPoint(20), TrackPoint(30)};
  FollowedPolyline other(tail.begin(), tail.end());
  poly.Append(other);
  TEST_ALMOST_EQUAL_ABS(poly.GetLastNonZeroSegmentStartMeters(), 2.0 * firstLength, 1.0E-6, ());
  poly.PopBack();
  TEST_ALMOST_EQUAL_ABS(poly.GetLastNonZeroSegmentStartMeters(), firstLength, 1.0E-6, ());
  poly.Swap(other);
  TEST_EQUAL(poly.GetLastNonZeroSegmentStartMeters(), 0.0, ());
  TEST_ALMOST_EQUAL_ABS(other.GetLastNonZeroSegmentStartMeters(), firstLength, 1.0E-6, ());
}
}  // namespace followed_polyline_test
