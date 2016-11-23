#include "routing/joint_index.hpp"

namespace routing
{
Joint::Id JointIndex::InsertJoint(RoadPoint const & rp)
{
  Joint::Id const jointId = GetNumJoints();
  m_points.emplace_back(rp);
  m_offsets.emplace_back(m_points.size());
  return jointId;
}

void JointIndex::AppendToJoint(Joint::Id jointId, RoadPoint rp)
{
  m_dynamicJoints[jointId].AddPoint(rp);
}

void JointIndex::Build(RoadIndex const & roadIndex, uint32_t numJoints)
{
  // + 1 is protection for 'End' method from out of bounds.
  // Call End(numJoints-1) requires more size, so add one more item.
  // Therefore m_offsets.size() == numJoints + 1,
  // And m_offsets.back() == m_points.size()
  m_offsets.assign(numJoints + 1, 0);

  // Calculate shifted sizes.
  // Example for numJoints = 6:
  // Original sizes: 2, 5, 3, 4, 2, 3, 0
  // Shifted sizes: 0, 2, 5, 3, 4, 2, 3
  roadIndex.ForEachRoad([this](uint32_t /* featureId */, RoadJointIds const & road) {
    road.ForEachJoint([this](uint32_t /* pointId */, Joint::Id jointId) {
      Joint::Id nextId = jointId + 1;
      ASSERT_LESS(jointId, m_offsets.size(), ());
      ++m_offsets[nextId];
    });
  });

  // Calculate twice shifted offsets.
  // Example: 0, 0, 2, 7, 10, 14, 16
  uint32_t sum = 0;
  uint32_t prevSum = 0;
  for (size_t i = 1; i < m_offsets.size(); ++i)
  {
    sum += m_offsets[i];
    m_offsets[i] = prevSum;
    prevSum = sum;
  }

  m_points.resize(sum);

  // Now fill points, m_offsets[nextId] is current incrementing begin.
  // Offsets after this operation: 0, 2, 7, 10, 14, 16, 19
  roadIndex.ForEachRoad([this](uint32_t featureId, RoadJointIds const & road) {
    road.ForEachJoint([this, featureId](uint32_t pointId, Joint::Id jointId) {
      Joint::Id nextId = jointId + 1;
      ASSERT_LESS(nextId, m_offsets.size(), ());
      uint32_t & offset = m_offsets[nextId];
      m_points[offset] = {featureId, pointId};
      ++offset;
    });
  });

  if (m_offsets[0] != 0)
    MYTHROW(RootException, ("Wrong offsets calculation: m_offsets[0] =", m_offsets[0]));

  if (m_offsets.back() != m_points.size())
    MYTHROW(RootException, ("Wrong offsets calculation: m_offsets.back() =", m_offsets.back(),
                            ", m_points.size()=", m_points.size()));
}
}  // namespace routing
