#pragma once

#include "routing/joint.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "std/cstdint.hpp"
#include "std/unordered_map.hpp"
#include "std/vector.hpp"

namespace routing
{
class RoadJointIds final
{
public:
  JointId GetJointId(uint32_t segId) const
  {
    if (segId < m_jointIds.size())
      return m_jointIds[segId];

    return kInvalidJointId;
  }

  void AddJoint(uint32_t segId, JointId jointId)
  {
    if (segId >= m_jointIds.size())
      m_jointIds.insert(m_jointIds.end(), segId + 1 - m_jointIds.size(), kInvalidJointId);

    ASSERT_EQUAL(m_jointIds[segId], kInvalidJointId, ());
    m_jointIds[segId] = jointId;
  }

  template <typename F>
  void ForEachJoint(F && f) const
  {
    for (uint32_t segId = 0; segId < m_jointIds.size(); ++segId)
    {
      JointId const jointId = m_jointIds[segId];
      if (jointId != kInvalidJointId)
        f(segId, jointId);
    }
  }

  size_t GetSize() const { return m_jointIds.size(); }

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    WriteToSink(sink, static_cast<JointId>(m_jointIds.size()));
    for (JointId jointId : m_jointIds)
      WriteToSink(sink, jointId);
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    JointId const jointsSize = ReadPrimitiveFromSource<JointId>(src);
    m_jointIds.reserve(jointsSize);
    for (JointId i = 0; i < jointsSize; ++i)
    {
      JointId const jointId = ReadPrimitiveFromSource<JointId>(src);
      m_jointIds.emplace_back(jointId);
    }
  }

private:
  vector<JointId> m_jointIds;
};

class FSegIndex final
{
public:
  void Export(vector<Joint> const & joints);

  void AddJoint(FSegId fseg, JointId jointId)
  {
    RoadJointIds & road = m_roads[fseg.GetFeatureId()];
    road.AddJoint(fseg.GetSegId(), jointId);
  }

  pair<JointId, uint32_t> FindNeigbor(FSegId fseg, bool forward) const;

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    WriteToSink(sink, static_cast<uint32_t>(m_roads.size()));
    for (auto it = m_roads.begin(); it != m_roads.end(); ++it)
    {
      uint32_t const featureId = it->first;
      WriteToSink(sink, featureId);
      it->second.Serialize(sink);
    }
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    size_t const roadsSize = static_cast<size_t>(ReadPrimitiveFromSource<uint32_t>(src));
    for (size_t i = 0; i < roadsSize; ++i)
    {
      uint32_t featureId = ReadPrimitiveFromSource<decltype(featureId)>(src);
      m_roads[featureId].Deserialize(src);
    }
  }

  uint32_t GetSize() const { return m_roads.size(); }

  JointId GetJointId(FSegId fseg) const
  {
    auto const it = m_roads.find(fseg.GetFeatureId());
    if (it == m_roads.end())
      return kInvalidJointId;

    return it->second.GetJointId(fseg.GetSegId());
  }

  template <typename F>
  void ForEachRoad(F && f) const
  {
    for (auto it = m_roads.begin(); it != m_roads.end(); ++it)
      f(it->first, it->second);
  }

private:
  // map from feature id to RoadJointIds.
  unordered_map<uint32_t, RoadJointIds> m_roads;
};
}  // namespace routing
