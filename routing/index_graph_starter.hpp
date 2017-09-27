#pragma once

#include "routing/base/routing_result.hpp"
#include "routing/index_graph.hpp"
#include "routing/joint.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/route_point.hpp"
#include "routing/world_graph.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace routing
{
class FakeEdgesContainer;

// IndexGraphStarter adds fake start and finish vertexes for AStarAlgorithm.
class IndexGraphStarter final
{
public:
  // AStarAlgorithm types aliases:
  using TVertexType = IndexGraph::TVertexType;
  using TEdgeType = IndexGraph::TEdgeType;
  using TWeightType = IndexGraph::TWeightType;

  struct Projection final
  {
    Segment m_segment;
    Junction m_junction;
  };

  struct FakeEnding final
  {
    Junction m_originJunction;
    std::vector<Projection> m_projections;
  };

  friend class FakeEdgesContainer;

  static uint32_t constexpr kFakeFeatureId = std::numeric_limits<uint32_t>::max();

  static FakeEnding MakeFakeEnding(Segment const & segment, m2::PointD const & point,
                                   WorldGraph & graph);
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
    CHECK_LESS_OR_EQUAL(m_fake.m_segmentToVertex.size(), numeric_limits<uint32_t>::max(), ());
    return static_cast<uint32_t>(m_fake.m_segmentToVertex.size());
  }

  std::set<NumMwmId> GetMwms() const;

  // Checks |result| meets nontransit crossing restrictions according to placement of
  // |result.path| start and finish in transit/nontransit area and number of nontransit crosses
  bool DoesRouteCrossNontransit(
      RoutingResult<Segment, RouteWeight> const & result) const;

  void GetEdgesList(Segment const & segment, bool isOutgoing,
                    std::vector<SegmentEdge> & edges) const;

  void GetOutgoingEdgesList(TVertexType const & segment, std::vector<TEdgeType> & edges) const
  {
    GetEdgesList(segment, true /* isOutgoing */, edges);
  }

  void GetIngoingEdgesList(TVertexType const & segment, std::vector<TEdgeType> & edges) const
  {
    GetEdgesList(segment, false /* isOutgoing */, edges);
  }

  RouteWeight HeuristicCostEstimate(TVertexType const & from, TVertexType const & to) const
  {
    return m_graph.HeuristicCostEstimate(GetPoint(from, true /* front */),
                                         GetPoint(to, true /* front */));
  }

  RouteWeight CalcSegmentWeight(Segment const & segment) const;
  RouteWeight CalcRouteSegmentWeight(std::vector<Segment> const & route, size_t segmentIndex) const;

  bool IsLeap(NumMwmId mwmId) const;

private:
  class FakeVertex final
  {
  public:
    enum class Type
    {
      PureFake,
      PartOfReal,
    };

    // For unit tests only.
    FakeVertex(NumMwmId mwmId, uint32_t featureId, uint32_t segmentIdx, m2::PointD const & point)
      : m_from(point, feature::kDefaultAltitudeMeters), m_to(point, feature::kDefaultAltitudeMeters)
    {
    }

    FakeVertex(Junction const & from, Junction const & to, Type type)
      : m_from(from), m_to(to), m_type(type)
    {
    }

    FakeVertex(FakeVertex const &) = default;
    FakeVertex() = default;

    bool operator==(FakeVertex const & rhs) const
    {
      return m_from == rhs.m_from && m_to == rhs.m_to && m_type == rhs.m_type;
    }

    Type GetType() const { return m_type; }

    Junction const & GetJunctionFrom() const { return m_from; }
    m2::PointD const & GetPointFrom() const { return m_from.GetPoint(); }
    Junction const & GetJunctionTo() const { return m_to; }
    m2::PointD const & GetPointTo() const { return m_to.GetPoint(); }

  private:
    Junction m_from;
    Junction m_to;
    Type m_type = Type::PureFake;
  };

  struct FakeGraph final
  {
    // Adds vertex. Connects newSegment to existentSegment. Adds ingoing and
    // outgoing edges, fills segment to vertex mapping. Fill real to fake and fake to real
    // mapping if isPartOfReal is true.
    void AddFakeVertex(Segment const & existentSegment, Segment const & newSegment,
                       FakeVertex const & newVertex, bool isOutgoing, bool isPartOfReal,
                       Segment const & realSegment);
    // Returns existent segment if possible. If segment does not exist makes new segment,
    // increments newNumber.
    Segment GetSegment(FakeVertex const & vertex, uint32_t & newNumber) const;
    void Append(FakeGraph const & rhs);

    // Key is fake segment, value is set of outgoing fake segments
    std::map<Segment, std::set<Segment>> m_outgoing;
    // Key is fake segment, value is set of ingoing fake segments
    std::map<Segment, std::set<Segment>> m_ingoing;
    // Key is fake segment, value is fake vertex which corresponds fake segment
    std::map<Segment, FakeVertex> m_segmentToVertex;
    // Key is fake segment of type FakeVertex::Type::PartOfReal, value is corresponding real segment
    std::map<Segment, Segment> m_fakeToReal;
    // Key is real segment, value is set of fake segments with type FakeVertex::Type::PartOfReal
    // which are parts of this real segment
    std::map<Segment, std::set<Segment>> m_realToFake;
  };

  static Segment GetFakeSegment(uint32_t segmentIdx)
  {
    return Segment(kFakeNumMwmId, kFakeFeatureId, segmentIdx, false);
  }

  static Segment GetFakeSegmentAndIncr(uint32_t & segmentIdx)
  {
    CHECK_LESS(segmentIdx, numeric_limits<uint32_t>::max(), ());
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

  WorldGraph & m_graph;
  // Start segment id
  uint32_t m_startId;
  // Finish segment id
  uint32_t m_finishId;
  FakeGraph m_fake;
};
}  // namespace routing
