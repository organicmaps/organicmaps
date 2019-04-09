#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/joint.hpp"
#include "routing/joint_index.hpp"
#include "routing/joint_segment.hpp"
#include "routing/restrictions_serialization.hpp"
#include "routing/road_access.hpp"
#include "routing/road_index.hpp"
#include "routing/road_point.hpp"
#include "routing/routing_options.hpp"
#include "routing/segment.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

namespace routing
{
enum class WorldGraphMode;

class IndexGraph final
{
public:
  // AStarAlgorithm types aliases:
  using Vertex = Segment;
  using Edge = SegmentEdge;
  using Weight = RouteWeight;

  IndexGraph() = default;
  IndexGraph(std::shared_ptr<Geometry> geometry, std::shared_ptr<EdgeEstimator> estimator,
             RoutingOptions routingOptions = RoutingOptions());

  // Put outgoing (or ingoing) egdes for segment to the 'edges' vector.
  void GetEdgeList(Segment const & segment, bool isOutgoing, std::vector<SegmentEdge> & edges);

  void GetEdgeList(Segment const & parent, bool isOutgoing, std::vector<JointEdge> & edges,
                   std::vector<RouteWeight> & parentWeights);

  boost::optional<JointEdge> GetJointEdgeByLastPoint(Segment const & parent, Segment const & firstChild,
                                                     bool isOutgoing, uint32_t lastPoint);

  Joint::Id GetJointId(RoadPoint const & rp) const { return m_roadIndex.GetJointId(rp); }

  Geometry & GetGeometry() { return *m_geometry; }
  bool IsRoad(uint32_t featureId) const { return m_roadIndex.IsRoad(featureId); }
  RoadJointIds const & GetRoad(uint32_t featureId) const { return m_roadIndex.GetRoad(featureId); }

  RoadAccess::Type GetAccessType(Segment const & segment) const
  {
    return m_roadAccess.GetFeatureType(segment.GetFeatureId());
  }

  uint32_t GetNumRoads() const { return m_roadIndex.GetSize(); }
  uint32_t GetNumJoints() const { return m_jointIndex.GetNumJoints(); }
  uint32_t GetNumPoints() const { return m_jointIndex.GetNumPoints(); }

  void Build(uint32_t numJoints);
  void Import(std::vector<Joint> const & joints);

  void SetRestrictions(RestrictionVec && restrictions);
  void SetRoadAccess(RoadAccess && roadAccess);

  // Interface for AStarAlgorithm:
  void GetOutgoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges);
  void GetIngoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges);

  void PushFromSerializer(Joint::Id jointId, RoadPoint const & rp)
  {
    m_roadIndex.PushFromSerializer(jointId, rp);
  }

  template <typename F>
  void ForEachRoad(F && f) const
  {
    m_roadIndex.ForEachRoad(std::forward<F>(f));
  }

  template <typename F>
  void ForEachPoint(Joint::Id jointId, F && f) const
  {
    m_jointIndex.ForEachPoint(jointId, forward<F>(f));
  }

  bool IsJoint(RoadPoint const & roadPoint) const;
  void GetLastPointsForJoint(std::vector<Segment> const & children, bool isOutgoing,
                             std::vector<uint32_t> & lastPoints);

  WorldGraphMode GetMode() const;
  m2::PointD const & GetPoint(Segment const & segment, bool front)
  {
    return GetGeometry().GetRoad(segment.GetFeatureId()).GetPoint(segment.GetPointId(front));
  }

  RouteWeight CalcSegmentWeight(Segment const & segment);

private:
  void GetNeighboringEdges(Segment const & from, RoadPoint const & rp, bool isOutgoing,
                           std::vector<SegmentEdge> & edges);
  void GetNeighboringEdge(Segment const & from, Segment const & to, bool isOutgoing,
                          std::vector<SegmentEdge> & edges);
  RouteWeight GetPenalties(Segment const & u, Segment const & v);

  void GetSegmentCandidateForJoint(Segment const & parent, bool isOutgoing, std::vector<Segment> & children);
  void ReconstructJointSegment(Segment const & parent, std::vector<Segment> const & firstChildren,
                               std::vector<uint32_t> const & lastPointIds,
                               bool isOutgoing, std::vector<JointEdge> & jointEdges,
                               std::vector<RouteWeight> & parentWeights);

  std::shared_ptr<Geometry> m_geometry;
  std::shared_ptr<EdgeEstimator> m_estimator;
  RoadIndex m_roadIndex;
  JointIndex m_jointIndex;
  RestrictionVec m_restrictions;
  RoadAccess m_roadAccess;
  RoutingOptions m_avoidRoutingOptions;
};
}  // namespace routing
