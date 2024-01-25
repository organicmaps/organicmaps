#include "base/string_utils.hpp"
#include "kml/serdes_common.hpp"
#include "geometry/mercator.hpp"
#include <sstream>

namespace kml
{

std::string PointToString(m2::PointD const & org)
{
  double const lon = mercator::XToLon(org.x);
  double const lat = mercator::YToLat(org.y);

  std::ostringstream ss;
  ss.precision(8);

  ss << lon << "," << lat;
  return ss.str();
}

std::string PointToString(geometry::PointWithAltitude const & pt)
{
  if (pt.GetAltitude() != geometry::kInvalidAltitude)
    return PointToString(pt.GetPoint()) + "," + strings::to_string(pt.GetAltitude());
  return PointToString(pt.GetPoint());
}

}  // namespace kml