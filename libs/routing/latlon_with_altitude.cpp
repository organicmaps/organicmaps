#include "routing/latlon_with_altitude.hpp"

#include "geometry/mercator.hpp"

#include <sstream>
#include <utility>

namespace routing
{
std::string DebugPrint(LatLonWithAltitude const & latLonWithAltitude)
{
  std::stringstream ss;
  ss << "LatLonWithAltitude(" << DebugPrint(latLonWithAltitude.GetLatLon()) << ", " << latLonWithAltitude.GetAltitude()
     << ")";
  return ss.str();
}

bool LatLonWithAltitude::operator<(LatLonWithAltitude const & rhs) const
{
  return m_latlon < rhs.m_latlon;
}

bool LatLonWithAltitude::operator==(LatLonWithAltitude const & rhs) const
{
  return m_latlon == rhs.m_latlon;
}

geometry::PointWithAltitude LatLonWithAltitude::ToPointWithAltitude() const
{
  return geometry::PointWithAltitude(mercator::FromLatLon(m_latlon), m_altitude);
}
}  // namespace routing
