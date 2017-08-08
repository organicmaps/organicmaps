#pragma once

#include "routing/index_graph.hpp"
#include "routing/joint.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/route_point.hpp"
#include "routing/world_graph.hpp"

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

  struct FakeEnding
  {
    Junction m_originJunction;
    std::vector<Junction> m_projectionJunctions;
    std::vector<Segment> m_projectionSegments;
  };

  friend class FakeEdgesContainer;

  static uint32_t constexpr kFakeFeatureId = numeric_limits<uint32_t>::max();

  static FakeEnding MakeFakeEnding(Segment const & segment, m2::PointD const & point,
                                   WorldGraph & graph);
  static void CheckValidRoute(std::vector<Segment> const & segments);
  static size_t GetRouteNumPoints(std::vector<Segment> const & route);

  // strictForward flag specifies which parts of real segment should be placed from the start
  // vertex. true: place exactly one fake edge to the m_segment with indicated m_forward. false:
  // place two fake edges to the m_segment with both directions.
  IndexGraphStarter(FakeEnding const & startEnding, FakeEnding const & finishEnding,
                    uint32_t fakeNumerationStart, bool strictForward, WorldGraph & graph);

  // Merge starters to single IndexGraphStarter.
  // Expects starters[0] has WorldGraph which is valid for all starters, starters.size() >= 1.
  IndexGraphStarter(std::vector<IndexGraphStarter> starters);

  void Append(FakeEdgesContainer const & container);

  WorldGraph & GetGraph() { return m_graph; }
  Junction const & GetStartJunction() const;
  Junction const & GetFinishJunction() const;
  Segment GetStartSegment() const { return GetFakeSegment(m_startId); }
  Segment GetFinishSegment() const { return GetFakeSegment(m_finishId); }
  // If segment is real return true and does not modify segment.
  // If segment is part of real converts it to real and returns true.
  // Otherwise returns false and does not modify segment.
  bool ConvertToReal(Segment & segment) const;
  Junction const & GetJunction(Segment const & segment, bool front) const;
  Junction const & GetRouteJunction(std::vector<Segment> const & route, size_t pointIndex) const;
  m2::PointD const & GetPoint(Segment const & segment, bool front) const;
  size_t GetNumFakeSegments() const { return m_fake.m_segmentToVertex.size(); }

  std::set<NumMwmId> GetMwms() const;

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
    return RouteWeight(m_graph.GetEstimator().CalcHeuristic(GetPoint(from, true /* front */),
                                                            GetPoint(to, true /* front */)));
  }

  double CalcSegmentWeight(Segment const & segment) const;
  double CalcRouteSegmentWeight(std::vector<Segment> const & route, size_t segmentIndex) const;

  bool IsLeap(NumMwmId mwmId) const;

  static bool IsFakeSegment(Segment const & segment)
  {
    return segment.GetFeatureId() == kFakeFeatureId;
  }

private:
  static Segment GetFakeSegment(uint32_t i)
  {
    return Segment(kFakeNumMwmId, kFakeFeatureId, i, false);
  }

  class FakeVertex final
  {
  public:
    enum class Type
    {
      PureFake,
      Projection,
      PartOfReal,
    };
    // For unit tests only.
    FakeVertex(NumMwmId mwmId, uint32_t featureId, uint32_t segmentIdx, m2::PointD const & point)
      : m_junctionFrom(point, feature::kDefaultAltitudeMeters)
      , m_junctionTo(point, feature::kDefaultAltitudeMeters)
    {
    }

    FakeVertex(Junction const & junctionFrom, Junction const & junctionTo, Type type)
      : m_junctionFrom(junctionFrom), m_junctionTo(junctionTo), m_type(type)
    {
    }

    FakeVertex(FakeVertex const &) = default;
    FakeVertex() = default;

    bool operator==(FakeVertex const & rhs) const
    {
      return m_junctionFrom == rhs.m_junctionFrom && m_junctionTo == rhs.m_junctionTo &&
             m_type == rhs.m_type;
    }

    Type GetType() const { return m_type; }

    Junction const & GetJunctionFrom() const { return m_junctionFrom; }
    m2::PointD const & GetPointFrom() const { return m_junctionFrom.GetPoint(); }
    Junction const & GetJunctionTo() const { return m_junctionTo; }
    m2::PointD const & GetPointTo() const { return m_junctionTo.GetPoint(); }

  private:
    Junction m_junctionFrom;
    Junction m_junctionTo;
    Type m_type = Type::PureFake;
  };

  struct FakeGraph
  {
    std::map<Segment, std::set<Segment>> m_outgoing;
    std::map<Segment, std::set<Segment>> m_ingoing;
    std::map<Segment, FakeVertex> m_segmentToVertex;
    std::map<Segment, Segment> m_fakeToReal;
    std::map<Segment, std::set<Segment>> m_realToFake;
  };

  void AddFakeVertex(Segment const & existentSegment, Segment const & newSegment,
                     FakeVertex const & newVertex, bool isOutgoing, bool isPartOfReal,
                     Segment const & realSegment);
  void AddEnding(FakeEnding const & thisEnding, FakeEnding const & otherEnding, bool isStart,
                 uint32_t & fakeNumerationStart, bool strictForward);
  void AddStart(FakeEnding const & startEnding, FakeEnding const & finishEnding,
                uint32_t & fakeNumerationStart, bool strictForward);
  void AddFinish(FakeEnding const & finishEnding, FakeEnding const & startEnding,
                 uint32_t & fakeNumerationStart);

  // Returns existent segment if possible. If segment does not exist makes new segment,
  // increments newNumber.
  Segment GetSegment(FakeVertex const & vertex, uint32_t & newNumber) const;

  void AddFakeEdges(vector<SegmentEdge> & edges) const;
  void AddRealEdges(Segment const & segment, bool isOutgoing,
                    std::vector<SegmentEdge> & edges) const;
  /// \brief If |isOutgoing| == true fills |edges| with SegmentEdge(s) which connects
  /// |fakeVertex| with all exits of mwm.
  /// \brief If |isOutgoing| == false fills |edges| with SegmentEdge(s) which connects
  /// all enters to mwm with |fakeVertex|.
  void ConnectLeapToTransitions(Segment const & segment, bool isOutgoing,
                                std::vector<SegmentEdge> & edges) const;

  WorldGraph & m_graph;
  uint32_t m_startId;
  uint32_t m_finishId;
  FakeGraph m_fake;
};

class FakeEdgesContainer final
{
  friend class IndexGraphStarter;

public:
  FakeEdgesContainer(IndexGraphStarter && starter)
    : m_finishId(move(starter.m_finishId)), m_fake(move(starter.m_fake))
  {
  }

  size_t GetNumFakeEdges() const { return m_fake.m_segmentToVertex.size(); }

private:
  uint32_t m_finishId;
  IndexGraphStarter::FakeGraph m_fake;
};
}  // namespace routing
