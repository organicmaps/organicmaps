#pragma once

#include "geometry/point2d.hpp"

#include <cstdint>
#include <iterator>
#include <vector>

namespace routing
{
/// \brief This class is responsable for prediction of the order of intermediate route points.
/// According to current implementation to find the best place for an adding intermediate points
/// this class inserts an added intermediate point between points at vector
/// {<route start>, <former intermediate points>, <route finish>}, calculates the length
/// of the broken line in meters, and finds the sequence of points which gets the shortest length.
class CheckpointPredictor final
{
  friend class CheckpointPredictorTest;

public:
  CheckpointPredictor(m2::PointD const & start, m2::PointD const & finish)
    : m_start(start), m_finish(finish)
  {
  }

  /// \brief finds the best position for |point| between |intermediatePoints| to minimize the length
  /// of broken line of |intermediatePoints| with inserted |point| between them.
  /// \param intermediatePoints is a sequence of points on the map.
  /// \param point is a point to be inserted between |points|.
  /// \returns zero based index of a line segment between two points at |points|.
  size_t PredictPosition(std::vector<m2::PointD> const & intermediatePoints, m2::PointD const & point) const;

private:
  /// \returns checkpoint by its |index|.
  /// \note checkpoints is a sequence of points: |m_start|, |intermediatePoints|, |m_finish|.
  m2::PointD const & GetCheckpoint(std::vector<m2::PointD> const & intermediatePoints, size_t index) const;

  m2::PointD const m_start;
  m2::PointD const m_finish;
};

/// \returns difference between distance |from|->|between| + |between|->|to| and distance |from|->|to|.
double CalculateDeltaMeters(m2::PointD const & from, m2::PointD const & to, m2::PointD const & between);
}  // namespace routing
