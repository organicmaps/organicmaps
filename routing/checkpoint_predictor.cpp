#include "routing/checkpoint_predictor.hpp"

#include "geometry/mercator.hpp"

#include "base/stl_helpers.hpp"

#include <limits>

namespace routing
{
using namespace std;

vector<int8_t> CheckpointPredictor::operator()(vector<m2::PointD> const & intermediatePoints)
{
  if (intermediatePoints.empty())
    return {};
  if (intermediatePoints.size() == 1)
    return {0};

  // Preparing |checkPoints|. It's all route points (including start and finish) except for the point
  // which is adding.
  m2::PointD const & addingPoint = intermediatePoints[0];
  vector<m2::PointD> checkPoints(intermediatePoints.cbegin() + 1, intermediatePoints.cend());
  checkPoints.insert(checkPoints.begin(), m_start);
  checkPoints.push_back(m_finish);
  CHECK_EQUAL(checkPoints.size(), intermediatePoints.size() + 1, ());

  // Looking for the best place for |addingPoint| between points at |checkPoints|.
  double const kInvalidDistance = numeric_limits<double>::max();
  double minDistMeters = kInvalidDistance;
  // |minDistIdx| is a zero based index of a section between two points at |checkPoints|
  // which should be split to minimize the result of |DistanceBetweenPointsMeters(checkPointsWithAdding)|.
  size_t minDistIdx = 0;
  for (size_t i = 1; i < checkPoints.size(); ++i)
  {
    vector<m2::PointD> checkPointsWithAdding(checkPoints.cbegin(), checkPoints.cend());
    checkPointsWithAdding.insert(checkPointsWithAdding.begin() + i, addingPoint);
    double const distBetweenPointsMeters = DistanceBetweenPointsMeters(checkPointsWithAdding);
    if (distBetweenPointsMeters < minDistMeters)
    {
      minDistMeters = distBetweenPointsMeters;
      minDistIdx = i - 1;
    }
  }
  CHECK_NOT_EQUAL(minDistMeters, kInvalidDistance, ());

  // Preparing |order|.
  vector<int8_t> order;
  order.reserve(intermediatePoints.size());
  for (size_t i = 0; i < intermediatePoints.size() - 1; ++i)
    order.push_back(i);

  // |minDistIdx| is a place for |addingPoint|. Increasing intermediate point indices which is greater
  // or equal to |minDistIdx| to empty place for |addingPoint|.
  // Note. |addingPoint| is places at the beginning of |intermediatePoints|.
  CHECK_LESS_OR_EQUAL(minDistIdx, order.size(), ());
  for (size_t i = minDistIdx; i < order.size(); ++i)
    ++order[i];
  order.insert(order.begin(), static_cast<int8_t>(minDistIdx));
  CHECK_EQUAL(order.size(), intermediatePoints.size(), ());

#ifdef DEBUG
  vector<int8_t> orderToCheck(order.cbegin(), order.cend());
  my::SortUnique(orderToCheck);
  ASSERT_EQUAL(orderToCheck.size(), intermediatePoints.size(), ());
  ASSERT_EQUAL(orderToCheck.back() + 1, orderToCheck.size(), ());
#endif
  return order;
}

double DistanceBetweenPointsMeters(vector<m2::PointD> const & points)
{
  if (points.size() <= 1)
    return 0.0;

  double length = 0.0;
  for (size_t i = 1; i < points.size(); ++i)
    length += MercatorBounds::DistanceOnEarth(points[i - 1], points[i]);

  return length;
}
}  // namespace routing
