#include "routing/road_access.hpp"

#include <sstream>

namespace routing
{
std::string DebugPrint(RoadAccess const & r)
{
  size_t const kMaxIdsToShow = 10;
  std::ostringstream oss;
  oss << "RoadAccess: Private roads [";
  auto const & privateRoads = r.GetPrivateRoads();
  for (size_t i = 0; i < privateRoads.size(); ++i)
  {
    if (i == kMaxIdsToShow)
    {
      oss << "...";
      break;
    }
    if (i > 0)
      oss << " ";
    oss << privateRoads[i];
  }
  oss << "]";
  return oss.str();
}
}  // namespace routing
