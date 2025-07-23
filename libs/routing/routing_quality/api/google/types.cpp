#include "routing/routing_quality/api/google/types.hpp"

#include <iomanip>
#include <sstream>
#include <tuple>

namespace routing_quality
{
namespace api
{
namespace google
{
bool LatLon::operator==(LatLon const & rhs) const
{
  return std::tie(m_lat, m_lon) == std::tie(rhs.m_lat, rhs.m_lon);
}

bool Step::operator==(Step const & rhs) const
{
  return std::tie(m_startLocation, m_endLocation) == std::tie(rhs.m_startLocation, rhs.m_endLocation);
}

std::string DebugPrint(LatLon const & latlon)
{
  std::stringstream ss;
  ss << std::setprecision(20);
  ss << "google::LatLon(" << latlon.m_lat << ", " << latlon.m_lon << ")";
  return ss.str();
}
}  // namespace google
}  // namespace api
}  // namespace routing_quality
