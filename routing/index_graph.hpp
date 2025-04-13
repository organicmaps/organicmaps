#pragma once

#include "routing/base/astar_graph.hpp"
#include "routing/base/astar_vertex_data.hpp"

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

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace routing
{
bool IsUTurn(Segment const & u, Segment const & v);

enum class WorldGraphMode;

class IndexGraph final
{
public:
  // AStarAlgorithm types aliases:
  using Vertex = Segment;
  using Edge = SegmentEdge;
  using Weight = RouteWeight;

  template <typename VertexType>
  using Parents = typename AStarGraph<VertexType, void, void>::Parents;

  using Restrictions = std::unordered_map<uint32_t, std::vector<std::vector<uint32_t>>>;

  using SegmentEdgeListT = SmallList<SegmentEdge>;
  using JointEdgeListT = SmallList<JointEdge>;
  using WeightListT = SmallList<RouteWeight>;
  using SegmentListT = SmallList<Segment>;
  using PointIdListT = SmallList<uint32_t>;

  IndexGraph() = default;
  IndexGraph(std::shared_ptr<Geometry> geometry, std::shared_ptr<EdgeEstimator> estimator,
             RoutingOptions routingOptions = RoutingOptions());

  // Put outgoing (or ingoing) egdes for segment to the 'edges' vector.
  void GetEdgeList(astar::VertexData<Segment, RouteWeight> const & vertexData, bool isOutgoing,
                   bool useRoutingOptions, SegmentEdgeListT & edges,
                   Parents<Segment> const & parents = {}) const;
  void GetEdgeList(Segment const & segment, bool isOutgoing, bool useRoutingOptions,
                   SegmentEdgeListT & edges,
                   Parents<Segment> const & parents = {}) const;

  void GetEdgeList(astar::VertexData<JointSegment, RouteWeight> const & parentVertexData,
                   Segment const & parent, bool isOutgoing, JointEdgeListT & edges,
                   WeightListT & parentWeights, Parents<JointSegment> const & parents) const;
  void GetEdgeList(JointSegment const & parentJoint, Segment const & parent, bool isOutgoing,
                   JointEdgeListT & edges, WeightListT & parentWeights,
                   Parents<JointSegment> const & parents) const;

  std::optional<JointEdge> GetJointEdgeByLastPoint(Segment const & parent,
                                                   Segment const & firstChild, bool isOutgoing,
                                                   uint32_t lastPoint) const;

  Joint::Id GetJointId(RoadPoint const & rp) const { return m_roadIndex.GetJointId(rp); }

  bool IsRoad(uint32_t featureId) const { return m_roadIndex.IsRoad(featureId); }
  RoadJointIds const & GetRoad(uint32_t featureId) const { return m_roadIndex.GetRoad(featureId); }
  RoadGeometry const & GetRoadGeometry(uint32_t featureId) const { return m_geometry->GetRoad(featureId); }

  Geometry & GetGeometry() const { return *m_geometry; }

  RoadAccess::Type GetAccessType(Segment const & segment) const
  {
    return m_roadAccess.GetAccessWithoutConditional(segment.GetFeatureId()).first;
  }

  uint32_t GetNumRoads() const { return m_roadIndex.GetSize(); }
  uint32_t GetNumJoints() const { return m_jointIndex.GetNumJoints(); }
  uint32_t GetNumPoints() const { return m_jointIndex.GetNumPoints(); }

  void Build(uint32_t numJoints);
  void Import(std::vector<Joint> const & joints);

  void SetRestrictions(RestrictionVec && restrictions);
  void SetUTurnRestrictions(std::vector<RestrictionUTurn> && noUTurnRestrictions);
  void SetRoadAccess(RoadAccess && roadAccess);

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
    m_jointIndex.ForEachPoint(jointId, std::forward<F>(f));
  }

  bool IsJoint(RoadPoint const & roadPoint) const;
  bool IsJointOrEnd(Segment const & segment, bool fromStart) const;
  void GetLastPointsForJoint(SegmentListT const & children, bool isOutgoing,
                             PointIdListT & lastPoints) const;

  WorldGraphMode GetMode() const;
  ms::LatLon const & GetPoint(Segment const & segment, bool front) const
  {
    return GetRoadGeometry(segment.GetFeatureId()).GetPoint(segment.GetPointId(front));
  }

  /// \brief Check, that we can go to |currentFeatureId|.
  /// We pass |parentFeatureId| and don't use |parent.GetFeatureId()| because
  /// |parent| can be fake sometimes but |parentFeatureId| is almost non-fake.
  template <typename ParentVertex>
  bool IsRestricted(ParentVertex const & parent,
                    uint32_t parentFeatureId,
                    uint32_t currentFeatureId, bool isOutgoing,
                    Parents<ParentVertex> const & parents) const;

  bool IsUTurnAndRestricted(Segment const & parent, Segment const & child, bool isOutgoing) const;

  /// @param[in]  isOutgoing true, when movig from -> to, false otherwise.
  /// @param[in]  prevWeight used for fetching access:conditional.
  /// I suppose :) its time when user will be at the end of |from| (|to| if \a isOutgoing == false) segment.
  /// @return Transition weight + |to| (|from| if \a isOutgoing == false) segment's weight.
  RouteWeight CalculateEdgeWeight(EdgeEstimator::Purpose purpose, bool isOutgoing,
                                  Segment const & from, Segment const & to,
                                  std::optional<RouteWeight const> const & prevWeight = std::nullopt) const;

  template <typename T>
  void SetCurrentTimeGetter(T && t) { m_currentTimeGetter = std::forward<T>(t); }

private:
  void GetEdgeListImpl(astar::VertexData<Segment, RouteWeight> const & vertexData, bool isOutgoing,
                       bool useRoutingOptions, bool useAccessConditional,
                       SegmentEdgeListT & edges, Parents<Segment> const & parents) const;

  void GetEdgeListImpl(astar::VertexData<JointSegment, RouteWeight> const & parentVertexData,
                       Segment const & parent, bool isOutgoing, bool useAccessConditional,
                       JointEdgeListT & edges, WeightListT & parentWeights,
                       Parents<JointSegment> const & parents) const;

  void GetNeighboringEdges(astar::VertexData<Segment, RouteWeight> const & fromVertexData,
                           RoadPoint const & rp, bool isOutgoing, bool useRoutingOptions,
                           SegmentEdgeListT & edges, Parents<Segment> const & parents,
                           bool useAccessConditional) const;
  void GetNeighboringEdge(astar::VertexData<Segment, RouteWeight> const & fromVertexData,
                          Segment const & to, bool isOutgoing, SegmentEdgeListT & edges,
                          Parents<Segment> const & parents, bool useAccessConditional) const;

  struct PenaltyData
  {
    PenaltyData(bool passThroughAllowed, bool isFerry)
      : m_passThroughAllowed(passThroughAllowed),
        m_isFerry(isFerry) {}

    bool m_passThroughAllowed;
    bool m_isFerry;
  };

  PenaltyData GetRoadPenaltyData(Segment const & segment) const;

  /// \brief Calculates penalties for moving from |u| to |v|.
  /// \param |prevWeight| uses for fetching access:conditional. In fact it is time when user
  /// will be at |u|. This time is based on start time of route building and weight of calculated
  /// path until |u|.
  RouteWeight GetPenalties(EdgeEstimator::Purpose purpose, Segment const & u, Segment const & v,
                           std::optional<RouteWeight> const & prevWeight) const;

  void GetSegmentCandidateForRoadPoint(RoadPoint const & rp, NumMwmId numMwmId,
                                       bool isOutgoing, SegmentListT & children) const;
  void GetSegmentCandidateForJoint(Segment const & parent, bool isOutgoing, SegmentListT & children) const;
  void ReconstructJointSegment(astar::VertexData<JointSegment, RouteWeight> const & parentVertexData,
                               Segment const & parent,
                               SegmentListT const & firstChildren,
                               PointIdListT const & lastPointIds,
                               bool isOutgoing,
                               JointEdgeListT & jointEdges,
                               WeightListT & parentWeights,
                               Parents<JointSegment> const & parents) const;

  template <typename AccessPositionType>
  bool IsAccessNoForSure(AccessPositionType const & accessPositionType,
                         RouteWeight const & weight, bool useAccessConditional) const;

  std::shared_ptr<Geometry> m_geometry;
  std::shared_ptr<EdgeEstimator> m_estimator;
  RoadIndex m_roadIndex;
  JointIndex m_jointIndex;

  Restrictions m_restrictionsForward;
  Restrictions m_restrictionsBackward;

  // u_turn can be in both sides of feature.
  struct UTurnEnding
  {
    bool m_atTheBegin = false;
    bool m_atTheEnd = false;
  };
  // Stored featureId and it's UTurnEnding, which shows where is
  // u_turn restriction is placed - at the beginning or at the ending of feature.
  //
  // If m_noUTurnRestrictions.count(featureId) == 0, that means, that there are no any
  // no_u_turn restriction at the feature with id = featureId.
  std::unordered_map<uint32_t, UTurnEnding> m_noUTurnRestrictions;

  RoadAccess m_roadAccess;
  RoutingOptions m_avoidRoutingOptions;

  std::function<time_t()> m_currentTimeGetter = []() {
    return GetCurrentTimestamp();
  };
};

template <typename AccessPositionType>
bool IndexGraph::IsAccessNoForSure(AccessPositionType const & accessPositionType,
                                   RouteWeight const & weight, bool useAccessConditional) const
{
  auto const [accessType, confidence] =
      useAccessConditional ? m_roadAccess.GetAccess(accessPositionType, weight)
                           : m_roadAccess.GetAccessWithoutConditional(accessPositionType);
  return accessType == RoadAccess::Type::No && confidence == RoadAccess::Confidence::Sure;
}

template <typename ParentVertex>
bool IndexGraph::IsRestricted(ParentVertex const & parent,
                              uint32_t parentFeatureId,
                              uint32_t currentFeatureId,
                              bool isOutgoing,
                              Parents<ParentVertex> const & parents) const
{
  if (parentFeatureId == currentFeatureId)
    return false;

  auto const & restrictions = isOutgoing ? m_restrictionsForward : m_restrictionsBackward;
  auto const it = restrictions.find(currentFeatureId);
  if (it == restrictions.cend())
    return false;

  std::vector<ParentVertex> parentsFromCurrent;
  // Finds the first featureId from parents, that differ from |p.GetFeatureId()|.
  auto const appendNextParent = [&parents](ParentVertex const & p, auto & parentsVector)
  {
    uint32_t prevFeatureId = p.GetFeatureId();
    uint32_t curFeatureId = prevFeatureId;

    auto nextParent = parents.end();
    auto * curParent = &p;
    while (curFeatureId == prevFeatureId)
    {
      auto const parentIt = parents.find(*curParent);
      if (parentIt == parents.cend())
        return false;

      curFeatureId = parentIt->second.GetFeatureId();
      nextParent = parentIt;
      curParent = &nextParent->second;
    }

    ASSERT(nextParent != parents.end(), ());
    parentsVector.emplace_back(nextParent->second);
    return true;
  };

  for (std::vector<uint32_t> const & restriction : it->second)
  {
    bool const prevIsParent = restriction[0] == parentFeatureId;
    if (!prevIsParent)
      continue;

    if (restriction.size() == 1)
      return true;

    // If parents are empty we process only two feature restrictions.
    if (parents.empty())
      continue;

    if (!appendNextParent(parent, parentsFromCurrent))
      continue;

    for (size_t i = 1; i < restriction.size(); ++i)
    {
      ASSERT_GREATER_OR_EQUAL(i, 1, ("Unexpected overflow."));
      if (i - 1 == parentsFromCurrent.size()
          && !appendNextParent(parentsFromCurrent.back(), parentsFromCurrent))
      {
        break;
      }

      if (parentsFromCurrent.back().GetFeatureId() != restriction[i])
        break;

      if (i + 1 == restriction.size())
        return true;
    }
  }

  return false;
}
}  // namespace routing
