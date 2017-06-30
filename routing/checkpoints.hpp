#pragma once

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <string>
#include <utility>
#include <vector>

namespace routing
{
class Checkpoints final
{
public:
  Checkpoints() = default;

  Checkpoints(m2::PointD const & start, m2::PointD const & finish) : m_points({start, finish}) {}

  Checkpoints(std::vector<m2::PointD> && points) : m_points(std::move(points))
  {
    CHECK_GREATER_OR_EQUAL(m_points.size(), 2,
                           ("Checkpoints should at least contain start and finish"));
  }

  m2::PointD const & GetStart() const { return m_points.front(); }
  m2::PointD const & GetFinish() const { return m_points.back(); }
  std::vector<m2::PointD> const & GetPoints() const { return m_points; }
  size_t GetPassedIdx() const { return m_passedIdx; }

  void SetPointFrom(m2::PointD const & point)
  {
    CHECK_LESS(m_passedIdx, m_points.size(), ());
    m_points[m_passedIdx] = point;
  }

  m2::PointD const & GetPoint(size_t pointIdx) const
  {
    CHECK_LESS(pointIdx, m_points.size(), ());
    return m_points[pointIdx];
  }

  m2::PointD const & GetPointFrom() const { return GetPoint(m_passedIdx); }
  m2::PointD const & GetPointTo() const { return GetPoint(m_passedIdx + 1); }

  size_t GetNumSubroutes() const { return m_points.size() - 1; }
  size_t GetNumRemainingSubroutes() const { return GetNumSubroutes() - m_passedIdx; }

  bool IsFinished() const { return m_passedIdx >= GetNumSubroutes(); }

  void PassNextPoint()
  {
    CHECK(!IsFinished(), ());
    ++m_passedIdx;
  }

private:
  // m_points contains start, finish and intermediate points.
  std::vector<m2::PointD> m_points = {m2::PointD::Zero(), m2::PointD::Zero()};
  // m_passedIdx is the index of the checkpoint by which the user passed by.
  // By default, user has passed 0, it is start point.
  size_t m_passedIdx = 0;
};

std::string DebugPrint(Checkpoints const & checkpoints);
}  // namespace routing
