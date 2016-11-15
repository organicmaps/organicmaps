#pragma once

#include "routing/joint.hpp"
#include "routing/road_index.hpp"
#include "routing/road_point.hpp"

#include "std/vector.hpp"

namespace routing
{
// JointIndex contains mapping from Joint::Id to RoadPoints.
//
// It is vector<Joint> conceptually.
// Technically Joint entries are joined into the single vector to reduce allocations overheads.
class JointIndex final
{
public:
  // Read comments in Build method about -1.
  size_t GetNumJoints() const { return m_offsets.size() - 1; }
  size_t GetNumPoints() const { return m_points.size(); }
  RoadPoint GetPoint(Joint::Id jointId) const { return m_points[Begin(jointId)]; }

  template <typename F>
  void ForEachPoint(Joint::Id jointId, F && f) const
  {
    for (uint32_t i = Begin(jointId); i < End(jointId); ++i)
      f(m_points[i]);

    auto const & it = m_dynamicJoints.find(jointId);
    if (it != m_dynamicJoints.end())
    {
      Joint const & joint = it->second;
      for (size_t i = 0; i < joint.GetSize(); ++i)
        f(joint.GetEntry(i));
    }
  }

  void Build(RoadIndex const & roadIndex, uint32_t numJoints);
  void FindPointsWithCommonFeature(Joint::Id jointId0, Joint::Id jointId1, RoadPoint & result0,
                                   RoadPoint & result1) const;
  Joint::Id InsertJoint(RoadPoint const & rp);
  void AppendToJoint(Joint::Id jointId, RoadPoint rp);

private:
  // Begin index for jointId entries.
  uint32_t Begin(Joint::Id jointId) const
  {
    ASSERT_LESS(jointId, m_offsets.size(), ());
    return m_offsets[jointId];
  }

  // End index (not inclusive) for jointId entries.
  uint32_t End(Joint::Id jointId) const
  {
    Joint::Id const nextId = jointId + 1;
    ASSERT_LESS(nextId, m_offsets.size(), ());
    return m_offsets[nextId];
  }

  vector<uint32_t> m_offsets;
  vector<RoadPoint> m_points;
  unordered_map<Joint::Id, Joint> m_dynamicJoints;
};
}  // namespace routing
