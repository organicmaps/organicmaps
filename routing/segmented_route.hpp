#pragma once

#include "routing/segment.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace routing
{
class SegmentedRouteStep final
{
public:
  SegmentedRouteStep() = default;

  SegmentedRouteStep(Segment const & segment, m2::PointD const & point)
    : m_segment(segment), m_point(point)
  {
  }

  Segment const & GetSegment() const { return m_segment; }
  m2::PointD const & GetPoint() const { return m_point; }

private:
  Segment m_segment;
  // The front point of segment
  m2::PointD m_point = m2::PointD::Zero();
};

class SegmentedRoute final
{
public:
  void Init(m2::PointD const & start, m2::PointD const & finish);

  void AddStep(Segment const & segment, m2::PointD const & point)
  {
    m_steps.emplace_back(segment, point);
  }

  void Clear() { Init(m2::PointD::Zero(), m2::PointD::Zero()); }

  double CalcDistance(m2::PointD const & point) const;
  Segment const & GetFinishSegment() const;

  m2::PointD const & GetStart() const { return m_start; }
  m2::PointD const & GetFinish() const { return m_finish; }
  std::vector<SegmentedRouteStep> const & GetSteps() const { return m_steps; }
  bool IsEmpty() const { return m_steps.empty(); }

private:
  m2::PointD m_start = m2::PointD::Zero();
  m2::PointD m_finish = m2::PointD::Zero();
  std::vector<SegmentedRouteStep> m_steps;
};
}  // namespace routing
