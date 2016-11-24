#include "routing/index_graph_starter.hpp"

#include "routing/routing_exception.hpp"

namespace routing
{
IndexGraphStarter::IndexGraphStarter(IndexGraph & graph, RoadPoint const & startPoint,
                                     RoadPoint const & finishPoint)
  : m_graph(graph)
  , m_start(graph, startPoint, graph.GetNumJoints())
  , m_finish(graph, finishPoint, graph.GetNumJoints() + 1)
{
  m_start.SetupJointId(graph);

  if (startPoint == finishPoint)
    m_finish.m_jointId = m_start.m_jointId;
  else
    m_finish.SetupJointId(graph);
}

m2::PointD const & IndexGraphStarter::GetPoint(Joint::Id jointId)
{
  if (jointId == m_start.m_fakeId)
    return m_graph.GetGeometry().GetPoint(m_start.m_point);

  if (jointId == m_finish.m_fakeId)
    return m_graph.GetGeometry().GetPoint(m_finish.m_point);

  return m_graph.GetPoint(jointId);
}

void IndexGraphStarter::RedressRoute(vector<Joint::Id> const & route,
                                     vector<RoadPoint> & roadPoints)
{
  if (route.size() < 2)
  {
    if (route.size() == 1)
      roadPoints.emplace_back(m_start.m_point);
    return;
  }

  roadPoints.reserve(route.size() * 2);

  for (size_t i = 0; i < route.size() - 1; ++i)
  {
    Joint::Id const prevJoint = route[i];
    Joint::Id const nextJoint = route[i + 1];

    RoadPoint rp0;
    RoadPoint rp1;
    FindPointsWithCommonFeature(prevJoint, nextJoint, rp0, rp1);
    if (i == 0)
      roadPoints.emplace_back(rp0);

    uint32_t const featureId = rp0.GetFeatureId();
    uint32_t const pointFrom = rp0.GetPointId();
    uint32_t const pointTo = rp1.GetPointId();

    if (pointFrom < pointTo)
    {
      for (uint32_t pointId = pointFrom + 1; pointId < pointTo; ++pointId)
        roadPoints.emplace_back(featureId, pointId);
    }
    else if (pointFrom > pointTo)
    {
      for (uint32_t pointId = pointFrom - 1; pointId > pointTo; --pointId)
        roadPoints.emplace_back(featureId, pointId);
    }
    else
    {
      CHECK(false,
            ("Wrong equality pointFrom = pointTo =", pointFrom, ", featureId = ", featureId));
    }

    roadPoints.emplace_back(rp1);
  }
}

void IndexGraphStarter::GetEdgesList(Joint::Id jointId, bool isOutgoing, vector<JointEdge> & edges)
{
  edges.clear();

  if (jointId == m_start.m_fakeId)
  {
    GetFakeEdges(m_start, m_finish, isOutgoing, edges);
    return;
  }

  if (jointId == m_finish.m_fakeId)
  {
    GetFakeEdges(m_finish, m_start, isOutgoing, edges);
    return;
  }

  m_graph.GetEdgeList(jointId, isOutgoing, edges);
  GetArrivalFakeEdges(jointId, m_start, isOutgoing, edges);
  GetArrivalFakeEdges(jointId, m_finish, isOutgoing, edges);
}

void IndexGraphStarter::GetFakeEdges(IndexGraphStarter::FakeJoint const & from,
                                     IndexGraphStarter::FakeJoint const & to, bool isOutgoing,
                                     vector<JointEdge> & edges)
{
  m_graph.GetNeighboringEdges(from.m_point, isOutgoing, edges);

  if (!to.BelongsToGraph() && from.m_point.GetFeatureId() == to.m_point.GetFeatureId())
  {
    m_graph.GetDirectedEdge(from.m_point.GetFeatureId(), from.m_point.GetPointId(),
                            to.m_point.GetPointId(), to.m_jointId, isOutgoing, edges);
  }
}

void IndexGraphStarter::GetArrivalFakeEdges(Joint::Id jointId,
                                            IndexGraphStarter::FakeJoint const & fakeJoint,
                                            bool isOutgoing, vector<JointEdge> & edges)
{
  if (fakeJoint.BelongsToGraph())
    return;

  if (!m_graph.JointLiesOnRoad(jointId, fakeJoint.m_point.GetFeatureId()))
    return;

  vector<JointEdge> startEdges;
  m_graph.GetNeighboringEdges(fakeJoint.m_point, !isOutgoing, startEdges);
  for (JointEdge const & edge : startEdges)
  {
    if (edge.GetTarget() == jointId)
      edges.emplace_back(fakeJoint.m_jointId, edge.GetWeight());
  }
}

void IndexGraphStarter::FindPointsWithCommonFeature(Joint::Id jointId0, Joint::Id jointId1,
                                                    RoadPoint & result0, RoadPoint & result1)
{
  bool found = false;
  double minWeight = -1.0;

  ForEachPoint(jointId0, [&](RoadPoint const & rp0) {
    ForEachPoint(jointId1, [&](RoadPoint const & rp1) {
      if (rp0.GetFeatureId() != rp1.GetFeatureId())
        return;

      RoadGeometry const & road = m_graph.GetGeometry().GetRoad(rp0.GetFeatureId());
      if (!road.IsRoad())
        return;

      if (road.IsOneWay() && rp0.GetPointId() > rp1.GetPointId())
        return;

      if (found)
      {
        if (minWeight < 0.0)
        {
          // CalcEdgesWeight is very expensive.
          // So calculate it only if second common feature found.
          RoadGeometry const & prevRoad = m_graph.GetGeometry().GetRoad(result0.GetFeatureId());
          minWeight = m_graph.GetEstimator().CalcEdgesWeight(prevRoad, result0.GetPointId(),
                                                             result1.GetPointId());
        }

        double const weight =
            m_graph.GetEstimator().CalcEdgesWeight(road, rp0.GetPointId(), rp1.GetPointId());
        if (weight < minWeight)
        {
          minWeight = weight;
          result0 = rp0;
          result1 = rp1;
        }
      }
      else
      {
        result0 = rp0;
        result1 = rp1;
        found = true;
      }
    });
  });

  CHECK(found, ("Can't find common feature for joints", jointId0, jointId1));
}

// IndexGraphStarter::FakeJoint --------------------------------------------------------------------
IndexGraphStarter::FakeJoint::FakeJoint(IndexGraph const & graph, RoadPoint const & point,
                                        Joint::Id fakeId)
  : m_point(point), m_fakeId(fakeId), m_jointId(Joint::kInvalidId)
{
}

void IndexGraphStarter::FakeJoint::SetupJointId(IndexGraph const & graph)
{
  m_jointId = graph.GetJointId(m_point);
  if (m_jointId == Joint::kInvalidId)
    m_jointId = m_fakeId;
}
}  // namespace routing
