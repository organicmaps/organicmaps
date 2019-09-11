#include "routing/index_graph.hpp"

#include "routing/restrictions_serialization.hpp"
#include "routing/routing_options.hpp"
#include "routing/world_graph.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/exception.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <utility>

using namespace base;
using namespace std;

namespace routing
{
bool IsUTurn(Segment const & u, Segment const & v)
{
  return u.GetFeatureId() == v.GetFeatureId() && u.GetSegmentIdx() == v.GetSegmentIdx() &&
         u.IsForward() != v.IsForward();
}

bool IsBoarding(bool prevIsFerry, bool nextIsFerry)
{
  return !prevIsFerry && nextIsFerry;
}

std::map<Segment, Segment> IndexGraph::kEmptyParentsSegments = {};

IndexGraph::IndexGraph(shared_ptr<Geometry> geometry, shared_ptr<EdgeEstimator> estimator,
                       RoutingOptions routingOptions)
  : m_geometry(move(geometry)),
    m_estimator(move(estimator)),
    m_avoidRoutingOptions(routingOptions)
{
  CHECK(m_geometry, ());
  CHECK(m_estimator, ());
}

bool IndexGraph::IsJoint(RoadPoint const & roadPoint) const
{
  return m_roadIndex.GetJointId(roadPoint) != Joint::kInvalidId;
}

bool IndexGraph::IsJointOrEnd(Segment const & segment, bool fromStart)
{
  if (IsJoint(segment.GetRoadPoint(fromStart)))
    return true;

  // For features, that ends out of mwm. In this case |m_graph.IsJoint| returns false, but we should
  // think, that it's Joint anyway.
  uint32_t const pointId = segment.GetPointId(fromStart);
  if (pointId == 0)
    return true;

  uint32_t const pointsNumber = GetGeometry().GetRoad(segment.GetFeatureId()).GetPointsCount();
  return pointId + 1 == pointsNumber;
}

void IndexGraph::GetEdgeList(Segment const & segment, bool isOutgoing, bool useRoutingOptions,
                             vector<SegmentEdge> & edges, map<Segment, Segment> & parents)
{
  RoadPoint const roadPoint = segment.GetRoadPoint(isOutgoing);
  Joint::Id const jointId = m_roadIndex.GetJointId(roadPoint);

  if (jointId != Joint::kInvalidId)
  {
    m_jointIndex.ForEachPoint(jointId, [&](RoadPoint const & rp) {
      GetNeighboringEdges(segment, rp, isOutgoing, useRoutingOptions, edges, parents);
    });
  }
  else
  {
    GetNeighboringEdges(segment, roadPoint, isOutgoing, useRoutingOptions, edges, parents);
  }
}

void IndexGraph::GetLastPointsForJoint(vector<Segment> const & children,
                                       bool isOutgoing,
                                       vector<uint32_t> & lastPoints)
{
  CHECK(lastPoints.empty(), ());

  lastPoints.reserve(children.size());
  for (auto const & child : children)
  {
    uint32_t const startPointId = child.GetPointId(!isOutgoing /* front */);
    uint32_t const pointsNumber = m_geometry->GetRoad(child.GetFeatureId()).GetPointsCount();
    CHECK_LESS(startPointId, pointsNumber, ());

    uint32_t endPointId;
    // child.IsForward() == isOutgoing
    //   This is the direction, indicating the end of the road,
    //   where we should go for building JointSegment.
    // You can retrieve such result if bust possible options of |child.IsForward()| and |isOutgoing|.
    bool forward = child.IsForward() == isOutgoing;
    if (IsRoad(child.GetFeatureId()))
    {
      std::tie(std::ignore, endPointId) =
        GetRoad(child.GetFeatureId()).FindNeighbor(startPointId, forward,
                                                   pointsNumber);
    }
    else
    {
      // child.GetFeatureId() can be not road in this case:
      // -->--> { -->-->--> } -->
      // Where { ... } - borders of mwm
      // Here one feature, which enter from mwm A to mwm B and then exit from B to A.
      // And in mwm B no other feature cross this one, those feature is in geometry
      // and absent in roadIndex.
      // In this case we just return endPointNumber.
      endPointId = forward ? pointsNumber - 1 : 0;
    }

    lastPoints.push_back(endPointId);
  }
}

void IndexGraph::GetEdgeList(JointSegment const & parentJoint,
                             Segment const & parent, bool isOutgoing, vector<JointEdge> & edges,
                             vector<RouteWeight> & parentWeights, map<JointSegment, JointSegment> & parents)
{
  vector<Segment> possibleChildren;
  GetSegmentCandidateForJoint(parent, isOutgoing, possibleChildren);

  vector<uint32_t> lastPoints;
  GetLastPointsForJoint(possibleChildren, isOutgoing, lastPoints);


  ReconstructJointSegment(parentJoint, parent, possibleChildren, lastPoints,
                          isOutgoing, edges, parentWeights, parents);
}

boost::optional<JointEdge>
IndexGraph::GetJointEdgeByLastPoint(Segment const & parent, Segment const & firstChild,
                                    bool isOutgoing, uint32_t lastPoint)
{
  vector<Segment> const possibleChilds = {firstChild};
  vector<uint32_t> const lastPoints = {lastPoint};

  vector<JointEdge> edges;
  vector<RouteWeight> parentWeights;
  map<JointSegment, JointSegment> emptyParents;
  ReconstructJointSegment({} /* parentJoint */, parent, possibleChilds, lastPoints,
                          isOutgoing, edges, parentWeights, emptyParents);

  CHECK_LESS_OR_EQUAL(edges.size(), 1, ());
  if (edges.size() == 1)
    return {edges.back()};

  return {};
}

void IndexGraph::Build(uint32_t numJoints)
{
  m_jointIndex.Build(m_roadIndex, numJoints);
}

void IndexGraph::Import(vector<Joint> const & joints)
{
  m_roadIndex.Import(joints);
  CHECK_LESS_OR_EQUAL(joints.size(), numeric_limits<uint32_t>::max(), ());
  Build(checked_cast<uint32_t>(joints.size()));
}

void IndexGraph::SetRestrictions(RestrictionVec && restrictions)
{
  m_restrictionsForward.clear();
  m_restrictionsBackward.clear();

  base::HighResTimer timer;
  for (auto const & restriction : restrictions)
  {
    ASSERT(!restriction.empty(), ());

    auto & forward = m_restrictionsForward[restriction.back()];
    forward.emplace_back(restriction.begin(), prev(restriction.end()));
    reverse(forward.back().begin(), forward.back().end());

    m_restrictionsBackward[restriction.front()].emplace_back(next(restriction.begin()), restriction.end());
  }

  LOG(LDEBUG, ("Restrictions are loaded in:", timer.ElapsedNano() / 1e6, "ms"));
}

void IndexGraph::SetUTurnRestrictions(vector<RestrictionUTurn> && noUTurnRestrictions)
{
  for (auto const & noUTurn : noUTurnRestrictions)
  {
    if (noUTurn.m_viaIsFirstPoint)
      m_noUTurnRestrictions[noUTurn.m_featureId].m_atTheBegin = true;
    else
      m_noUTurnRestrictions[noUTurn.m_featureId].m_atTheEnd = true;
  }
}

void IndexGraph::SetRoadAccess(RoadAccess && roadAccess) { m_roadAccess = move(roadAccess); }

void IndexGraph::GetNeighboringEdges(Segment const & from, RoadPoint const & rp, bool isOutgoing,
                                     bool useRoutingOptions, vector<SegmentEdge> & edges,
                                     map<Segment, Segment> & parents)
{
  RoadGeometry const & road = m_geometry->GetRoad(rp.GetFeatureId());

  if (!road.IsValid())
    return;

  if (useRoutingOptions && !road.SuitableForOptions(m_avoidRoutingOptions))
    return;

  bool const bidirectional = !road.IsOneWay();

  if ((isOutgoing || bidirectional) && rp.GetPointId() + 1 < road.GetPointsCount())
  {
    GetNeighboringEdge(from,
                       Segment(from.GetMwmId(), rp.GetFeatureId(), rp.GetPointId(), isOutgoing),
                       isOutgoing, edges, parents);
  }

  if ((!isOutgoing || bidirectional) && rp.GetPointId() > 0)
  {
    GetNeighboringEdge(
        from, Segment(from.GetMwmId(), rp.GetFeatureId(), rp.GetPointId() - 1, !isOutgoing),
        isOutgoing, edges, parents);
  }
}

void IndexGraph::GetSegmentCandidateForRoadPoint(RoadPoint const & rp, NumMwmId numMwmId,
                                                 bool isOutgoing, std::vector<Segment> & children)
{
  RoadGeometry const & road = m_geometry->GetRoad(rp.GetFeatureId());
  if (!road.IsValid())
    return;

  if (!road.SuitableForOptions(m_avoidRoutingOptions))
    return;

  bool const bidirectional = !road.IsOneWay();
  auto const pointId = rp.GetPointId();

  if ((isOutgoing || bidirectional) && pointId + 1 < road.GetPointsCount())
    children.emplace_back(numMwmId, rp.GetFeatureId(), pointId, isOutgoing);

  if ((!isOutgoing || bidirectional) && pointId > 0)
    children.emplace_back(numMwmId, rp.GetFeatureId(), pointId - 1, !isOutgoing);
}

void IndexGraph::GetSegmentCandidateForJoint(Segment const & parent, bool isOutgoing,
                                             vector<Segment> & children)
{
  RoadPoint const roadPoint = parent.GetRoadPoint(isOutgoing);
  Joint::Id const jointId = m_roadIndex.GetJointId(roadPoint);

  if (jointId == Joint::kInvalidId)
    return;

  m_jointIndex.ForEachPoint(jointId, [&](RoadPoint const & rp) {
    GetSegmentCandidateForRoadPoint(rp, parent.GetMwmId(), isOutgoing, children);
  });
}

/// \brief Prolongs segments from |parent| to |firstChildren| directions in order to
///        create JointSegments.
/// \param |firstChildren| - vecotor of neigbouring segments from parent.
/// \param |lastPointIds| - vector of the end numbers of road points for |firstChildren|.
/// \param |jointEdges| - the result vector with JointEdges.
/// \param |parentWeights| - see |IndexGraphStarterJoints::GetEdgeList| method about this argument.
///                          Shortly - in case of |isOutgoing| == false, method saves here the weights
///                                   from parent to firstChildren.
void IndexGraph::ReconstructJointSegment(JointSegment const & parentJoint,
                                         Segment const & parent,
                                         vector<Segment> const & firstChildren,
                                         vector<uint32_t> const & lastPointIds,
                                         bool isOutgoing,
                                         vector<JointEdge> & jointEdges,
                                         vector<RouteWeight> & parentWeights,
                                         map<JointSegment, JointSegment> & parents)
{
  CHECK_EQUAL(firstChildren.size(), lastPointIds.size(), ());

  for (size_t i = 0; i < firstChildren.size(); ++i)
  {
    auto const & firstChild = firstChildren[i];
    auto const lastPointId = lastPointIds[i];

    uint32_t currentPointId = firstChild.GetPointId(!isOutgoing /* front */);
    CHECK_NOT_EQUAL(currentPointId, lastPointId,
                    ("Invariant violated, can not build JointSegment,"
                     "started and ended in the same point."));

    auto const increment = [currentPointId, lastPointId](uint32_t pointId) {
      return currentPointId < lastPointId ? pointId + 1 : pointId - 1;
    };

    if (m_roadAccess.GetFeatureType(firstChild.GetFeatureId()) == RoadAccess::Type::No)
      continue;

    if (m_roadAccess.GetPointType(parent.GetRoadPoint(isOutgoing)) == RoadAccess::Type::No)
      continue;

    if (IsUTurn(parent, firstChild) && IsUTurnAndRestricted(parent, firstChild, isOutgoing))
      continue;

    if (IsRestricted(parentJoint, parent.GetFeatureId(), firstChild.GetFeatureId(),
                     isOutgoing, parents))
    {
      continue;
    }

    // Check current JointSegment for bad road access between segments.
    RoadPoint rp = firstChild.GetRoadPoint(isOutgoing);
    uint32_t start = currentPointId;
    bool noRoadAccess = false;
    do
    {
      if (m_roadAccess.GetPointType(rp) == RoadAccess::Type::No)
      {
        noRoadAccess = true;
        break;
      }

      start = increment(start);
      rp.SetPointId(start);
    } while (start != lastPointId);

    if (noRoadAccess)
      continue;

    bool forward = currentPointId < lastPointId;
    Segment current = firstChild;
    Segment prev = parent;
    RouteWeight summaryWeight;

    do
    {
      RouteWeight const weight =
          CalculateEdgeWeight(EdgeEstimator::Purpose::Weight, isOutgoing, prev, current);

      if (isOutgoing || prev != parent)
        summaryWeight += weight;

      if (prev == parent)
        parentWeights.emplace_back(weight);

      prev = current;
      current.Next(forward);
      currentPointId = increment(currentPointId);
    } while (currentPointId != lastPointId);

    jointEdges.emplace_back(isOutgoing ? JointSegment(firstChild, prev) :
                                         JointSegment(prev, firstChild),
                            summaryWeight);
  }
}

void IndexGraph::GetNeighboringEdge(Segment const & from, Segment const & to, bool isOutgoing,
                                    vector<SegmentEdge> & edges, map<Segment, Segment> & parents)
{
  if (IsUTurn(from, to) && IsUTurnAndRestricted(from, to, isOutgoing))
    return;

  if (IsRestricted(from, from.GetFeatureId(), to.GetFeatureId(), isOutgoing, parents))
    return;

  if (m_roadAccess.GetFeatureType(to.GetFeatureId()) == RoadAccess::Type::No)
    return;

  RoadPoint const rp = from.GetRoadPoint(isOutgoing);
  if (m_roadAccess.GetPointType(rp) == RoadAccess::Type::No)
    return;

  RouteWeight weight = CalculateEdgeWeight(EdgeEstimator::Purpose::Weight, isOutgoing, from, to);
  edges.emplace_back(to, std::move(weight));
}

IndexGraph::PenaltyData IndexGraph::GetRoadPenaltyData(Segment const & segment)
{
  auto const & road = m_geometry->GetRoad(segment.GetFeatureId());

  PenaltyData result(road.IsPassThroughAllowed(),
                     road.GetRoutingOptions().Has(RoutingOptions::Road::Ferry));

  return result;
}

RouteWeight IndexGraph::GetPenalties(EdgeEstimator::Purpose purpose,
                                     Segment const & u, Segment const & v)
{
  auto const & fromPenaltyData = GetRoadPenaltyData(u);
  auto const & toPenaltyData = GetRoadPenaltyData(v);
  // Route crosses border of pass-through/non-pass-through area if |u| and |v| have different
  // pass through restrictions.
  int8_t const passThroughPenalty =
      fromPenaltyData.m_passThroughAllowed == toPenaltyData.m_passThroughAllowed ? 0 : 1;

  // We do not distinguish between RoadAccess::Type::Private and RoadAccess::Type::Destination for now.
  bool const fromAccessAllowed = m_roadAccess.GetFeatureType(u.GetFeatureId()) == RoadAccess::Type::Yes;
  bool const toAccessAllowed = m_roadAccess.GetFeatureType(v.GetFeatureId()) == RoadAccess::Type::Yes;
  // Route crosses border of access=yes/access={private, destination} area if |u| and |v| have different
  // access restrictions.
  int8_t accessPenalty = fromAccessAllowed == toAccessAllowed ? 0 : 1;

  // RoadPoint between u and v is front of u.
  auto const rp = u.GetRoadPoint(true /* front */);
  // No double penalty for barriers on the border of access=yes/access={private, destination} area.
  if (m_roadAccess.GetPointType(rp) != RoadAccess::Type::Yes)
    accessPenalty = 1;

  double weightPenalty = 0.0;
  if (IsUTurn(u, v))
    weightPenalty += m_estimator->GetUTurnPenalty(purpose);

  if (IsBoarding(fromPenaltyData.m_isFerry, toPenaltyData.m_isFerry))
    weightPenalty += m_estimator->GetFerryLandingPenalty(purpose);

  return {weightPenalty /* weight */, passThroughPenalty, accessPenalty, 0.0 /* transitTime */};
}

WorldGraphMode IndexGraph::GetMode() const { return WorldGraphMode::Undefined; }

bool IndexGraph::IsUTurnAndRestricted(Segment const & parent, Segment const & child,
                                      bool isOutgoing) const
{
  ASSERT(IsUTurn(parent, child), ());

  uint32_t const featureId = parent.GetFeatureId();
  uint32_t const turnPoint = parent.GetPointId(isOutgoing);
  auto const & roadGeometry = m_geometry->GetRoad(featureId);

  RoadPoint const rp = parent.GetRoadPoint(isOutgoing);
  if (m_roadIndex.GetJointId(rp) == Joint::kInvalidId && !roadGeometry.IsEndPointId(turnPoint))
    return true;

  auto const it = m_noUTurnRestrictions.find(featureId);
  if (it == m_noUTurnRestrictions.cend())
    return false;

  auto const & uTurn = it->second;
  if (uTurn.m_atTheBegin && turnPoint == 0)
    return true;

  uint32_t const n = roadGeometry.GetPointsCount();
  ASSERT_GREATER_OR_EQUAL(n, 1, ());
  return uTurn.m_atTheEnd && turnPoint == n - 1;
}

RouteWeight IndexGraph::CalculateEdgeWeight(EdgeEstimator::Purpose purpose, bool isOutgoing,
                                            Segment const & from, Segment const & to)
{
  auto const getWeight = [this, isOutgoing, &to, &from, purpose]() {
    auto const & segment = isOutgoing ? to : from;
    auto const & road = m_geometry->GetRoad(segment.GetFeatureId());
    return RouteWeight(m_estimator->CalcSegmentWeight(segment, road, purpose));
  };

  auto const & weight = getWeight();
  auto const & penalties = GetPenalties(purpose, isOutgoing ? from : to, isOutgoing ? to : from);

  return weight + penalties;
}
}  // namespace routing
