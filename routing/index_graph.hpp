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
  // AStarAlgorithm types aliases:
  using TVertexType = Joint::Id;
  using TEdgeType = JointEdge;

  IndexGraph() = default;
  explicit IndexGraph(unique_ptr<GeometryLoader> loader, shared_ptr<EdgeEstimator> estimator);

  // AStarAlgorithm<TGraph> overloads:
  void GetOutgoingEdgesList(Joint::Id vertex, vector<JointEdge> & edges) const;
  void GetIngoingEdgesList(Joint::Id vertex, vector<JointEdge> & edges) const;
  double HeuristicCostEstimate(Joint::Id from, Joint::Id to) const;

  Geometry const & GetGeometry() const { return m_geometry; }
  m2::PointD const & GetPoint(Joint::Id jointId) const;
  size_t GetNumRoads() const { return m_roadIndex.GetSize(); }
  size_t GetNumJoints() const { return m_jointIndex.GetNumJoints(); }
  size_t GetNumPoints() const { return m_jointIndex.GetNumPoints(); }
  void Import(vector<Joint> const & joints);
  Joint::Id InsertJoint(RoadPoint const & rp);

  // Add intermediate points to route (those don't correspond to any joint).
  //
  // Also convert joint ids to RoadPoints.
  void RedressRoute(vector<Joint::Id> const & route, vector<RoadPoint> & roadPoints) const;

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
  void AddNeighboringEdge(RoadGeometry const & road, RoadPoint rp, bool forward,
                          vector<TEdgeType> & edges) const;
  void GetEdgesList(Joint::Id jointId, bool forward, vector<TEdgeType> & edges) const;

  Geometry m_geometry;
  shared_ptr<EdgeEstimator> m_estimator;
  RoadIndex m_roadIndex;
  JointIndex m_jointIndex;
};
}  // namespace routing
