#include "fseg_index.hpp"

#include "std/utility.hpp"

namespace routing
{
void FSegIndex::Export(vector<Joint> const & joints)
{
  for (JointId jointId = 0; jointId < joints.size(); ++jointId)
  {
    Joint const & joint = joints[jointId];
    for (uint32_t i = 0; i < joint.GetSize(); ++i)
    {
      FSegId const & entry = joint.GetEntry(i);
      RoadJointIds & roadJoints = m_roads[entry.GetFeatureId()];
      roadJoints.AddJoint(entry.GetSegId(), jointId);
    }
  }
}

pair<JointId, uint32_t> FSegIndex::FindNeigbor(FSegId fseg, bool forward) const
{
  auto const it = m_roads.find(fseg.GetFeatureId());
  if (it == m_roads.cend())
    return make_pair(kInvalidJointId, 0);

  RoadJointIds const & joints = it->second;
  int32_t const step = forward ? 1 : -1;

  for (uint32_t segId = fseg.GetSegId() + step; segId < joints.GetSize(); segId += step)
  {
    JointId const jointId = joints.GetJointId(segId);
    if (jointId != kInvalidJointId)
      return make_pair(jointId, segId);
  }

  return make_pair(kInvalidJointId, 0);
}
}  // namespace routing
