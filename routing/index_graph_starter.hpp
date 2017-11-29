#pragma once

#include "routing/base/routing_result.hpp"
#include "routing/fake_ending.hpp"
#include "routing/fake_feature_ids.hpp"
#include "routing/fake_graph.hpp"
#include "routing/fake_vertex.hpp"
#include "routing/index_graph.hpp"
#include "routing/joint.hpp"
#include "routing/route_point.hpp"
#include "routing/route_weight.hpp"
#include "routing/segment.hpp"
#include "routing/world_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <utility>
#include <vector>

namespace routing
{
class FakeEdgesContainer;

// IndexGraphStarter adds fake start and finish vertices for AStarAlgorithm.
class IndexGraphStarter final
{
public:
  // AStarAlgorithm types aliases:
  using Vertex = IndexGraph::Vertex;
  using Edge = IndexGraph::Edge;
  using Weight = IndexGraph::Weight;

  friend class FakeEdgesContainer;

  static void CheckValidRoute(std::vector<Segment> const & segments);
  static size_t GetRouteNumPoints(std::vector<Segment> const & route);

  static bool IsFakeSegment(Segment const & segment)
  {
    return segment.GetFeatureId() == kFakeFeatureId;
  }

  // strictForward flag specifies which parts of real segment should be placed from the start
  // vertex. true: place exactly one fake edge to the m_segment with indicated m_forward. false:
  // place two fake edges to the m_segment with both directions.
  IndexGraphStarter(FakeEnding const & startEnding, FakeEnding const & finishEnding,
                    uint32_t fakeNumerationStart, bool strictForward, WorldGraph & graph);

  void Append(FakeEdgesContainer const & container);

  WorldGraph & GetGraph() const { return m_graph; }
  Junction const & GetStartJunction() const;
  Junction const & GetFinishJunction() const;
  Segment GetStartSegment() const { return GetFakeSegment(m_startId); }
  Segment GetFinishSegment() const { return GetFakeSegment(m_finishId); }
  // If segment is real returns true and does not modify segment.
  // If segment is part of real converts it to real and returns true.
  // Otherwise returns false and does not modify segment.
  bool ConvertToReal(Segment & segment) const;
  Junction const & GetJunction(Segment const & segment, bool front) const;
  Junction const & GetRouteJunction(std::vector<Segment> const & route, size_t pointIndex) const;
  m2::PointD const & GetPoint(Segment const & segment, bool front) const;
  uint32_t GetNumFakeSegments() const
  {
    // Maximal number of fake segments in fake graph is numeric_limits<uint32_t>::max()
    // because segment idx type is uint32_t.
    CHECK_LESS_OR_EQUAL(m_fake.GetSize(), std::numeric_limits<uint32_t>::max(), ());
    return static_cast<uint32_t>(m_fake.GetSize());
  }

  std::set<NumMwmId> GetMwms() const;

  // Checks whether |weight| meets non-pass-through crossing restrictions according to placement of
  // start and finish in pass-through/non-pass-through area and number of non-pass-through crosses.
  bool CheckLength(RouteWeight const & weight) const;

  void GetEdgesList(Segment const & segment, bool isOutgoing,
                    std::vector<SegmentEdge> & edges) const;

  void GetOutgoingEdgesList(Vertex const & segment, std::vector<Edge> & edges) const
  {
    GetEdgesList(segment, true /* isOutgoing */, edges);
  }

  void GetIngoingEdgesList(Vertex const & segment, std::vector<Edge> & edges) const
  {
    GetEdgesList(segment, false /* isOutgoing */, edges);
  }

  RouteWeight HeuristicCostEstimate(Vertex const & from, Vertex const & to) const
  {
    return m_graph.HeuristicCostEstimate(GetPoint(from, true /* front */),
                                         GetPoint(to, true /* front */));
  }

  RouteWeight CalcSegmentWeight(Segment const & segment) const;
  RouteWeight CalcRouteSegmentWeight(std::vector<Segment> const & route, size_t segmentIndex) const;

  bool IsLeap(NumMwmId mwmId) const;

private:
  static Segment GetFakeSegment(uint32_t segmentIdx)
  {
    return Segment(kFakeNumMwmId, kFakeFeatureId, segmentIdx, false);
  }

  static Segment GetFakeSegmentAndIncr(uint32_t & segmentIdx)
  {
    CHECK_LESS(segmentIdx, std::numeric_limits<uint32_t>::max(), ());
    return GetFakeSegment(segmentIdx++);
  }

  // Creates fake edges for fake ending and adds it to  fake graph. |otherEnding| used to generate
  // propper fake edges in case both endings have projections to the same segment.
  void AddEnding(FakeEnding const & thisEnding, FakeEnding const & otherEnding, bool isStart,
                 bool strictForward, uint32_t & fakeNumerationStart);
  void AddStart(FakeEnding const & startEnding, FakeEnding const & finishEnding, bool strictForward,
                uint32_t & fakeNumerationStart);
  void AddFinish(FakeEnding const & finishEnding, FakeEnding const & startEnding,
                 uint32_t & fakeNumerationStart);

  // Adds fake edges of type PartOfReal which correspond real edges from |edges| and are connected
  // to |segment|
  void AddFakeEdges(Segment const & segment, vector<SegmentEdge> & edges) const;
  // Adds real edges from m_graph
  void AddRealEdges(Segment const & segment, bool isOutgoing,
                    std::vector<SegmentEdge> & edges) const;

  // Checks whether ending belongs to pass-through or non-pass-through zone.
  bool EndingPassThroughAllowed(FakeEnding const & ending);

  static uint32_t constexpr kFakeFeatureId = FakeFeatureIds::kIndexGraphStarterId;
  WorldGraph & m_graph;
  // Start segment id
  uint32_t m_startId;
  // Finish segment id
  uint32_t m_finishId;
  // Start segment is located in a pass-through/non-pass-through area.
  bool m_startPassThroughAllowed;
  // Finish segment is located in a pass-through/non-pass-through area.
  bool m_finishPassThroughAllowed;
  double m_startToFinishDistanceM;
  FakeGraph<Segment, FakeVertex, Segment> m_fake;
};
}  // namespace routing
