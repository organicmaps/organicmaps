#include "routing/checkpoints.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <iomanip>
#include <sstream>

namespace routing
{
Checkpoints::Checkpoints(std::vector<m2::PointD> && points) : m_points(std::move(points))
{
  CHECK_GREATER_OR_EQUAL(m_points.size(), 2,
                         ("Checkpoints should at least contain start and finish"));
}

void Checkpoints::SetPointFrom(m2::PointD const & point)
{
  CHECK_LESS(m_passedIdx, m_points.size(), ());
  m_points[m_passedIdx] = point;
}

m2::PointD const & Checkpoints::GetPoint(size_t pointIdx) const
{
  CHECK_LESS(pointIdx, m_points.size(), ());
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

std::string DebugPrint(Checkpoints const & checkpoints)
{
  std::ostringstream out;
  out << std::fixed << std::setprecision(6);
  out << "Checkpoints(";
  for (auto const & point : checkpoints.GetPoints())
  {
    auto const latlon = MercatorBounds::ToLatLon(point);
    out << latlon.lat << ", " << latlon.lon << "; ";
  }
  out << "passed: " << checkpoints.GetPassedIdx() << ")";
  return out.str();
}
}  // namespace routing
