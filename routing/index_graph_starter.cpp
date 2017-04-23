#include "routing/index_graph_starter.hpp"

#include "routing/routing_exceptions.hpp"

namespace routing
{
// static
Segment constexpr IndexGraphStarter::kStartFakeSegment;
Segment constexpr IndexGraphStarter::kFinishFakeSegment;

IndexGraphStarter::IndexGraphStarter(FakeVertex const & start, FakeVertex const & finish,
                                     WorldGraph & graph)
  : m_graph(graph), m_start(start), m_finish(finish)
{
}

m2::PointD const & IndexGraphStarter::GetPoint(Segment const & segment, bool front)
{
  if (segment == kStartFakeSegment || (!front && m_start.Fits(segment)))
    return m_start.GetPoint();

  if (segment == kFinishFakeSegment || (front && m_finish.Fits(segment)))
    return m_finish.GetPoint();

  return m_graph.GetPoint(segment, front);
}

// static
size_t IndexGraphStarter::GetRouteNumPoints(vector<Segment> const & segments)
{
  // Valid route contains at least 3 segments:
  // start fake, finish fake and at least one normal nearest segment.
  CHECK_GREATER_OR_EQUAL(segments.size(), 3, ());

  // -2 for fake start and finish.
  // +1 for front point of first segment.
  return segments.size() - 1;
}

m2::PointD const & IndexGraphStarter::GetRoutePoint(vector<Segment> const & segments,
                                                    size_t pointIndex)
{
  if (pointIndex == 0)
    return m_start.GetPoint();

  CHECK_LESS(pointIndex, segments.size(), ());
  return GetPoint(segments[pointIndex], true /* front */);
}

void IndexGraphStarter::GetEdgesList(Segment const & segment, bool isOutgoing,
                                     vector<SegmentEdge> & edges)
{
  edges.clear();

  if (segment == kStartFakeSegment)
  {
    GetFakeToNormalEdges(m_start, isOutgoing, edges);
    return;
  }

  if (segment == kFinishFakeSegment)
  {
    GetFakeToNormalEdges(m_finish, isOutgoing, edges);
    return;
  }

  m_graph.GetEdgeList(segment, isOutgoing, IsLeap(segment.GetMwmId()), edges);
  GetNormalToFakeEdge(segment, m_start, kStartFakeSegment, isOutgoing, edges);
  GetNormalToFakeEdge(segment, m_finish, kFinishFakeSegment, isOutgoing, edges);
}

void IndexGraphStarter::GetFakeToNormalEdges(FakeVertex const & fakeVertex,
                                             bool isOutgoing,
                                             vector<SegmentEdge> & edges)
{
  if (m_graph.GetMode() == WorldGraph::Mode::LeapsOnly)
  {
    ConnectLeapToTransitions(fakeVertex, isOutgoing, edges);
    return;
  }

  GetFakeToNormalEdge(fakeVertex, true /* forward */, edges);

  if (!m_graph.GetRoadGeometry(fakeVertex.GetMwmId(), fakeVertex.GetFeatureId()).IsOneWay())
    GetFakeToNormalEdge(fakeVertex, false /* forward */, edges);
}

void IndexGraphStarter::GetFakeToNormalEdge(FakeVertex const & fakeVertex, bool forward,
                                            vector<SegmentEdge> & edges)
{
  Segment const segment(fakeVertex.GetMwmId(), fakeVertex.GetFeatureId(),
                        fakeVertex.GetSegmentIdx(), forward);
  m2::PointD const & pointTo = GetPoint(segment, true /* front */);
  double const weight = m_graph.GetEstimator().CalcLeapEdgeTime(fakeVertex.GetPoint(), pointTo);
  edges.emplace_back(segment, weight);
}

void IndexGraphStarter::GetNormalToFakeEdge(Segment const & segment, FakeVertex const & fakeVertex,
                                            Segment const & fakeSegment, bool isOutgoing,
                                            vector<SegmentEdge> & edges)
{
  m2::PointD const & pointFrom = GetPoint(segment, isOutgoing);
  if (segment.GetMwmId() == fakeVertex.GetMwmId() && m_graph.GetMode() == WorldGraph::Mode::LeapsOnly)
  {
    // It's assumed here that GetEstimator().CalcLeapEdgeTime(p1, p2) == GetEstimator().CalcLeapEdgeTime(p2, p1).
    if (m_graph.IsTransition(segment, isOutgoing))
      edges.emplace_back(fakeSegment, m_graph.GetEstimator().CalcLeapEdgeTime(pointFrom, fakeVertex.GetPoint()));
    return;
  }

  if (!fakeVertex.Fits(segment))
    return;

  edges.emplace_back(fakeSegment, m_graph.GetEstimator().CalcLeapEdgeTime(pointFrom, fakeVertex.GetPoint()));
}

void IndexGraphStarter::ConnectLeapToTransitions(FakeVertex const & fakeVertex, bool isOutgoing,
                                                 vector<SegmentEdge> & edges)
{
  edges.clear();
  m2::PointD const & segmentPoint = fakeVertex.GetPoint();

  // Note. If |isOutgoing| == true it's necessary to add edges which connect the start with all
  // exits of its mwm.
  // So |isEnter| below should be set to false. If |isOutgoing| == false all enters of the
  // finish mwm should be connected with the finish point. So |isEnter| below should be set to true.
  m_graph.ForEachTransition(
      fakeVertex.GetMwmId(), !isOutgoing /* isEnter */, [&](Segment const & transition) {
        // It's assumed here that GetEstimator().CalcLeapEdgeTime(p1, p2) ==
        // GetEstimator().CalcLeapEdgeTime(p2, p1).
        edges.emplace_back(transition, m_graph.GetEstimator().CalcLeapEdgeTime(
          segmentPoint, GetPoint(transition, isOutgoing)));
      });
}

Segment const & IndexGraphStarter::ConvertSegment(Segment const & segment)
{
  if (segment == kStartFakeSegment)
    return m_start.GetSegment();
  if (segment == kFinishFakeSegment)
    return m_finish.GetSegment();
  return segment;
}
}  // namespace routing
