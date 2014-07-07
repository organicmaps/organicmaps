#include "road_graph.hpp"

#include "../std/sstream.hpp"


namespace routing
{

string DebugPrint(RoadPos const & r)
{
  ostringstream ss;
  ss << "{" << r.GetFeatureId() << ", " << r.m_pointId << ", " << r.IsForward() << "}";
  return ss.str();
}

} // namespace routing
