#include "road_graph.hpp"

#include "../base/assert.hpp"

#include "../std/sstream.hpp"


namespace routing
{

RoadPos::RoadPos(uint32_t featureId, bool bForward, size_t pointId)
  : m_featureId((featureId << 1) + (bForward ? 1 : 0)), m_pointId(pointId)
{
  ASSERT_LESS(featureId, 1U << 31, ());
  ASSERT_LESS(pointId, 0xFFFFFFFF, ());
}

string DebugPrint(RoadPos const & r)
{
  ostringstream ss;
  ss << "{" << r.GetFeatureId() << ", " << r.IsForward() << ", " << r.m_pointId << "}";
  return ss.str();
}

} // namespace routing
