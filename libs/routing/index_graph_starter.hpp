#pragma once

#include "routing/base/astar_graph.hpp"
#include "routing/base/astar_vertex_data.hpp"
#include "routing/fake_ending.hpp"
#include "routing/fake_feature_ids.hpp"
#include "routing/fake_graph.hpp"
#include "routing/fake_vertex.hpp"
#include "routing/guides_graph.hpp"
#include "routing/index_graph.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/route_weight.hpp"
#include "routing/segment.hpp"
#include "routing/world_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <memory>
#include <set>
#include <vector>

namespace routing
{
class FakeEdgesContainer;
class RegionsSparseGraph;

// Group highway types on categories (classes) to use in leaps candidates filtering.
enum class HighwayCategory : uint8_t
{
  // Do not change order!
  Major,
  Primary,
  Usual,
  Minor,
  Transit,
  Unknown
};

// IndexGraphStarter adds fake start and finish vertices for AStarAlgorithm.
class IndexGraphStarter : public AStarGraph<IndexGraph::Vertex, IndexGraph::Edge, IndexGraph::Weight>
{
public:
  template <typename VertexType>
  using Parents = IndexGraph::Parents<VertexType>;

  using JointEdgeListT = IndexGraph::JointEdgeListT;
  using WeightListT = IndexGraph::WeightListT;

  friend class FakeEdgesContainer;

  static void CheckValidRoute(std::vector<Segment> const & segments);

  static bool IsFakeSegment(Segment const & segment) { return segment.IsFakeCreated(); }

  static bool IsGuidesSegment(Segment const & segment)
  {
    return FakeFeatureIds::IsGuidesFeature(segment.GetFeatureId());
  }

  // strictForward flag specifies which parts of real segment should be placed from the start
  // vertex. true: place exactly one fake edge to the m_segment indicated with m_forward. false:
  // place two fake edges to the m_segment with both directions.
  IndexGraphStarter(FakeEnding const & startEnding, FakeEnding const & finishEnding, uint32_t fakeNumerationStart,
                    bool strictForward, WorldGraph & graph);

  void Append(FakeEdgesContainer const & container);

  void SetGuides(GuidesGraph const & guides);

  void SetRegionsGraphMode(std::shared_ptr<RegionsSparseGraph> regionsSparseGraph);
  bool IsRegionsGraphMode() const { return m_regionsGraph != nullptr; }

  WorldGraph & GetGraph() const { return m_graph; }
  WorldGraphMode GetMode() const { return m_graph.GetMode(); }
  LatLonWithAltitude const & GetStartJunction() const;
  LatLonWithAltitude const & GetFinishJunction() const;
  Segment GetStartSegment() const { return GetFakeSegment(m_start.m_id); }
  Segment GetFinishSegment() const { return GetFakeSegment(m_finish.m_id); }
  // If segment is real returns true and does not modify segment.
  // If segment is part of real converts it to real and returns true.
  // Otherwise returns false and does not modify segment.
  bool ConvertToReal(Segment & segment) const;
  LatLonWithAltitude const & GetJunction(Segment const & segment, bool front) const;
  LatLonWithAltitude const & GetRouteJunction(std::vector<Segment> const & route, size_t pointIndex) const;
  ms::LatLon const & GetPoint(Segment const & segment, bool front) const;

  bool IsRoutingOptionsGood(Segment const & segment) const;
  RoutingOptions GetRoutingOptions(Segment const & segment) const;

  uint32_t GetNumFakeSegments() const { return base::checked_cast<uint32_t>(m_fake.GetSize()); }

  std::set<NumMwmId> GetMwms() const;
  std::set<NumMwmId> const & GetStartMwms() const { return m_start.m_mwmIds; }
  std::set<NumMwmId> const & GetFinishMwms() const { return m_finish.m_mwmIds; }

  // Checks whether |weight| meets non-pass-through crossing restrictions according to placement of
  // start and finish in pass-through/non-pass-through area and number of non-pass-through crosses.
  bool CheckLength(RouteWeight const & weight);

  void GetEdgeList(astar::VertexData<JointSegment, Weight> const & parentVertexData, Segment const & segment,
                   bool isOutgoing, JointEdgeListT & edges, WeightListT & parentWeights) const
  {
    return m_graph.GetEdgeList(parentVertexData, segment, isOutgoing, true /* useAccessConditional */, edges,
                               parentWeights);
  }

  // AStarGraph overridings:
  // @{
  void GetOutgoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & edges) override
  {
    GetEdgesList(vertexData, true /* isOutgoing */, true /* useAccessConditional */, edges);
  }

  void GetIngoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, EdgeListT & edges) override
  {
    GetEdgesList(vertexData, false /* isOutgoing */, true /* useAccessConditional */, edges);
  }

  RouteWeight HeuristicCostEstimate(Vertex const & from, Vertex const & to) override
  {
    return m_graph.HeuristicCostEstimate(GetPoint(from, true /* front */), GetPoint(to, true /* front */));
  }

  void SetAStarParents(bool forward, Parents<Segment> & parents) override { m_graph.SetAStarParents(forward, parents); }

  void DropAStarParents() override { m_graph.DropAStarParents(); }

  bool AreWavesConnectible(Parents<Segment> & forwardParents, Vertex const & commonVertex,
                           Parents<Segment> & backwardParents) override
  {
    return m_graph.AreWavesConnectible(forwardParents, commonVertex, backwardParents);
  }

  RouteWeight GetAStarWeightEpsilon() override;
  // @}

  void GetEdgesList(Vertex const & vertex, bool isOutgoing, EdgeListT & edges) const
  {
    GetEdgesList({vertex, Weight(0.0)}, isOutgoing, false /* useAccessConditional */, edges);
  }

  RouteWeight HeuristicCostEstimate(Vertex const & from, ms::LatLon const & to) const
  {
    return m_graph.HeuristicCostEstimate(GetPoint(from, true /* front */), to);
  }

  RouteWeight CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) const;
  RouteWeight CalcGuidesSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) const;
  double CalculateETA(Segment const & from, Segment const & to) const;
  double CalculateETAWithoutPenalty(Segment const & segment) const;

  /// @name For compatibility with IndexGraphStarterJoints.
  /// @{
  void SetAStarParents(bool forward, Parents<JointSegment> & parents) { m_graph.SetAStarParents(forward, parents); }

  bool AreWavesConnectible(Parents<JointSegment> & forwardParents, JointSegment const & commonVertex,
                           Parents<JointSegment> & backwardParents,
                           WorldGraph::FakeConverterT const & fakeFeatureConverter)
  {
    return m_graph.AreWavesConnectible(forwardParents, commonVertex, backwardParents, fakeFeatureConverter);
  }

  bool IsJoint(Segment const & segment, bool fromStart)
  {
    return GetGraph().GetIndexGraph(segment.GetMwmId()).IsJoint(segment.GetRoadPoint(fromStart));
  }

  bool IsJointOrEnd(Segment const & segment, bool fromStart)
  {
    return GetGraph().GetIndexGraph(segment.GetMwmId()).IsJointOrEnd(segment, fromStart);
  }

  RouteWeight GetCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2)
  {
    return GetGraph().GetCrossBorderPenalty(mwmId1, mwmId2);
  }
  /// @}

  // Start or finish ending information.
  struct Ending
  {
    void FillMwmIds();
    // Fake segment id.
    uint32_t m_id = 0;
    // Real segments connected to the ending.
    std::set<Segment> m_real;
    // Mwm ids of connected segments to the ending.
    std::set<NumMwmId> m_mwmIds;
  };

  uint32_t GetFakeNumerationStart() const { return m_fakeNumerationStart; }

  // Creates fake edges for guides fake ending and adds them to the fake graph.
  void AddEnding(FakeEnding const & thisEnding);

  void ConnectLoopToGuideSegments(FakeVertex const & loop, Segment const & realSegment, LatLonWithAltitude realFrom,
                                  LatLonWithAltitude realTo,
                                  std::vector<std::pair<FakeVertex, Segment>> const & partsOfReal);

  HighwayCategory GetHighwayCategory(Segment seg) const;

private:
  // Creates fake edges for fake ending and adds it to fake graph. |otherEnding| is used to
  // generate proper fake edges in case both endings have projections to the same segment.
  void AddEnding(FakeEnding const & thisEnding, FakeEnding const & otherEnding, bool isStart, bool strictForward);

  static Segment GetFakeSegment(uint32_t segmentIdx)
  {
    // We currently ignore |isForward| and use FakeGraph to get ingoing/outgoing.
    // But all fake segments are oneway and placement of segment head and tail
    // correspond forward direction.
    return Segment(kFakeNumMwmId, FakeFeatureIds::kIndexGraphStarterId, segmentIdx, true /* isForward */);
  }

  Segment GetFakeSegmentAndIncr();

  void GetEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, bool isOutgoing, bool useAccessConditional,
                    EdgeListT & edges) const;

  void AddStart(FakeEnding const & startEnding, FakeEnding const & finishEnding, bool strictForward);
  void AddFinish(FakeEnding const & finishEnding, FakeEnding const & startEnding);

  // Adds fake edges of type PartOfReal which correspond real edges from |edges| and are connected
  // to |segment|
  void AddFakeEdges(Segment const & segment, bool isOutgoing, EdgeListT & edges) const;

  // Checks whether ending belongs to non-pass-through zone (service, living street, etc).
  bool HasNoPassThroughAllowed(Ending const & ending) const;

  WorldGraph & m_graph;
  // Start segment id
  Ending m_start;
  // Finish segment id
  Ending m_finish;
  double m_startToFinishDistanceM;
  FakeGraph m_fake;
  GuidesGraph m_guides;
  uint32_t m_fakeNumerationStart;

  std::vector<FakeEnding> m_otherEndings;

  // Field for routing in mode for finding all route mwms.
  std::shared_ptr<RegionsSparseGraph> m_regionsGraph = nullptr;
};
}  // namespace routing
