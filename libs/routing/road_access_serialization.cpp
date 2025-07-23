#include "routing/road_access_serialization.hpp"

#include <string>

namespace routing
{
std::string DebugPrint(RoadAccessSerializer::Header const & header)
{
  switch (header)
  {
  case RoadAccessSerializer::Header::TheFirstVersionRoadAccess: return "TheFirstVersionRoadAccess";
  case RoadAccessSerializer::Header::WithoutAccessConditional: return "WithoutAccessConditional";
  case RoadAccessSerializer::Header::WithAccessConditional: return "WithAccessConditional";
  }
  UNREACHABLE();
}
}  // namespace routing
