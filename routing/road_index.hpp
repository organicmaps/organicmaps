#pragma once

#include "routing/joint.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace routing
{
class RoadJointIds final
{
public:
  void Init(uint32_t maxPointId)
  {
    m_jointIds.clear();
    m_jointIds.reserve(maxPointId + 1);
  }

  Joint::Id GetJointId(uint32_t pointId) const
  {
    if (pointId < m_jointIds.size())
      return m_jointIds[pointId];

    return Joint::kInvalidId;
  }

  Joint::Id GetEndingJointId() const
  {
    if (m_jointIds.empty())
      return Joint::kInvalidId;

    ASSERT_NOT_EQUAL(m_jointIds.back(), Joint::kInvalidId, ());
    return m_jointIds.back();
  }

  void AddJoint(uint32_t pointId, Joint::Id jointId)
  {
    ASSERT_NOT_EQUAL(jointId, Joint::kInvalidId, ());

    if (pointId >= m_jointIds.size())
      m_jointIds.insert(m_jointIds.end(), pointId + 1 - m_jointIds.size(), Joint::kInvalidId);

    ASSERT_EQUAL(m_jointIds[pointId], Joint::kInvalidId, ());
    m_jointIds[pointId] = jointId;
  }

  uint32_t GetJointsNumber() const
  {
    uint32_t count = 0;

    for (Joint::Id const jointId : m_jointIds)
      if (jointId != Joint::kInvalidId)
        ++count;

    return count;
  }

  template <typename F>
  void ForEachJoint(F && f) const
  {
    for (uint32_t pointId = 0; pointId < m_jointIds.size(); ++pointId)
    {
      Joint::Id const jointId = m_jointIds[pointId];
      if (jointId != Joint::kInvalidId)
        f(pointId, jointId);
    }
  }

  std::pair<Joint::Id, uint32_t> FindNeighbor(uint32_t pointId, bool forward, uint32_t pointsNumber) const
  {
    CHECK_GREATER_OR_EQUAL(pointsNumber, 2, ("Number of points of road should be greater or equal 2"));

    uint32_t index = 0;
    if (forward)
    {
      for (index = pointId + 1; index < pointsNumber; ++index)
      {
        Joint::Id const jointId = GetJointId(index);
        if (jointId != Joint::kInvalidId)
          return {jointId, index};
      }
    }
    else
    {
      for (index = std::min(pointId, pointsNumber) - 1; index < pointsNumber; --index)
      {
        Joint::Id const jointId = GetJointId(index);
        if (jointId != Joint::kInvalidId)
          return {jointId, index};
      }
    }

    // Return the end or start of road, depends on forward flag.
    index = forward ? pointsNumber - 1 : 0;
    return {Joint::kInvalidId, index};
  }

private:
  // Joint ids indexed by point id.
  // If some point id doesn't match any joint id, this vector contains Joint::kInvalidId.
  std::vector<Joint::Id> m_jointIds;
};

class RoadIndex final
{
public:
  void Import(std::vector<Joint> const & joints);

  void AddJoint(RoadPoint const & rp, Joint::Id jointId)
  {
    m_roads[rp.GetFeatureId()].AddJoint(rp.GetPointId(), jointId);
  }

  bool IsRoad(uint32_t featureId) const { return m_roads.count(featureId) != 0; }

  RoadJointIds const & GetRoad(uint32_t featureId) const
  {
    auto const & it = m_roads.find(featureId);
    CHECK(it != m_roads.cend(), ("Feature id:", featureId));
    return it->second;
  }

  void PushFromSerializer(Joint::Id jointId, RoadPoint const & rp)
  {
    m_roads[rp.GetFeatureId()].AddJoint(rp.GetPointId(), jointId);
  }

  // Find nearest point with normal joint id.
  // If forward == true: neighbor with larger point id (right neighbor)
  // If forward == false: neighbor with smaller point id (left neighbor)
  //
  // If there is no nearest point, return {Joint::kInvalidId, 0}
  std::pair<Joint::Id, uint32_t> FindNeighbor(RoadPoint const & rp, bool forward) const;

  uint32_t GetSize() const { return base::asserted_cast<uint32_t>(m_roads.size()); }

  Joint::Id GetJointId(RoadPoint const & rp) const
  {
    auto const it = m_roads.find(rp.GetFeatureId());
    if (it == m_roads.end())
      return Joint::kInvalidId;

    return it->second.GetJointId(rp.GetPointId());
  }

  template <typename F>
  void ForEachRoad(F && f) const
  {
    for (auto const & it : m_roads)
      f(it.first, it.second);
  }

private:
  // Map from feature id to RoadJointIds.
  std::unordered_map<uint32_t, RoadJointIds> m_roads;
};
}  // namespace routing
