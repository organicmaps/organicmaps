#pragma once

#include "routing/index_graph.hpp"
#include "routing/joint.hpp"

#include "std/utility.hpp"

namespace routing
{
// The problem:
// IndexGraph contains only road points connected in joints.
// So it is possible IndexGraph doesn't contain start and finish.
//
// IndexGraphStarter adds fake start and finish joint ids for AStarAlgorithm.
//
class IndexGraphStarter final
{
public:
  // AStarAlgorithm types aliases:
  using TVertexType = Joint::Id;
  using TEdgeType = JointEdge;

  IndexGraphStarter(IndexGraph & graph, RoadPoint const & startPoint,
                    RoadPoint const & finishPoint);

  IndexGraph & GetGraph() const { return m_graph; }
  Joint::Id GetStartJoint() const { return m_start.m_jointId; }
  Joint::Id GetFinishJoint() const { return m_finish.m_jointId; }

  m2::PointD const & GetPoint(Joint::Id jointId);

  void GetOutgoingEdgesList(Joint::Id jointId, vector<JointEdge> & edges)
  {
    GetEdgesList(jointId, true /* isOutgoing */, edges);
  }

  void GetIngoingEdgesList(Joint::Id jointId, vector<JointEdge> & edges)
  {
    GetEdgesList(jointId, false /* isOutgoing */, edges);
  }

  double HeuristicCostEstimate(Joint::Id from, Joint::Id to)
  {
    return m_graph.GetEstimator().CalcHeuristic(GetPoint(from), GetPoint(to));
  }

  // Add intermediate points to route (those don't correspond to any joint).
  //
  // Also convert joint ids to RoadPoints.
  void RedressRoute(vector<Joint::Id> const & route, vector<RoadPoint> & roadPoints);

private:
  struct FakeJoint final
  {
    FakeJoint(IndexGraph const & graph, RoadPoint const & point, Joint::Id fakeId);

    void SetupJointId(IndexGraph const & graph);
    bool BelongsToGraph() const { return m_jointId != m_fakeId; }

    RoadPoint const m_point;
    Joint::Id const m_fakeId;
    Joint::Id m_jointId;
  };

  template <typename F>
  void ForEachPoint(Joint::Id jointId, F && f) const
  {
    if (jointId == m_start.m_fakeId)
    {
      f(m_start.m_point);
      return;
    }

    if (jointId == m_finish.m_fakeId)
    {
      f(m_finish.m_point);
      return;
    }

    m_graph.ForEachPoint(jointId, forward<F>(f));
  }

  void GetEdgesList(Joint::Id jointId, bool isOutgoing, vector<JointEdge> & edges);
  void GetFakeEdges(FakeJoint const & from, FakeJoint const & to, bool isOutgoing,
                    vector<JointEdge> & edges);
  void GetArrivalFakeEdges(Joint::Id jointId, FakeJoint const & fakeJoint, bool isOutgoing,
                           vector<JointEdge> & edges);

  // Find two road points lying on the same feature.
  // If there are several pairs of such points, return pair with minimal distance.
  void FindPointsWithCommonFeature(Joint::Id jointId0, Joint::Id jointId1, RoadPoint & result0,
                                   RoadPoint & result1);

  IndexGraph & m_graph;
  FakeJoint m_start;
  FakeJoint m_finish;
};
}  // namespace routing
