#include "road_graph.hpp"

#include "../std/sstream.hpp"


namespace routing
{

string DebugPrint(RoadPos const & r)
{
  ostringstream ss;
  ss << "{" << r.GetFeatureId() << ", " << r.IsForward() << ", " << r.m_pointId << "}";
  return ss.str();
}

} // namespace routing
