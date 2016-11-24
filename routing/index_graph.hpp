#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/joint.hpp"
#include "routing/joint_index.hpp"
#include "routing/road_index.hpp"
#include "routing/road_point.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/point2d.hpp"

#include "std/cstdint.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace routing
{
class JointEdge final
{
public:
  JointEdge(Joint::Id target, double weight) : m_target(target), m_weight(weight) {}
  Joint::Id GetTarget() const { return m_target; }
  double GetWeight() const { return m_weight; }

private:
  // Target is vertex going to for outgoing edges, vertex going from for ingoing edges.
  Joint::Id const m_target;
  double const m_weight;
};

class IndexGraph final
{
public:
  IndexGraph() = default;
  explicit IndexGraph(unique_ptr<GeometryLoader> loader, shared_ptr<EdgeEstimator> estimator);

  // Creates edge for points in same feature.
  void GetDirectedEdge(uint32_t featureId, uint32_t pointFrom, uint32_t pointTo, Joint::Id target,
                       bool forward, vector<JointEdge> & edges);
  void GetNeighboringEdges(RoadPoint const & rp, bool isOutgoing, vector<JointEdge> & edges);

  // Put outgoing (or ingoing) egdes for jointId to the 'edges' vector.
  void GetEdgeList(Joint::Id jointId, bool isOutgoing, vector<JointEdge> & edges);
  Joint::Id GetJointId(RoadPoint const & rp) const { return m_roadIndex.GetJointId(rp); }
  m2::PointD const & GetPoint(Joint::Id jointId);

  Geometry & GetGeometry() { return m_geometry; }
  EdgeEstimator const & GetEstimator() const { return *m_estimator; }
  uint32_t GetNumRoads() const { return m_roadIndex.GetSize(); }
  uint32_t GetNumJoints() const { return m_jointIndex.GetNumJoints(); }
  uint32_t GetNumPoints() const { return m_jointIndex.GetNumPoints(); }

  void Import(vector<Joint> const & joints);
  Joint::Id InsertJoint(RoadPoint const & rp);
  bool JointLiesOnRoad(Joint::Id jointId, uint32_t featureId) const;

  template <typename F>
  void ForEachPoint(Joint::Id jointId, F && f) const
  {
    m_jointIndex.ForEachPoint(jointId, forward<F>(f));
  }

  template <class Sink>
  void Serialize(Sink & sink) const
  {
    WriteToSink(sink, static_cast<uint32_t>(GetNumJoints()));
    m_roadIndex.Serialize(sink);
  }

  template <class Source>
  void Deserialize(Source & src)
  {
    uint32_t const jointsSize = ReadPrimitiveFromSource<uint32_t>(src);
    m_roadIndex.Deserialize(src);
    m_jointIndex.Build(m_roadIndex, jointsSize);
  }

private:
  void GetNeighboringEdge(RoadGeometry const & road, RoadPoint const & rp, bool forward,
                          vector<JointEdge> & edges) const;

  Geometry m_geometry;
  shared_ptr<EdgeEstimator> m_estimator;
  RoadIndex m_roadIndex;
  JointIndex m_jointIndex;
};
}  // namespace routing
