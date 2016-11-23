#pragma once

#include "routing/index_graph.hpp"
#include "routing/joint.hpp"

namespace routing
{
// The problem:
// IndexGraph contains only road points connected in joints.
// So it is possible IndexGraph doesn't contain start and finish.
//
// IndexGraphStarter implants start and finish for AStarAlgorithm.
//
class IndexGraphStarter final
{
public:
  // AStarAlgorithm types aliases:
  using TVertexType = Joint::Id;
  using TEdgeType = JointEdge;

  IndexGraphStarter(IndexGraph const & graph, RoadPoint startPoint, RoadPoint finishPoint);

  IndexGraph const & GetGraph() const { return m_graph; }
  Joint::Id GetStartJoint() const { return m_startJoint; }
  Joint::Id GetFinishJoint() const { return m_finishJoint; }

  m2::PointD const & GetPoint(Joint::Id jointId) const
  {
    if (jointId == m_startImplant)
      return m_graph.GetGeometry().GetPoint(m_startPoint);

    if (jointId == m_finishImplant)
      return m_graph.GetGeometry().GetPoint(m_finishPoint);

    return m_graph.GetPoint(jointId);
  }

  void GetOutgoingEdgesList(Joint::Id jointId, vector<JointEdge> & edges) const
  {
    GetEdgesList(jointId, true, edges);
  }

  void GetIngoingEdgesList(Joint::Id jointId, vector<JointEdge> & edges) const
  {
    GetEdgesList(jointId, false, edges);
  }

  void GetEdgesList(Joint::Id jointId, bool isOutgoing, vector<JointEdge> & edges) const;

  double HeuristicCostEstimate(Joint::Id from, Joint::Id to) const
  {
    return m_graph.CalcHeuristic(GetPoint(from), GetPoint(to));
  }

  // Add intermediate points to route (those don't correspond to any joint).
  //
  // Also convert joint ids to RoadPoints.
  void RedressRoute(vector<Joint::Id> const & route, vector<RoadPoint> & roadPoints) const;

private:
  Joint::Id CalcStartJoint() const;
  Joint::Id CalcFinishJoint() const;

  bool StartIsImplant() const { return m_startJoint == m_startImplant; }
  bool FinishIsImplant() const { return m_finishJoint == m_finishImplant; }

  template <typename F>
  void ForEachPoint(Joint::Id jointId, F && f) const
  {
    if (jointId == m_startImplant)
    {
      f(m_startPoint);
      return;
    }

    if (jointId == m_finishImplant)
    {
      f(m_finishPoint);
      return;
    }

    m_graph.ForEachPoint(jointId, f);
  }

  void FindPointsWithCommonFeature(Joint::Id jointId0, Joint::Id jointId1, RoadPoint & result0,
                                   RoadPoint & result1) const;

  IndexGraph const & m_graph;
  RoadPoint const m_startPoint;
  RoadPoint const m_finishPoint;
  Joint::Id const m_startImplant;
  Joint::Id const m_finishImplant;
  Joint::Id const m_startJoint;
  Joint::Id const m_finishJoint;
};
}  // namespace routing
