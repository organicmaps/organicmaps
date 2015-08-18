#include "testing/testing.hpp"

#include "routing/base/followed_polyline.hpp"

#include "geometry/polyline2d.hpp"

namespace routing_test
{
using namespace routing;

namespace
{
  static const m2::PolylineD kTestDirectedPolyline({{0.0, 0.0}, {3.0, 0.0}, {5.0, 0.0}});
}  // namespace

UNIT_TEST(FollowedPolylineInitializationFogTest)
{
  FollowedPolyline polyline(kTestDirectedPolyline.Begin(), kTestDirectedPolyline.End());
  TEST(polyline.IsValid(), ());
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  TEST_EQUAL(polyline.GetPolyline().GetSize(), 3, ());
}

UNIT_TEST(FollowedPolylineFollowingTestByProjection)
{
  FollowedPolyline polyline(kTestDirectedPolyline.Begin(), kTestDirectedPolyline.End());
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  polyline.UpdateProjection(MercatorBounds::RectByCenterXYAndSizeInMeters({0, 0}, 2));
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(0, 0), ());
  polyline.UpdateProjection(MercatorBounds::RectByCenterXYAndSizeInMeters({1, 0}, 2));
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(1, 0), ());
  polyline.UpdateProjection(MercatorBounds::RectByCenterXYAndSizeInMeters({4, 0}, 2));
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 1, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(4, 0), ());
  auto iter = polyline.UpdateProjection(MercatorBounds::RectByCenterXYAndSizeInMeters({5.0001, 0}, 1));
  TEST(!iter.IsValid(), ());
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 1, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(4, 0), ());
  polyline.UpdateProjection(MercatorBounds::RectByCenterXYAndSizeInMeters({5.0001, 0}, 2000));
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 1, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(5, 0), ());
}

UNIT_TEST(FollowedPolylineFollowingTestByPrediction)
{
  m2::PolylineD testPolyline({{0, 0}, {0.003, 0}, {0.003, 1}});
  FollowedPolyline polyline(testPolyline.Begin(), testPolyline.End());
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  polyline.UpdateProjection(MercatorBounds::RectByCenterXYAndSizeInMeters({0, 0}, 2));
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  TEST_EQUAL(polyline.GetCurrentIter().m_pt, m2::PointD(0, 0), ());
  // Near 0 distances between lons and lats are almost equal.
  double dist = MercatorBounds::DistanceOnEarth({0, 0}, {0.003, 0}) * 2;
  polyline.UpdateProjectionByPrediction(
    MercatorBounds::RectByCenterXYAndSizeInMeters({0.002, 0.003}, 20000), dist);
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 1, ());
  TEST_LESS_OR_EQUAL(MercatorBounds::DistanceOnEarth(polyline.GetCurrentIter().m_pt,
                                                     m2::PointD(0.003, 0.003)), 0.1, ());
}

UNIT_TEST(FollowedPolylineDistanceCalculationTest)
{
  // Test full length case.
  FollowedPolyline polyline(kTestDirectedPolyline.Begin(), kTestDirectedPolyline.End());
  double distance = polyline.GetDistanceM(polyline.Begin(), polyline.End());
  double masterDistance = MercatorBounds::DistanceOnEarth(kTestDirectedPolyline.Front(),
                                                          kTestDirectedPolyline.Back());
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());
  distance = polyline.GetTotalDistanceM();
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());

  // Test partial length case.
  polyline.UpdateProjection(MercatorBounds::RectByCenterXYAndSizeInMeters({3, 0}, 2));
  distance = polyline.GetDistanceM(polyline.GetCurrentIter(), polyline.End());
  masterDistance = MercatorBounds::DistanceOnEarth(kTestDirectedPolyline.GetPoint(1),
                                                   kTestDirectedPolyline.Back());
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());
  distance = polyline.GetDistanceToEndM();
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());

  // Test point in the middle case.
  polyline.UpdateProjection(MercatorBounds::RectByCenterXYAndSizeInMeters({4, 0}, 2));
  distance = polyline.GetDistanceM(polyline.GetCurrentIter(), polyline.End());
  masterDistance = MercatorBounds::DistanceOnEarth(m2::PointD(4, 0),
                                                   kTestDirectedPolyline.Back());
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());
  distance = polyline.GetDistanceToEndM();
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());
}

UNIT_TEST(FollowedPolylineDirectionTest)
{
  m2::PolylineD testPolyline({{0, 0}, {1.00003, 0}, {1.00003, 1}});
  FollowedPolyline polyline(testPolyline.Begin(), testPolyline.End());
  TEST_EQUAL(polyline.GetCurrentIter().m_ind, 0, ());
  m2::PointD directionPoint;
  polyline.GetCurrentDirectionPoint(directionPoint, 20);
  TEST_EQUAL(directionPoint, testPolyline.GetPoint(1), ());
  polyline.UpdateProjection(MercatorBounds::RectByCenterXYAndSizeInMeters({1.0, 0}, 2));
  polyline.GetCurrentDirectionPoint(directionPoint, 0.0001);
  TEST_EQUAL(directionPoint, testPolyline.GetPoint(1), ());
  polyline.GetCurrentDirectionPoint(directionPoint, 20);
  TEST_EQUAL(directionPoint, testPolyline.GetPoint(2), ());
}

UNIT_TEST(FollowedPolylineGetDistanceFromBeginM)
{
  m2::PolylineD testPolyline({{0, 0}, {1, 0}, {2, 0}, {3, 0}, {5, 0}, {6, 0}});
  FollowedPolyline polyline(testPolyline.Begin(), testPolyline.End());
  m2::PointD point(4, 0);
  polyline.UpdateProjection(MercatorBounds::RectByCenterXYAndSizeInMeters(point, 2));
  double distance = polyline.GetDistanceFromBeginM();
  double masterDistance = MercatorBounds::DistanceOnEarth(kTestDirectedPolyline.Front(),
                                                          point);
  TEST_ALMOST_EQUAL_ULPS(distance, masterDistance, ());
}
}  // namespace routing_test
