#pragma once

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <utility>
#include <vector>

namespace routing
{
class Checkpoints final
{
public:
  Checkpoints() = default;

  Checkpoints(m2::PointD const & start, m2::PointD const & finish) : m_points({start, finish}) {}

  Checkpoints(size_t arriveIdx, std::vector<m2::PointD> && points)
    : m_points(std::move(points)), m_arriveIdx(arriveIdx)
  {
    CheckValid();
  }

  m2::PointD const & GetStart() const
  {
    CheckValid();
    return m_points.front();
  }

  m2::PointD const & GetFinish() const
  {
    CheckValid();
    return m_points.back();
  }

  void SetStart(m2::PointD const & start)
  {
    CheckValid();
    m_points.front() = start;
  }

  std::vector<m2::PointD> const & GetPoints() const
  {
    CheckValid();
    return m_points;
  }

  size_t GetArriveIdx() const { return m_arriveIdx; }

  void ArriveNextPoint()
  {
    ++m_arriveIdx;
    CheckValid();
  }

  void CheckValid() const
  {
    CHECK_GREATER_OR_EQUAL(m_points.size(), 2,
                           ("Checkpoints should at least contain start and finish"));
    CHECK_LESS(m_arriveIdx, m_points.size(), ());
  }

private:
  // m_points contains start, finish and intermediate points.
  std::vector<m2::PointD> m_points;
  // Arrive idx is the index of the checkpoint by which the user passed by.
  // By default, user has arrived at 0, it is start point.
  size_t m_arriveIdx = 0;
};
}  // namespace routing
