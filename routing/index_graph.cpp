#include "index_graph.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"

#include "std/limits.hpp"

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
}

void IndexGraph::Build(uint32_t numJoints) { m_jointIndex.Build(m_roadIndex, numJoints); }

void IndexGraph::Import(vector<Joint> const & joints)
{
  m_roadIndex.Import(joints);
  CHECK_LESS_OR_EQUAL(joints.size(), numeric_limits<uint32_t>::max(), ());
  Build(static_cast<uint32_t>(joints.size()));
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
  RoadPoint const rp(from.GetPointId(isOutgoing), from.GetFeatureId());
  if (from.GetFeatureId() == to.GetFeatureId() && from.GetSegmentIdx() == to.GetSegmentIdx()
      && from.IsForward() != to.IsForward()
      && m_roadIndex.GetJointId(rp) == Joint::kInvalidId
      && rp.GetPointId() != 0
      && rp.GetPointId() + 1 != m_geometry.GetRoad(from.GetFeatureId()).GetPointsCount())
  {
    return;
  }

  double const weight = CalcSegmentWeight(isOutgoing ? to : from);
  edges.emplace_back(to, weight);
}
}  // namespace routing
