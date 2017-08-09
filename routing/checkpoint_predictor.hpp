#pragma once

#include "geometry/point2d.hpp"

#include <cstdint>
#include <vector>

namespace routing
{
/// \brief This class is responsable for prediction of the order of intermediate route points.
/// According to current implementation to find the best place for an adding intermediate points
/// this class inserts an adding intermediate point between points at vector
/// {<route start>, <former intermediate points>, <route finish>}, calculates the length
/// of the broken line in meters, and finds the sequence of points which gets the shortest length.
class CheckpointPredictor
{
public:
  CheckpointPredictor(m2::PointD const & start, m2::PointD const & finish)
    : m_start(start), m_finish(finish)
  {
  }

  /// \brief Calculates the order of intermediate route points.
  /// \pamam intermediatePoints intermediate route points. According to current implementation
  /// it's assumed the |intermediatePoints[0]| is an added intermediate point and the rest items
  /// at |intermediatePoints| are former route intermediate points which should keep their
  /// relative order.
  /// \returns zero based indices of route intermediate points after placing adding
  /// intermediate point (|intermediatePoints[0]|). Please see CheckpointPredictorTest
  /// for examples.
  std::vector<int8_t> operator()(std::vector<m2::PointD> const & intermediatePoints);

private:
  m2::PointD const m_start;
  m2::PointD const m_finish;
};

/// \returns length of broken line |points| in meters.
double DistanceBetweenPointsMeters(std::vector<m2::PointD> const & points);
}  // namespace routing
