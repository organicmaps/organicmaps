#pragma once

#include "routing/index_graph.hpp"
#include "routing/joint.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/route_point.hpp"
#include "routing/world_graph.hpp"

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
    FakeVertex(NumMwmId mwmId, uint32_t featureId, uint32_t segmentIdx, m2::PointD const & point)
      : m_featureId(featureId), m_segmentIdx(segmentIdx), m_mwmId(mwmId), m_point(point)
    {
    }

    FakeVertex(Segment const & segment, m2::PointD const & point)
      : m_featureId(segment.GetFeatureId())
      , m_segmentIdx(segment.GetSegmentIdx())
      , m_mwmId(segment.GetMwmId())
      , m_point(point)
    {
    }

    NumMwmId GetMwmId() const { return m_mwmId; }
    uint32_t GetFeatureId() const { return m_featureId; }
    uint32_t GetSegmentIdx() const { return m_segmentIdx; }
    m2::PointD const & GetPoint() const { return m_point; }

    bool Fits(Segment const & segment) const
    {
      return segment.GetFeatureId() == m_featureId && segment.GetSegmentIdx() == m_segmentIdx &&
             segment.GetMwmId() == m_mwmId;
    }

  private:
    uint32_t const m_featureId;
    uint32_t const m_segmentIdx;
    NumMwmId const m_mwmId;
    m2::PointD const m_point;
  };

  IndexGraphStarter(FakeVertex const & start, FakeVertex const & finish, WorldGraph & graph);

  WorldGraph & GetGraph() { return m_graph; }
  Segment const & GetStart() const { return kStartFakeSegment; }
  Segment const & GetFinish() const { return kFinishFakeSegment; }
  m2::PointD const & GetPoint(Segment const & segment, bool front);

  static size_t GetRouteNumPoints(vector<Segment> const & route);
  m2::PointD const & GetRoutePoint(vector<Segment> const & route, size_t pointIndex);

  void GetEdgesList(Segment const & segment, bool isOutgoing, vector<SegmentEdge> & edges);

  void GetOutgoingEdgesList(TVertexType const & segment, vector<TEdgeType> & edges)
  {
    GetEdgesList(segment, true /* isOutgoing */, edges);
  }

  void GetIngoingEdgesList(TVertexType const & segment, vector<TEdgeType> & edges)
  {
    GetEdgesList(segment, false /* isOutgoing */, edges);
  }

  double HeuristicCostEstimate(TVertexType const & from, TVertexType const & to)
  {
    return m_graph.GetEstimator().CalcHeuristic(GetPoint(from, true /* front */),
                                                GetPoint(to, true /* front */));
  }

  double CalcSegmentWeight(Segment const & segment) const
  {
    return m_graph.GetEstimator().CalcSegmentWeight(
        segment, m_graph.GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId()));
  }

  bool IsLeap(NumMwmId mwmId) const
  {
    return mwmId != kFakeNumMwmId && mwmId != m_start.GetMwmId() && mwmId != m_finish.GetMwmId() &&
           m_graph.GetEstimator().LeapIsAllowed(mwmId);
  }

  static bool IsFakeSegment(Segment const & segment)
  {
    return segment.GetFeatureId() == kFakeFeatureId;
  }

private:
  static uint32_t constexpr kFakeFeatureId = numeric_limits<uint32_t>::max();
  static uint32_t constexpr kFakeSegmentIdx = numeric_limits<uint32_t>::max();
  static Segment constexpr kStartFakeSegment =
      Segment(kFakeNumMwmId, kFakeFeatureId, kFakeSegmentIdx, false);
  static Segment constexpr kFinishFakeSegment =
      Segment(kFakeNumMwmId, kFakeFeatureId, kFakeSegmentIdx, true);

  void GetFakeToNormalEdges(FakeVertex const & fakeVertex, vector<SegmentEdge> & edges);
  void GetFakeToNormalEdge(FakeVertex const & fakeVertex, bool forward,
                           vector<SegmentEdge> & edges);
  void GetNormalToFakeEdge(Segment const & segment, FakeVertex const & fakeVertex,
                           Segment const & fakeSegment, bool isOutgoing,
                           vector<SegmentEdge> & edges);

  WorldGraph & m_graph;
  FakeVertex const m_start;
  FakeVertex const m_finish;
};
}  // namespace routing
