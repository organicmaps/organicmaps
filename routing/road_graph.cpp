#include "road_graph.hpp"

#include "../base/assert.hpp"

#include "../std/limits.hpp"
#include "../std/sstream.hpp"

namespace routing
{
RoadPos::RoadPos(uint32_t featureId, bool forward, size_t segmentId, m2::PointD const & p)
    : m_featureId((featureId << 1) + (forward ? 1 : 0)), m_segmentId(segmentId), m_segEndpoint(p)
{
  ASSERT_LESS(featureId, 1U << 31, ());
  ASSERT_LESS(segmentId, numeric_limits<uint32_t>::max(), ());
}

string DebugPrint(RoadPos const & r)
{
  ostringstream ss;
  ss << "{ featureId: " << r.GetFeatureId() << ", isForward: " << r.IsForward()
     << ", segmentId:" << r.m_segmentId << "}";
  return ss.str();
}

}  // namespace routing
