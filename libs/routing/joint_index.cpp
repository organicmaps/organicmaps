#include "routing/joint_index.hpp"

#include "routing/routing_exceptions.hpp"

namespace routing
{
void JointIndex::Build(RoadIndex const & roadIndex, uint32_t numJoints)
{
  // + 1 is protection for 'End' method from out of bounds.
  // Call End(numJoints-1) requires more size, so add one more item.
  // Therefore m_offsets.size() == numJoints + 1,
  // And m_offsets.back() == m_points.size()
  m_offsets.assign(numJoints + 1, 0);

  // Calculate sizes.
  // Example for numJoints = 6:
  // 2, 5, 3, 4, 2, 3, 0
  roadIndex.ForEachRoad([this, numJoints](uint32_t /* featureId */, RoadJointIds const & road)
  {
    road.ForEachJoint([this, numJoints](uint32_t /* pointId */, Joint::Id jointId)
    {
      UNUSED_VALUE(numJoints);
      ASSERT_LESS(jointId, numJoints, ());
      ++m_offsets[jointId];
    });
  });

  // Fill offsets with end bounds.
  // Example: 2, 7, 10, 14, 16, 19, 19
  for (size_t i = 1; i < m_offsets.size(); ++i)
    m_offsets[i] += m_offsets[i - 1];

  m_points.resize(m_offsets.back());

  // Now fill points.
  // Offsets after this operation are begin bounds:
  // 0, 2, 7, 10, 14, 16, 19
  roadIndex.ForEachRoad([this](uint32_t featureId, RoadJointIds const & road)
  {
    road.ForEachJoint([this, featureId](uint32_t pointId, Joint::Id jointId)
    {
      uint32_t & offset = m_offsets[jointId];
      --offset;
      m_points[offset] = {featureId, pointId};
    });
  });

  CHECK_EQUAL(m_offsets[0], 0, ());
  CHECK_EQUAL(m_offsets.back(), m_points.size(), ());
}
}  // namespace routing
