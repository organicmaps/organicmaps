#pragma once

#include "geometry/point2d.hpp"

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
  explicit Checkpoints(std::vector<m2::PointD> && points);

  m2::PointD const & GetStart() const { return m_points.front(); }
  m2::PointD const & GetFinish() const { return m_points.back(); }
  std::vector<m2::PointD> const & GetPoints() const { return m_points; }
  size_t GetPassedIdx() const { return m_passedIdx; }

  void SetPointFrom(m2::PointD const & point);
  m2::PointD const & GetPoint(size_t pointIdx) const;
  m2::PointD const & GetPointFrom() const;
  m2::PointD const & GetPointTo() const;

  size_t GetNumSubroutes() const { return m_points.size() - 1; }
  bool IsFinished() const { return m_passedIdx >= GetNumSubroutes(); }

  double GetSummaryLengthBetweenPointsMeters() const;

  void PassNextPoint();

private:
  // m_points contains start, finish and intermediate points.
  std::vector<m2::PointD> m_points = {m2::PointD::Zero(), m2::PointD::Zero()};
  // m_passedIdx is the index of the checkpoint by which the user passed by.
  // By default, user has passed 0, it is start point.
  size_t m_passedIdx = 0;
};

std::string DebugPrint(Checkpoints const & checkpoints);
}  // namespace routing
