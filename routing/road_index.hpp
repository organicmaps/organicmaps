#pragma once

#include "routing/joint.hpp"

#include "base/checked_cast.hpp"

#include "std/algorithm.hpp"
#include "std/cstdint.hpp"
#include "std/unordered_map.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

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
    {
      if (jointId != Joint::kInvalidId)
        ++count;
    }

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

  pair<Joint::Id, uint32_t> FindNeighbor(uint32_t pointId, bool forward) const
  {
    uint32_t const size = static_cast<uint32_t>(m_jointIds.size());
    pair<Joint::Id, uint32_t> result = make_pair(Joint::kInvalidId, 0);

    if (forward)
    {
      for (uint32_t i = pointId + 1; i < size; ++i)
      {
        Joint::Id const jointId = m_jointIds[i];
        if (jointId != Joint::kInvalidId)
        {
          result = {jointId, i};
          return result;
        }
      }
    }
    else
    {
      for (uint32_t i = min(pointId, size) - 1; i < size; --i)
      {
        Joint::Id const jointId = m_jointIds[i];
        if (jointId != Joint::kInvalidId)
        {
          result = {jointId, i};
          return result;
        }
      }
    }

    return result;
  }

private:
  // Joint ids indexed by point id.
  // If some point id doesn't match any joint id, this vector contains Joint::kInvalidId.
  vector<Joint::Id> m_jointIds;
};

class RoadIndex final
{
public:
  void Import(vector<Joint> const & joints);

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
  pair<Joint::Id, uint32_t> FindNeighbor(RoadPoint const & rp, bool forward) const;

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
  unordered_map<uint32_t, RoadJointIds> m_roads;
};
}  // namespace routing
