#include "routing/index_graph_starter.hpp"

namespace routing
{
IndexGraphStarter::IndexGraphStarter(IndexGraph const & graph, RoadPoint startPoint,
                                     RoadPoint finishPoint)
  : m_graph(graph)
  , m_startPoint(startPoint)
  , m_finishPoint(finishPoint)
  , m_startImplant(graph.GetNumJoints())
  , m_finishImplant(graph.GetNumJoints() + 1)
  , m_startJoint(CalcStartJoint())
  , m_finishJoint(CalcFinishJoint())
{
}

Joint::Id IndexGraphStarter::CalcStartJoint() const
{
  Joint::Id const jointId = m_graph.GetJointId(m_startPoint);
  if (jointId == Joint::kInvalidId)
    return m_startImplant;

  return jointId;
}

Joint::Id IndexGraphStarter::CalcFinishJoint() const
{
  if (m_startPoint == m_finishPoint)
    return CalcStartJoint();

  Joint::Id const jointId = m_graph.GetJointId(m_finishPoint);
  if (jointId == Joint::kInvalidId)
    return m_finishImplant;

  return jointId;
}

void IndexGraphStarter::GetEdgesList(Joint::Id jointId, bool isOutgoing,
                                     vector<JointEdge> & edges) const
{
  edges.clear();

  if (jointId == m_startImplant)
  {
    m_graph.AddNeighboringEdges(m_startPoint, isOutgoing, edges);

    if (FinishIsImplant() && m_startPoint.GetFeatureId() == m_finishPoint.GetFeatureId())
      m_graph.AddDirectEdge(m_startPoint.GetFeatureId(), m_startPoint.GetPointId(),
                            m_finishPoint.GetPointId(), m_finishJoint, isOutgoing, edges);

    return;
  }

  if (jointId == m_finishImplant)
  {
    m_graph.AddNeighboringEdges(m_finishPoint, isOutgoing, edges);

    if (StartIsImplant() && m_startPoint.GetFeatureId() == m_finishPoint.GetFeatureId())
      m_graph.AddDirectEdge(m_finishPoint.GetFeatureId(), m_finishPoint.GetPointId(),
                            m_startPoint.GetPointId(), m_startJoint, isOutgoing, edges);

    return;
  }

  m_graph.GetEdgesList(jointId, isOutgoing, edges);

  if (StartIsImplant() && m_graph.JointLaysOnRoad(jointId, m_startPoint.GetFeatureId()))
  {
    vector<JointEdge> startEdges;
    m_graph.AddNeighboringEdges(m_startPoint, !isOutgoing, startEdges);
    for (JointEdge const & edge : startEdges)
    {
      if (edge.GetTarget() == jointId)
        edges.emplace_back(m_startJoint, edge.GetWeight());
    }
  }

  if (FinishIsImplant() && m_graph.JointLaysOnRoad(jointId, m_finishPoint.GetFeatureId()))
  {
    vector<JointEdge> finishEdges;
    m_graph.AddNeighboringEdges(m_finishPoint, !isOutgoing, finishEdges);
    for (JointEdge const & edge : finishEdges)
    {
      if (edge.GetTarget() == jointId)
        edges.emplace_back(m_finishJoint, edge.GetWeight());
    }
  }
}

void IndexGraphStarter::RedressRoute(vector<Joint::Id> const & route,
                                     vector<RoadPoint> & roadPoints) const
{
  if (route.size() < 2)
  {
    if (route.size() == 1)
      roadPoints.emplace_back(m_startPoint);
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
      MYTHROW(RootException,
              ("Wrong equality pointFrom = pointTo =", pointFrom, ", featureId = ", featureId));
    }

    roadPoints.emplace_back(rp1);
  }
}

void IndexGraphStarter::FindPointsWithCommonFeature(Joint::Id jointId0, Joint::Id jointId1,
                                                    RoadPoint & result0, RoadPoint & result1) const
{
  bool found = false;
  double minWeight = -1.0;

  ForEachPoint(jointId0, [&](RoadPoint const & rp0) {
    ForEachPoint(jointId1, [&](RoadPoint const & rp1) {
      if (rp0.GetFeatureId() == rp1.GetFeatureId())
      {
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
      }
    });
  });

  if (!found)
    MYTHROW(RootException, ("Can't find common feature for joints", jointId0, jointId1));
}
}  // namespace routing
