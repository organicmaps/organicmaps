#pragma once

#include "routing/fseg.hpp"
#include "routing/fseg_index.hpp"
#include "routing/geometry.hpp"
#include "routing/joint.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/point2d.hpp"

#include "std/cstdint.hpp"

namespace routing
{
class JointEdge final
{
public:
  JointEdge(JointId target, double weight) : target(target), weight(weight) {}
  JointId GetTarget() const { return target; }
  double GetWeight() const { return weight; }
private:
  JointId target;
  double weight;
};

class IndexGraph final
{
public:
  using TVertexType = JointId;
  using TEdgeType = JointEdge;

  IndexGraph() = default;
  explicit IndexGraph(unique_ptr<Geometry> geometry);

  // TGraph overloads:
  void GetOutgoingEdgesList(TVertexType vertex, vector<TEdgeType> & edges) const;
  void GetIngoingEdgesList(TVertexType vertex, vector<TEdgeType> & edges) const;
  double HeuristicCostEstimate(TVertexType from, TVertexType to) const;

  // Access methods.
  Geometry const & GetGeometry() const { return *m_geometry; }
  m2::PointD const & GetPoint(JointId jointId) const;
  size_t GetRoadsAmount() const { return m_fsegIndex.GetSize(); }
  size_t GetJointsAmount() const { return m_jointOffsets.size(); }
  size_t GetFSegsAmount() const { return m_fsegs.size(); }
  void Export(vector<Joint> const & joints);
  JointId InsertJoint(FSegId fseg);
  vector<FSegId> RedressRoute(vector<JointId> const & route) const;

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    WriteToSink(sink, static_cast<JointId>(GetJointsAmount()));
    m_fsegIndex.Serialize(sink);
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    uint32_t const jointsSize = ReadPrimitiveFromSource<uint32_t>(src);
    m_fsegIndex.Deserialize(src);
    BuildJoints(jointsSize);
  }

private:
  // Access methods.
  FSegId GetFSeg(JointId jointId) const;
  JointOffset const & GetJointOffset(JointId jointId) const;
  pair<FSegId, FSegId> FindSharedFeature(JointId jointId0, JointId jointId1) const;

  void BuildJoints(uint32_t jointsAmount);

  // Edge methods.
  void AddNeigborEdge(vector<TEdgeType> & edges, FSegId fseg, bool forward) const;
  void GetEdgesList(JointId jointId, vector<TEdgeType> & edges, bool forward) const;

  unique_ptr<Geometry> m_geometry;
  FSegIndex m_fsegIndex;
  vector<JointOffset> m_jointOffsets;
  vector<FSegId> m_fsegs;
};
}  // namespace routing
