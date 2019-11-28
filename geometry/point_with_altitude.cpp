#include "geometry/point_with_altitude.hpp"

#include <sstream>

namespace geometry
{
PointWithAltitude::PointWithAltitude()
  : m_point(m2::PointD::Zero()), m_altitude(kDefaultAltitudeMeters)
{
}

PointWithAltitude::PointWithAltitude(m2::PointD const & point, TAltitude altitude)
  : m_point(point), m_altitude(altitude)
{
}

std::string DebugPrint(PointWithAltitude const & r)
{
  std::ostringstream ss;
  ss << "PointWithAltitude{point:" << DebugPrint(r.m_point) << ", altitude:" << r.GetAltitude()
     << "}";
  return ss.str();
}
}  // namespace geometry
