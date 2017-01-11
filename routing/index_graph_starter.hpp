#pragma once

#include "routing/index_graph.hpp"
#include "routing/joint.hpp"
#include "routing/route_point.hpp"

#include "std/limits.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace routing
{
// IndexGraphStarter adds fake start and finish vertexes for AStarAlgorithm.
class IndexGraphStarter final
{
public:
  // AStarAlgorithm types aliases:
  using TVertexType = Segment;
  using TEdgeType = SegmentEdge;

  class FakeVertex final
  {
  public:
    FakeVertex(uint32_t featureId, uint32_t segmentIdx, m2::PointD const & point)
      : m_featureId(featureId), m_segmentIdx(segmentIdx), m_point(point)
    {
    }

    uint32_t GetFeatureId() const { return m_featureId; }
    uint32_t GetSegmentIdx() const { return m_segmentIdx; }
    m2::PointD const & GetPoint() const { return m_point; }

    bool Fits(Segment const & segment) const
    {
      return segment.GetFeatureId() == m_featureId && segment.GetSegmentIdx() == m_segmentIdx;
    }

  private:
    uint32_t const m_featureId;
    uint32_t const m_segmentIdx;
    m2::PointD const m_point;
  };

  IndexGraphStarter(IndexGraph & graph, FakeVertex const & start, FakeVertex const & finish);

  IndexGraph & GetGraph() const { return m_graph; }
  Segment const & GetStart() const { return kStartFakeSegment; }
  Segment const & GetFinish() const { return kFinishFakeSegment; }
  m2::PointD const & GetPoint(Segment const & segment, bool front);

  static size_t GetRouteNumPoints(vector<Segment> const & route);
  m2::PointD const & GetRoutePoint(vector<Segment> const & route, size_t pointIndex);

  void GetEdgesList(Segment const & segment, bool isOutgoing, vector<SegmentEdge> & edges);
  void GetOutgoingEdgesList(TVertexType const & segment, vector<TEdgeType> & edges);
  void GetIngoingEdgesList(TVertexType const & segment, vector<TEdgeType> & edges);

  double HeuristicCostEstimate(TVertexType const & from, TVertexType const & to)
  {
    return m_graph.GetEstimator().CalcHeuristic(GetPoint(from, true /* front */),
                                                GetPoint(to, true /* front */));
  }

  static bool IsFakeSegment(Segment const & segment) { return segment.GetFeatureId() == kFakeFeatureId; }

private:
  static uint32_t constexpr kFakeFeatureId = numeric_limits<uint32_t>::max();
  static uint32_t constexpr kFakeSegmentIdx = numeric_limits<uint32_t>::max();
  static Segment constexpr kStartFakeSegment =
      Segment(kFakeFeatureId, kFakeSegmentIdx, false);
  static Segment constexpr kFinishFakeSegment =
      Segment(kFakeFeatureId, kFakeSegmentIdx, true);

  void GetFakeToNormalEdges(FakeVertex const & fakeVertex, vector<SegmentEdge> & edges);
  void GetFakeToNormalEdge(FakeVertex const & fakeVertex, bool forward,
                           vector<SegmentEdge> & edges);
  void GetNormalToFakeEdge(Segment const & segment, FakeVertex const & fakeVertex,
                           Segment const & fakeSegment, bool isOutgoing,
                           vector<SegmentEdge> & edges);

  IndexGraph & m_graph;
  FakeVertex const m_start;
  FakeVertex const m_finish;
};
}  // namespace routing
