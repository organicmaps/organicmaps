#include "geometry/point_with_altitude.hpp"

#include <sstream>

#include "3party/boost/boost/container_hash/hash.hpp"

namespace geometry
{
PointWithAltitude::PointWithAltitude()
  : m_point(m2::PointD::Zero()), m_altitude(kDefaultAltitudeMeters)
{
}

PointWithAltitude::PointWithAltitude(m2::PointD const & point, Altitude altitude)
  : m_point(point), m_altitude(altitude)
{
}

PointWithAltitude::PointWithAltitude(m2::PointD && point, Altitude altitude)
  : m_point(std::move(point)), m_altitude(altitude)
{
}

bool PointWithAltitude::operator==(PointWithAltitude const & r) const
{
  return m_point == r.m_point && m_altitude == r.m_altitude;
}

bool PointWithAltitude::operator<(PointWithAltitude const & r) const
{
  if (m_point != r.m_point)
    return m_point < r.m_point;

  return m_altitude < r.m_altitude;
}

std::string DebugPrint(PointWithAltitude const & r)
{
  std::ostringstream ss;
  ss << "PointWithAltitude{point:" << DebugPrint(r.m_point) << ", altitude:" << r.GetAltitude()
     << "}";
  return ss.str();
}

PointWithAltitude MakePointWithAltitudeForTesting(m2::PointD const & point)
{
  return PointWithAltitude(point, kDefaultAltitudeMeters);
}

bool AlmostEqualAbs(PointWithAltitude const & lhs, PointWithAltitude const & rhs, double eps)
{
  return lhs.GetPoint().EqualDxDy(rhs.GetPoint(), eps) && lhs.GetAltitude() == rhs.GetAltitude();
}
}  // namespace geometry

namespace std
{
size_t hash<geometry::PointWithAltitude>::operator()(geometry::PointWithAltitude const & point) const
{
  size_t seed = 0;
  boost::hash_combine(seed, point.GetPoint().x);
  boost::hash_combine(seed, point.GetPoint().y);
  boost::hash_combine(seed, point.GetAltitude());
  return seed;
}
}  // namespace std
