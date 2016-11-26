#include "index_graph.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"

namespace routing
{
IndexGraph::IndexGraph(unique_ptr<GeometryLoader> loader, shared_ptr<EdgeEstimator> estimator)
  : m_geometry(move(loader)), m_estimator(move(estimator))
{
  ASSERT(m_estimator, ());
}

void IndexGraph::GetEdgeList(Joint::Id jointId, bool isOutgoing, vector<JointEdge> & edges)
{
  m_jointIndex.ForEachPoint(jointId, [this, &edges, isOutgoing](RoadPoint const & rp) {
    GetNeighboringEdges(rp, isOutgoing, edges);
  });
}

m2::PointD const & IndexGraph::GetPoint(Joint::Id jointId)
{
  return m_geometry.GetPoint(m_jointIndex.GetPoint(jointId));
}

void IndexGraph::Import(vector<Joint> const & joints)
{
  m_roadIndex.Import(joints);
  m_jointIndex.Build(m_roadIndex, joints.size());
}

Joint::Id IndexGraph::InsertJoint(RoadPoint const & rp)
{
  Joint::Id const existId = m_roadIndex.GetJointId(rp);
  if (existId != Joint::kInvalidId)
    return existId;

  Joint::Id const jointId = m_jointIndex.InsertJoint(rp);
  m_roadIndex.AddJoint(rp, jointId);
  return jointId;
}

bool IndexGraph::JointLiesOnRoad(Joint::Id jointId, uint32_t featureId) const
{
  bool result = false;
  m_jointIndex.ForEachPoint(jointId, [&result, featureId](RoadPoint const & rp) {
    if (rp.GetFeatureId() == featureId)
      result = true;
  });

  return result;
}

void IndexGraph::GetNeighboringEdges(RoadPoint const & rp, bool isOutgoing,
                                     vector<JointEdge> & edges)
{
  RoadGeometry const & road = m_geometry.GetRoad(rp.GetFeatureId());
  if (!road.IsRoad())
    return;

  bool const bidirectional = !road.IsOneWay();
  if (!isOutgoing || bidirectional)
    GetNeighboringEdge(road, rp, false /* forward */, edges);

  if (isOutgoing || bidirectional)
    GetNeighboringEdge(road, rp, true /* forward */, edges);
}

inline void IndexGraph::GetNeighboringEdge(RoadGeometry const & road, RoadPoint const & rp,
                                           bool forward, vector<JointEdge> & edges) const
{
  pair<Joint::Id, uint32_t> const & neighbor = m_roadIndex.FindNeighbor(rp, forward);
  if (neighbor.first != Joint::kInvalidId)
  {
    double const distance = m_estimator->CalcEdgesWeight(road, rp.GetPointId(), neighbor.second);
    edges.push_back({neighbor.first, distance});
  }
}

void IndexGraph::GetDirectedEdge(uint32_t featureId, uint32_t pointFrom, uint32_t pointTo,
                                 Joint::Id target, bool forward, vector<JointEdge> & edges)
{
  RoadGeometry const & road = m_geometry.GetRoad(featureId);
  if (!road.IsRoad())
    return;

  if (road.IsOneWay() && forward != (pointFrom < pointTo))
    return;

  double const distance = m_estimator->CalcEdgesWeight(road, pointFrom, pointTo);
  edges.emplace_back(target, distance);
}
}  // namespace routing
