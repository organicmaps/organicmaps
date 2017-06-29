#include "routing/checkpoints.hpp"

#include "geometry/mercator.hpp"

#include <iomanip>
#include <sstream>

namespace routing
{
std::string DebugPrint(Checkpoints const & checkpoints)
{
  std::ostringstream out;
  out << std::fixed << std::setprecision(6);
  out << "Checkpoints(";
  for (auto const & point : checkpoints.GetPoints())
  {
    auto const latlon = MercatorBounds::ToLatLon(point);
    out << latlon.lat << " " << latlon.lon << ", ";
  }
  out << "passed: " << checkpoints.GetPassedIdx() << ")";
  return out.str();
}
}  // namespace routing
