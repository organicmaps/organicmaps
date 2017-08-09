#include "routing/checkpoint_predictor.hpp"

#include "geometry/mercator.hpp"

#include "base/stl_helpers.hpp"

#include <limits>

namespace routing
{
using namespace std;

size_t CheckpointPredictor::PredictPosition(vector<m2::PointD> const & intermediatePoints,
                                            m2::PointD const & point) const
{
  double constexpr kInvalidDistance = numeric_limits<double>::max();
  double minDeltaMeters = kInvalidDistance;
  size_t minDeltaIdx = 0;
  // Checkpoints include start, all the intermediate points and finish.
  size_t const checkpointNum = intermediatePoints.size() + 2 /* for start and finish points */;
  for (size_t i = 0; i + 1 != checkpointNum; ++i)
  {
    double const delta = CalculateDeltaMeters(GetCheckpoint(intermediatePoints, i),
                                              GetCheckpoint(intermediatePoints, i + 1), point);
    if (minDeltaMeters > delta)
    {
      minDeltaMeters = delta;
      minDeltaIdx = i;
    }
  }

  CHECK_NOT_EQUAL(minDeltaMeters, kInvalidDistance, ());
  return minDeltaIdx;
}

m2::PointD const & CheckpointPredictor::GetCheckpoint(vector<m2::PointD> const & intermediatePoints,
                                                      size_t index) const
{
  if (index == 0)
    return m_start;
  if (index <= intermediatePoints.size())
    return intermediatePoints[index - 1];
  CHECK_EQUAL(index, intermediatePoints.size() + 1, ());
  return m_finish;
}

double CalculateDeltaMeters(m2::PointD const & from, m2::PointD const & to,
                            m2::PointD const & between)
{
  double const directDist = MercatorBounds::DistanceOnEarth(from, to);
  double const distThroughPoint = MercatorBounds::DistanceOnEarth(from, between) +
                                  MercatorBounds::DistanceOnEarth(between, to);
  return distThroughPoint - directDist;
}
}  // namespace routing
