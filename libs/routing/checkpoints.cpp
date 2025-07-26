#include "routing/checkpoints.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <iomanip>
#include <sstream>

namespace routing
{
Checkpoints::Checkpoints(std::vector<m2::PointD> && points) : m_points(std::move(points))
{
  CHECK_GREATER_OR_EQUAL(m_points.size(), 2, ("Checkpoints should at least contain start and finish"));
}

void Checkpoints::SetPointFrom(m2::PointD const & point)
{
  ASSERT_LESS(m_passedIdx, m_points.size(), ());
  m_points[m_passedIdx] = point;
}

m2::PointD const & Checkpoints::GetPoint(size_t pointIdx) const
{
  ASSERT_LESS(pointIdx, m_points.size(), ());
  return m_points[pointIdx];
}

m2::PointD const & Checkpoints::GetPointFrom() const
{
  CHECK(!IsFinished(), ());
  return GetPoint(m_passedIdx);
}

m2::PointD const & Checkpoints::GetPointTo() const
{
  CHECK(!IsFinished(), ());
  return GetPoint(m_passedIdx + 1);
}

void Checkpoints::PassNextPoint()
{
  CHECK(!IsFinished(), ());
  ++m_passedIdx;
}
double Checkpoints::GetSummaryLengthBetweenPointsMeters() const
{
  double dist = 0.0;
  for (size_t i = 1; i < m_points.size(); ++i)
    dist += mercator::DistanceOnEarth(m_points[i - 1], m_points[i]);

  return dist;
}

std::string DebugPrint(Checkpoints const & checkpoints)
{
  std::ostringstream out;
  out << "Checkpoints(";
  for (auto const & pt : checkpoints.GetPoints())
    out << DebugPrint(mercator::ToLatLon(pt)) << "; ";
  out << "passed: " << checkpoints.GetPassedIdx() << ")";
  return out.str();
}
}  // namespace routing
