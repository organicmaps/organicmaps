#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/joint.hpp"
#include "routing/joint_index.hpp"
#include "routing/restrictions_serialization.hpp"
#include "routing/road_access.hpp"
#include "routing/road_index.hpp"
#include "routing/road_point.hpp"
#include "routing/segment.hpp"

#include "geometry/point2d.hpp"

#include "std/cstdint.hpp"
#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace routing
{
class IndexGraph final
{
public:
  // AStarAlgorithm types aliases:
  using TVertexType = Segment;
  using TEdgeType = SegmentEdge;
  using TWeightType = RouteWeight;

  IndexGraph() = default;
  explicit IndexGraph(unique_ptr<GeometryLoader> loader, shared_ptr<EdgeEstimator> estimator);

  // Put outgoing (or ingoing) egdes for segment to the 'edges' vector.
  void GetEdgeList(Segment const & segment, bool isOutgoing, vector<SegmentEdge> & edges);

  Joint::Id GetJointId(RoadPoint const & rp) const { return m_roadIndex.GetJointId(rp); }

  Geometry & GetGeometry() { return m_geometry; }
  bool IsRoad(uint32_t featureId) const { return m_roadIndex.IsRoad(featureId); }
  RoadJointIds const & GetRoad(uint32_t featureId) const { return m_roadIndex.GetRoad(featureId); }

  RoadAccess::Type GetAccessType(Segment const & segment) const
  {
    return m_roadAccess.GetSegmentType(segment);
  }

  uint32_t GetNumRoads() const { return m_roadIndex.GetSize(); }
  uint32_t GetNumJoints() const { return m_jointIndex.GetNumJoints(); }
  uint32_t GetNumPoints() const { return m_jointIndex.GetNumPoints(); }

  void Build(uint32_t numJoints);
  void Import(vector<Joint> const & joints);

  void SetRestrictions(RestrictionVec && restrictions);
  void SetRoadAccess(RoadAccess && roadAccess);

  // Interface for AStarAlgorithm:
  void GetOutgoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges);
  void GetIngoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges);
  RouteWeight HeuristicCostEstimate(Segment const & from, Segment const & to);

  void PushFromSerializer(Joint::Id jointId, RoadPoint const & rp)
  {
    m_roadIndex.PushFromSerializer(jointId, rp);
  }

  template <typename F>
  void ForEachRoad(F && f) const
  {
    m_roadIndex.ForEachRoad(forward<F>(f));
  }

  template <typename F>
  void ForEachPoint(Joint::Id jointId, F && f) const
  {
    m_jointIndex.ForEachPoint(jointId, forward<F>(f));
  }

private:
  RouteWeight CalcSegmentWeight(Segment const & segment);
  void GetNeighboringEdges(Segment const & from, RoadPoint const & rp, bool isOutgoing,
                           vector<SegmentEdge> & edges);
  void GetNeighboringEdge(Segment const & from, Segment const & to, bool isOutgoing,
                          vector<SegmentEdge> & edges);
  RouteWeight GetPenalties(Segment const & u, Segment const & v);
  m2::PointD const & GetPoint(Segment const & segment, bool front)
  {
    return GetGeometry().GetRoad(segment.GetFeatureId()).GetPoint(segment.GetPointId(front));
  }

  Geometry m_geometry;
  shared_ptr<EdgeEstimator> m_estimator;
  RoadIndex m_roadIndex;
  JointIndex m_jointIndex;
  RestrictionVec m_restrictions;
  RoadAccess m_roadAccess;
};
}  // namespace routing
