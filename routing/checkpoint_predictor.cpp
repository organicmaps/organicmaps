#include "routing/checkpoint_predictor.hpp"

#include "geometry/mercator.hpp"

#include "base/stl_helpers.hpp"

#include <limits>

namespace routing
{
using namespace std;

// static
double CheckpointPredictor::CalculateDeltaMeters(m2::PointD const & from, m2::PointD const & to,
                                                 m2::PointD const & between)
{
  double const directDist = mercator::DistanceOnEarth(from, to);
  double const distThroughPoint = mercator::DistanceOnEarth(from, between) + mercator::DistanceOnEarth(between, to);
  return distThroughPoint - directDist;
}

size_t CheckpointPredictor::PredictPosition(vector<m2::PointD> const & points, m2::PointD const & point) const
{
  double constexpr kInvalidDistance = numeric_limits<double>::max();
  double minDeltaMeters = kInvalidDistance;
  size_t minDeltaIdx = 0;
  // Checkpoints include start, all the intermediate points and finish.
  size_t const checkpointNum = points.size() + 2 /* for start and finish points */;
  for (size_t i = 0; i + 1 != checkpointNum; ++i)
  {
    double const delta = CalculateDeltaMeters(GetCheckpoint(points, i), GetCheckpoint(points, i + 1), point);
    if (minDeltaMeters > delta)
    {
      minDeltaMeters = delta;
      minDeltaIdx = i;
    }
  }

  CHECK_NOT_EQUAL(minDeltaMeters, kInvalidDistance, ());
  return minDeltaIdx;
}

m2::PointD const & CheckpointPredictor::GetCheckpoint(vector<m2::PointD> const & points, size_t index) const
{
  if (index == 0)
    return m_start;
  if (index <= points.size())
    return points[index - 1];
  CHECK_EQUAL(index, points.size() + 1, ());
  return m_finish;
}
}  // namespace routing
