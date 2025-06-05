#include "testing/testing.hpp"

#include "routing/base/followed_polyline.hpp"

#include "geometry/polyline2d.hpp"

namespace routing_test
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
}  // namespace routing_test
