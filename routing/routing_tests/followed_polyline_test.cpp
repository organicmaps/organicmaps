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
                                                     m2::PointD(0.003, 0.003)),
                     0.1, ());

}
}  // namespace routing_test
