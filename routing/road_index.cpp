#include "routing/road_index.hpp"

#include "routing/routing_exceptions.hpp"

namespace routing
{
void RoadIndex::Import(vector<Joint> const & joints)
{
  for (Joint::Id jointId = 0; jointId < joints.size(); ++jointId)
  {
    Joint const & joint = joints[jointId];
    for (uint32_t i = 0; i < joint.GetSize(); ++i)
    {
      RoadPoint const & entry = joint.GetEntry(i);
      RoadJointIds & roadJoints = m_roads[entry.GetFeatureId()];
      roadJoints.AddJoint(entry.GetPointId(), jointId);
    }
  }
}

pair<Joint::Id, uint32_t> RoadIndex::FindNeighbor(RoadPoint const & rp, bool forward) const
{
  auto const it = m_roads.find(rp.GetFeatureId());
  if (it == m_roads.cend())
    MYTHROW(RoutingException, ("RoadIndex doesn't contains feature", rp.GetFeatureId()));

  return it->second.FindNeighbor(rp.GetPointId(), forward);
}
}  // namespace routing
