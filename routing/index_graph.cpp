#include "routing/index_graph.hpp"

#include "routing/restrictions_serialization.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"

#include <algorithm>
#include <limits>

namespace
{
using namespace routing;
using namespace std;

bool IsRestricted(RestrictionVec const & restrictions, vector<SegmentEdge> const & edges,
                  Segment const & u, Segment const & v, bool isOutgoing)
{
  uint32_t const featureIdFrom = isOutgoing ? u.GetFeatureId() : v.GetFeatureId();
  uint32_t const featureIdTo = isOutgoing ? v.GetFeatureId() : u.GetFeatureId();

  // Looking for at least one restriction of type No from |featureIdFrom| to |featureIdTo|.
  if (binary_search(restrictions.cbegin(), restrictions.cend(),
                    Restriction(Restriction::Type::No, {featureIdFrom, featureIdTo})))
  {
    if (featureIdFrom != featureIdTo)
      return true;

    // @TODO(bykoianko) According to current code if a feature id is marked as a feature with
    // restrictricted U-turn it's restricted to make a U-turn on the both ends of the feature.
    // Generally speaking it's wrong. In osm there's information about the end of the feature
    // where the U-turn is restricted. It's necessary to pass the data to mwm and to use it here.
    // Please see test LineGraph_RestrictionF1F1No for details.
    return u.GetSegmentIdx() == v.GetSegmentIdx() && u.IsForward() != v.IsForward();
  }

  // Taking into account that number of restrictions of type Only starting and
  // ending with the same feature id is relevantly small it's possible to ignore such cases.
  if (featureIdFrom == featureIdTo)
    return false;

  // Looking for a range of restrictins of type Only starting from |featureIdFrom|
  // and finishing with any feature.
  auto const range = equal_range(restrictions.cbegin(), restrictions.cend(),
                                 Restriction(Restriction::Type::Only, {featureIdFrom}),
                                 [](Restriction const & r1, Restriction const & r2) {
                                   CHECK(!r1.m_featureIds.empty(), ());
                                   CHECK(!r2.m_featureIds.empty(), ());
                                   if (r1.m_type != r2.m_type)
                                     return r1.m_type < r2.m_type;
                                   return r1.m_featureIds[0] < r2.m_featureIds[0];
                                 });
  auto const lower = range.first;
  auto const upper = range.second;

  // Checking if there's at least one restrictions of type Only from |featureIdFrom| and
  // ending with |featureIdTo|. If yes, returns false.
  if (lower != upper && lower->m_featureIds[1] == featureIdTo)
    return false;

  // Checking if there's at least one Only starting from |featureIdFrom|.
  // If not, returns false.
  if (lower == restrictions.cend() || lower->m_type != Restriction::Type::Only ||
      lower->m_featureIds[0] != featureIdFrom)
  {
    return false;
  }

  // Note. At this point it's clear that there is an item in |restrictions| with type Only
  // and starting with |featureIdFrom|. So there are two possibilities:
  // * |edges| contains |it->m_featureIds[1]| => the end of |featureIdFrom| which implies in
  //   |restrictions| is considered.
  // * |edges| does not contain |it->m_featureIds[1]| => either the other end or an intermediate
  // point of |featureIdFrom| is considered.
  // See test FGraph_RestrictionF0F2Only for details.
  CHECK_EQUAL(lower->m_featureIds.size(), 2, ("Only two link restrictions are support."));
  for (SegmentEdge const & e : edges)
  {
    if (e.GetTarget().GetFeatureId() == lower->m_featureIds[isOutgoing ? 1 /* to */ : 0 /* from */])
      return true;
  }
  return false;
}

bool IsUTurn(Segment const & u, Segment const & v)
{
  return u.GetFeatureId() == v.GetFeatureId()
      && u.GetSegmentIdx() == v.GetSegmentIdx()
      && u.IsForward() != v.IsForward();
}
}  // namespace

namespace routing
{
IndexGraph::IndexGraph(unique_ptr<GeometryLoader> loader, shared_ptr<EdgeEstimator> estimator)
  : m_geometry(move(loader)), m_estimator(move(estimator))
{
  ASSERT(m_estimator, ());
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

  // Removing some edges according to restriction rules.
  vector<SegmentEdge> filteredEdges;
  filteredEdges.reserve(edges.size());
  for (SegmentEdge const & e : edges)
  {
    if (!IsRestricted(m_restrictions, edges, segment, e.GetTarget(), isOutgoing))
      filteredEdges.push_back(e);
  }
  edges.swap(filteredEdges);
}

void IndexGraph::Build(uint32_t numJoints) { m_jointIndex.Build(m_roadIndex, numJoints); }

void IndexGraph::Import(vector<Joint> const & joints)
{
  m_roadIndex.Import(joints);
  CHECK_LESS_OR_EQUAL(joints.size(), numeric_limits<uint32_t>::max(), ());
  Build(static_cast<uint32_t>(joints.size()));
}

void IndexGraph::SetRestrictions(RestrictionVec && restrictions)
{
  ASSERT(is_sorted(restrictions.cbegin(), restrictions.cend()), ());
  m_restrictions = move(restrictions);
}

double IndexGraph::CalcSegmentWeight(Segment const & segment)
{
  return m_estimator->CalcSegmentWeight(segment, m_geometry.GetRoad(segment.GetFeatureId()));
}

void IndexGraph::GetNeighboringEdges(Segment const & from, RoadPoint const & rp, bool isOutgoing,
                                     vector<SegmentEdge> & edges)
{
  RoadGeometry const & road = m_geometry.GetRoad(rp.GetFeatureId());

  if (!road.IsValid())
    return;

  bool const bidirectional = !road.IsOneWay();

  if ((isOutgoing || bidirectional) && rp.GetPointId() + 1 < road.GetPointsCount())
  {
    GetNeighboringEdge(from, Segment(rp.GetFeatureId(), rp.GetPointId(), isOutgoing), isOutgoing,
                       edges);
  }

  if ((!isOutgoing || bidirectional) && rp.GetPointId() > 0)
  {
    GetNeighboringEdge(from, Segment(rp.GetFeatureId(), rp.GetPointId() - 1, !isOutgoing),
                       isOutgoing, edges);
  }
}

void IndexGraph::GetNeighboringEdge(Segment const & from, Segment const & to, bool isOutgoing,
                                    vector<SegmentEdge> & edges)
{
  RoadPoint const rp = from.GetRoadPoint(isOutgoing);
  if (IsUTurn(from, to)
      && m_roadIndex.GetJointId(rp) == Joint::kInvalidId
      && rp.GetPointId() != 0
      && rp.GetPointId() + 1 != m_geometry.GetRoad(from.GetFeatureId()).GetPointsCount())
  {
    return;
  }

  double const weight = CalcSegmentWeight(isOutgoing ? to : from) +
                        GetPenalties(isOutgoing ? from : to, isOutgoing ? to : from);
  edges.emplace_back(to, weight);
}

double IndexGraph::GetPenalties(Segment const & u, Segment const & v)
{
  if (IsUTurn(u, v))
    return GetEstimator().GetUTurnPenalty();

  return 0.0;
}
}  // namespace routing
