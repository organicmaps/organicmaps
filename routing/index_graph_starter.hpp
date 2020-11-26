#pragma once

#include "routing/base/astar_graph.hpp"
#include "routing/base/astar_vertex_data.hpp"
#include "routing/base/routing_result.hpp"
#include "routing/fake_ending.hpp"
#include "routing/fake_feature_ids.hpp"
#include "routing/fake_graph.hpp"
#include "routing/fake_vertex.hpp"
#include "routing/guides_graph.hpp"
#include "routing/index_graph.hpp"
#include "routing/joint.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/regions_sparse_graph.hpp"
#include "routing/route_point.hpp"
#include "routing/route_weight.hpp"
#include "routing/segment.hpp"
#include "routing/world_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace routing
{
class FakeEdgesContainer;

// IndexGraphStarter adds fake start and finish vertices for AStarAlgorithm.
class IndexGraphStarter : public AStarGraph<IndexGraph::Vertex, IndexGraph::Edge, IndexGraph::Weight>
{
public:
  template <typename VertexType>
  using Parents = IndexGraph::Parents<VertexType>;

  friend class FakeEdgesContainer;

  static void CheckValidRoute(std::vector<Segment> const & segments);
  static size_t GetRouteNumPoints(std::vector<Segment> const & route);

  static bool IsFakeSegment(Segment const & segment)
  {
    return segment.GetFeatureId() == kFakeFeatureId;
  }

  static bool IsGuidesSegment(Segment const & segment)
  {
    return FakeFeatureIds::IsGuidesFeature(segment.GetFeatureId());
  }

  // strictForward flag specifies which parts of real segment should be placed from the start
  // vertex. true: place exactly one fake edge to the m_segment indicated with m_forward. false:
  // place two fake edges to the m_segment with both directions.
  IndexGraphStarter(FakeEnding const & startEnding, FakeEnding const & finishEnding,
                    uint32_t fakeNumerationStart, bool strictForward, WorldGraph & graph);

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
  LatLonWithAltitude const & GetRouteJunction(std::vector<Segment> const & route,
                                                       size_t pointIndex) const;
  ms::LatLon const & GetPoint(Segment const & segment, bool front) const;

  bool IsRoutingOptionsGood(Segment const & segment) const;
  RoutingOptions GetRoutingOptions(Segment const & segment) const;

  uint32_t GetNumFakeSegments() const
  {
    // Maximal number of fake segments in fake graph is numeric_limits<uint32_t>::max()
    // because segment idx type is uint32_t.
    CHECK_LESS_OR_EQUAL(m_fake.GetSize(), std::numeric_limits<uint32_t>::max(), ());
    return static_cast<uint32_t>(m_fake.GetSize());
  }

  std::set<NumMwmId> GetMwms() const;
  std::set<NumMwmId> GetStartMwms() const;
  std::set<NumMwmId> GetFinishMwms() const;

  // Checks whether |weight| meets non-pass-through crossing restrictions according to placement of
  // start and finish in pass-through/non-pass-through area and number of non-pass-through crosses.
  bool CheckLength(RouteWeight const & weight);

  void GetEdgeList(astar::VertexData<JointSegment, Weight> const & parentVertexData,
                   Segment const & segment, bool isOutgoing, std::vector<JointEdge> & edges,
                   std::vector<RouteWeight> & parentWeights) const
  {
    return m_graph.GetEdgeList(parentVertexData, segment, isOutgoing,
                               true /* useAccessConditional */, edges, parentWeights);
  }

  // AStarGraph overridings:
  // @{
  void GetOutgoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData,
                            std::vector<Edge> & edges) override
  {
    GetEdgesList(vertexData, true /* isOutgoing */, true /* useAccessConditional */, edges);
  }

  void GetIngoingEdgesList(astar::VertexData<Vertex, Weight> const & vertexData,
                           std::vector<Edge> & edges) override
  {
    GetEdgesList(vertexData, false /* isOutgoing */, true /* useAccessConditional */, edges);
  }

  RouteWeight HeuristicCostEstimate(Vertex const & from, Vertex const & to) override
  {
    return m_graph.HeuristicCostEstimate(GetPoint(from, true /* front */),
                                         GetPoint(to, true /* front */));
  }

  void SetAStarParents(bool forward, Parents<Segment> & parents) override
  {
    m_graph.SetAStarParents(forward, parents);
  }

  void DropAStarParents() override
  {
    m_graph.DropAStarParents();
  }

  bool AreWavesConnectible(Parents<Segment> & forwardParents, Vertex const & commonVertex,
                           Parents<Segment> & backwardParents) override
  {
    return m_graph.AreWavesConnectible(forwardParents, commonVertex, backwardParents, nullptr);
  }

  RouteWeight GetAStarWeightEpsilon() override;
  // @}

  void GetEdgesList(Vertex const & vertex, bool isOutgoing, std::vector<SegmentEdge> & edges) const
  {
    GetEdgesList({vertex, Weight(0.0)}, isOutgoing, false /* useAccessConditional */, edges);
  }

  RouteWeight HeuristicCostEstimate(Vertex const & from, ms::LatLon const & to) const
  {
    return m_graph.HeuristicCostEstimate(GetPoint(from, true /* front */), to);
  }

  RouteWeight CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) const;
  RouteWeight CalcGuidesSegmentWeight(Segment const & segment,
                                      EdgeEstimator::Purpose purpose) const;
  double CalculateETA(Segment const & from, Segment const & to) const;
  double CalculateETAWithoutPenalty(Segment const & segment) const;

  // For compatibility with IndexGraphStarterJoints
  // @{
  void SetAStarParents(bool forward, Parents<JointSegment> & parents)
  {
    m_graph.SetAStarParents(forward, parents);
  }

  bool AreWavesConnectible(Parents<JointSegment> & forwardParents, JointSegment const & commonVertex,
                           Parents<JointSegment> & backwardParents,
                           std::function<uint32_t(JointSegment const &)> && fakeFeatureConverter)
  {
    return m_graph.AreWavesConnectible(forwardParents, commonVertex, backwardParents,
                                       std::move(fakeFeatureConverter));
  }

  bool IsJoint(Segment const & segment, bool fromStart)
  {
    return GetGraph().GetIndexGraph(segment.GetMwmId()).IsJoint(segment.GetRoadPoint(fromStart));
  }

  bool IsJointOrEnd(Segment const & segment, bool fromStart)
  {
    return GetGraph().GetIndexGraph(segment.GetMwmId()).IsJointOrEnd(segment, fromStart);
  }
  // @}

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

  Ending const & GetStartEnding() const { return m_start; }
  Ending const & GetFinishEnding() const { return m_finish; }

  uint32_t GetFakeNumerationStart() const { return m_fakeNumerationStart; }

  // Creates fake edges for guides fake ending and adds them to the fake graph.
  void AddEnding(FakeEnding const & thisEnding);

  void ConnectLoopToGuideSegments(FakeVertex const & loop, Segment const & realSegment,
                                  LatLonWithAltitude realFrom, LatLonWithAltitude realTo,
                                  std::vector<std::pair<FakeVertex, Segment>> const & partsOfReal);

  ~IndexGraphStarter() override = default;

private:
  // Creates fake edges for fake ending and adds it to fake graph. |otherEnding| is used to
  // generate proper fake edges in case both endings have projections to the same segment.
  void AddEnding(FakeEnding const & thisEnding, FakeEnding const & otherEnding, bool isStart,
                 bool strictForward);

  static Segment GetFakeSegment(uint32_t segmentIdx)
  {
    // We currently ignore |isForward| and use FakeGraph to get ingoing/outgoing.
    // But all fake segments are oneway and placement of segment head and tail
    // correspond forward direction.
    return Segment(kFakeNumMwmId, kFakeFeatureId, segmentIdx, true /* isForward */);
  }

  Segment GetFakeSegmentAndIncr();

  void GetEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, bool isOutgoing,
                    bool useAccessConditional, std::vector<SegmentEdge> & edges) const;

  void AddStart(FakeEnding const & startEnding, FakeEnding const & finishEnding,
                bool strictForward);
  void AddFinish(FakeEnding const & finishEnding, FakeEnding const & startEnding);

  // Adds fake edges of type PartOfReal which correspond real edges from |edges| and are connected
  // to |segment|
  void AddFakeEdges(Segment const & segment, bool isOutgoing, std::vector<SegmentEdge> & edges) const;

  // Checks whether ending belongs to pass-through or non-pass-through zone.
  bool EndingPassThroughAllowed(Ending const & ending);
  // Start segment is located in a pass-through/non-pass-through area.
  bool StartPassThroughAllowed();
  // Finish segment is located in a pass-through/non-pass-through area.
  bool FinishPassThroughAllowed();

  static uint32_t constexpr kFakeFeatureId = FakeFeatureIds::kIndexGraphStarterId;
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
