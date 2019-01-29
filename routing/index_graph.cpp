#include "routing/index_graph.hpp"

#include "routing/restrictions_serialization.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/exception.hpp"

#include <algorithm>
#include <limits>

namespace
{
using namespace base;
using namespace routing;
using namespace std;

bool IsUTurn(Segment const & u, Segment const & v)
{
  return u.GetFeatureId() == v.GetFeatureId() && u.GetSegmentIdx() == v.GetSegmentIdx() &&
         u.IsForward() != v.IsForward();
}

bool IsRestricted(RestrictionVec const & restrictions, Segment const & u, Segment const & v,
                  bool isOutgoing)
{
  uint32_t const featureIdFrom = isOutgoing ? u.GetFeatureId() : v.GetFeatureId();
  uint32_t const featureIdTo = isOutgoing ? v.GetFeatureId() : u.GetFeatureId();

  if (!binary_search(restrictions.cbegin(), restrictions.cend(),
                     Restriction(Restriction::Type::No, {featureIdFrom, featureIdTo})))
  {
    return false;
  }

  if (featureIdFrom != featureIdTo)
    return true;

  // @TODO(bykoianko) According to current code if a feature id is marked as a feature with
  // restrictricted U-turn it's restricted to make a U-turn on the both ends of the feature.
  // Generally speaking it's wrong. In osm there's information about the end of the feature
  // where the U-turn is restricted. It's necessary to pass the data to mwm and to use it here.
  // Please see test LineGraph_RestrictionF1F1No for details.
  //
  // Another example when it's necessary to be aware about feature end participated in restriction
  // is
  //        *---F1---*
  //        |        |
  // *--F3--A        B--F4--*
  //        |        |
  //        *---F2---*
  // In case of restriction F1-A-F2 or F1-B-F2 of any type (No, Only) the important information
  // is lost.
  return IsUTurn(u, v);
}
}  // namespace

namespace routing
{
IndexGraph::IndexGraph(shared_ptr<Geometry> geometry, shared_ptr<EdgeEstimator> estimator)
  : m_geometry(geometry), m_estimator(move(estimator))
{
  CHECK(m_geometry, ());
  CHECK(m_estimator, ());
}

bool IndexGraph::IsJoint(RoadPoint const & roadPoint) const
{
  return m_roadIndex.GetJointId(roadPoint) != Joint::kInvalidId;
}

void IndexGraph::GetEdgeList(Segment const & segment, bool isOutgoing, vector<SegmentEdge> & edges)
{
  RoadPoint const roadPoint = segment.GetRoadPoint(isOutgoing);
  Joint::Id const jointId = m_roadIndex.GetJointId(roadPoint);

  if (jointId != Joint::kInvalidId)
  {
    m_jointIndex.ForEachPoint(jointId, [&](RoadPoint const & rp) {
      GetNeighboringEdges(segment, rp, isOutgoing, edges);
    });
  }
  else
  {
    GetNeighboringEdges(segment, roadPoint, isOutgoing, edges);
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
    std::tie(std::ignore, endPointId) =
      GetRoad(child.GetFeatureId()).FindNeighbor(startPointId, child.IsForward() == isOutgoing,
                                                 pointsNumber);

    lastPoints.push_back(endPointId);
  }
}

void IndexGraph::GetEdgeList(Segment const & parent, bool isOutgoing, vector<JointEdge> & edges,
                             vector<RouteWeight> & parentWeights)
{
  vector<Segment> possibleChildren;
  GetSegmentCandidateForJoint(parent, isOutgoing, possibleChildren);

  vector<uint32_t> lastPoints;
  GetLastPointsForJoint(possibleChildren, isOutgoing, lastPoints);

  ReconstructJointSegment(parent, possibleChildren, lastPoints,
                          isOutgoing, edges, parentWeights);
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
  ASSERT(is_sorted(restrictions.cbegin(), restrictions.cend()), ());
  m_restrictions = move(restrictions);
}

void IndexGraph::SetRoadAccess(RoadAccess && roadAccess) { m_roadAccess = move(roadAccess); }

void IndexGraph::GetOutgoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, true /* isOutgoing */, edges);
}

void IndexGraph::GetIngoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, false /* isOutgoing */, edges);
}

RouteWeight IndexGraph::CalcSegmentWeight(Segment const & segment)
{
  return RouteWeight(
      m_estimator->CalcSegmentWeight(segment, m_geometry->GetRoad(segment.GetFeatureId())));
}

void IndexGraph::GetNeighboringEdges(Segment const & from, RoadPoint const & rp, bool isOutgoing,
                                     vector<SegmentEdge> & edges)
{
  RoadGeometry const & road = m_geometry->GetRoad(rp.GetFeatureId());

  if (!road.IsValid())
    return;

  bool const bidirectional = !road.IsOneWay();

  if ((isOutgoing || bidirectional) && rp.GetPointId() + 1 < road.GetPointsCount())
  {
    GetNeighboringEdge(from,
                       Segment(from.GetMwmId(), rp.GetFeatureId(), rp.GetPointId(), isOutgoing),
                       isOutgoing, edges);
  }

  if ((!isOutgoing || bidirectional) && rp.GetPointId() > 0)
  {
    GetNeighboringEdge(
        from, Segment(from.GetMwmId(), rp.GetFeatureId(), rp.GetPointId() - 1, !isOutgoing),
        isOutgoing, edges);
  }
}

void IndexGraph::GetSegmentCandidateForJoint(Segment const & parent, bool isOutgoing,
                                             vector<Segment> & children)
{
  RoadPoint const roadPoint = parent.GetRoadPoint(isOutgoing);
  Joint::Id const jointId = m_roadIndex.GetJointId(roadPoint);

  if (jointId == Joint::kInvalidId)
    return;

  m_jointIndex.ForEachPoint(jointId, [&](RoadPoint const & rp)
  {
    RoadGeometry const & road = m_geometry->GetRoad(rp.GetFeatureId());
    if (!road.IsValid())
      return;

    bool const bidirectional = !road.IsOneWay();
    auto const pointId = rp.GetPointId();

    if ((isOutgoing || bidirectional) && pointId + 1 < road.GetPointsCount())
      children.emplace_back(parent.GetMwmId(), rp.GetFeatureId(), pointId, isOutgoing);

    if ((!isOutgoing || bidirectional) && pointId > 0)
      children.emplace_back(parent.GetMwmId(), rp.GetFeatureId(), pointId - 1, !isOutgoing);
  });
}

/// \brief Prolongs segments from |parent| to |firstChildren| directions in order to
///        create JointSegments.
/// \param |firstChildren| - vecotor of neigbouring segments from parent.
/// \param |lastPointIds| - vector of the end numbers of road points for |firstChildren|.
/// \param |jointEdges| - the result vector with JointEdges.
/// \param |parentWeights| - see |IndexGraphStarterJoints::GetEdgeList| method about this argument.
///                          Shotly - in case of |isOutgoing| == false, method saves here the weights
///                                   from parent to firstChildren.
void IndexGraph::ReconstructJointSegment(Segment const & parent,
                                         vector<Segment> const & firstChildren,
                                         vector<uint32_t> const & lastPointIds,
                                         bool isOutgoing,
                                         vector<JointEdge> & jointEdges,
                                         vector<RouteWeight> & parentWeights)
{
  CHECK_EQUAL(firstChildren.size(), lastPointIds.size(), ());

  auto const step = [this](Segment const & from, Segment const & to, bool isOutgoing, SegmentEdge & edge)
  {
    if (IsRestricted(m_restrictions, from, to, isOutgoing))
      return false;

    RouteWeight weight = CalcSegmentWeight(isOutgoing ? to : from) +
                         GetPenalties(isOutgoing ? from : to, isOutgoing ? to : from);
    edge = SegmentEdge(to, weight);
    return true;
  };

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

    // Check current JointSegment for bad road access between segments.
    uint32_t start = currentPointId;
    bool noRoadAccess = false;
    RoadPoint rp = firstChild.GetRoadPoint(isOutgoing);
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

    // Check firstChild for UTurn.
    rp = parent.GetRoadPoint(isOutgoing);
    if (IsUTurn(parent, firstChild) && m_roadIndex.GetJointId(rp) == Joint::kInvalidId
        && !m_geometry->GetRoad(parent.GetFeatureId()).IsEndPointId(rp.GetPointId()))
    {
      continue;
    }

    bool forward = currentPointId < lastPointId;
    Segment current = firstChild;
    Segment prev = parent;
    SegmentEdge edge;
    RouteWeight summaryWeight;

    bool hasRestriction = false;
    do
    {
      if (step(prev, current, isOutgoing, edge)) // Access ok
      {
        if (isOutgoing || prev != parent)
          summaryWeight += edge.GetWeight();

        if (prev == parent)
          parentWeights.emplace_back(edge.GetWeight());
      }
      else
      {
        hasRestriction = true;
        break;
      }

      prev = current;
      current.Next(forward);
      currentPointId = increment(currentPointId);
    } while (currentPointId != lastPointId);

    if (hasRestriction)
      continue;

    jointEdges.emplace_back(isOutgoing ? JointSegment(firstChild, prev) :
                                         JointSegment(prev, firstChild),
                            summaryWeight);
  }
}

void IndexGraph::GetNeighboringEdge(Segment const & from, Segment const & to, bool isOutgoing,
                                    vector<SegmentEdge> & edges)
{
  // Blocking U-turns on internal feature points.
  RoadPoint const rp = from.GetRoadPoint(isOutgoing);
  if (IsUTurn(from, to) && m_roadIndex.GetJointId(rp) == Joint::kInvalidId
      && !m_geometry->GetRoad(from.GetFeatureId()).IsEndPointId(rp.GetPointId()))
  {
    return;
  }

  if (IsRestricted(m_restrictions, from, to, isOutgoing))
    return;

  if (m_roadAccess.GetFeatureType(to.GetFeatureId()) == RoadAccess::Type::No)
    return;

  if (m_roadAccess.GetPointType(rp) == RoadAccess::Type::No)
    return;

  RouteWeight const weight = CalcSegmentWeight(isOutgoing ? to : from) +
                             GetPenalties(isOutgoing ? from : to, isOutgoing ? to : from);
  edges.emplace_back(to, weight);
}

RouteWeight IndexGraph::GetPenalties(Segment const & u, Segment const & v)
{
  bool const fromPassThroughAllowed = m_geometry->GetRoad(u.GetFeatureId()).IsPassThroughAllowed();
  bool const toPassThroughAllowed = m_geometry->GetRoad(v.GetFeatureId()).IsPassThroughAllowed();
  // Route crosses border of pass-through/non-pass-through area if |u| and |v| have different
  // pass through restrictions.
  int32_t const passThroughPenalty = fromPassThroughAllowed == toPassThroughAllowed ? 0 : 1;

  // We do not distinguish between RoadAccess::Type::Private and RoadAccess::Type::Destination for now.
  bool const fromAccessAllowed = m_roadAccess.GetFeatureType(u.GetFeatureId()) == RoadAccess::Type::Yes;
  bool const toAccessAllowed = m_roadAccess.GetFeatureType(v.GetFeatureId()) == RoadAccess::Type::Yes;
  // Route crosses border of access=yes/access={private, destination} area if |u| and |v| have different
  // access restrictions.
  int32_t accessPenalty = fromAccessAllowed == toAccessAllowed ? 0 : 1;

  // RoadPoint between u and v is front of u.
  auto const rp = u.GetRoadPoint(true /* front */);
  // No double penalty for barriers on the border of access=yes/access={private, destination} area.
  if (m_roadAccess.GetPointType(rp) != RoadAccess::Type::Yes)
    accessPenalty = 1;

  auto const uTurnPenalty = IsUTurn(u, v) ? m_estimator->GetUTurnPenalty() : 0.0;

  return RouteWeight(uTurnPenalty /* weight */, passThroughPenalty, accessPenalty, 0.0 /* transitTime */);
}
}  // namespace routing
