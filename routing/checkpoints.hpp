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

  Checkpoints(std::vector<m2::PointD> && points) : m_points(std::move(points)) { CheckValid(); }

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

  void CheckValid() const
  {
    CHECK_GREATER_OR_EQUAL(m_points.size(), 2, ("Checkpoints should contain start and finish"));
  }

private:
  std::vector<m2::PointD> m_points;
};
}  // namespace routing
