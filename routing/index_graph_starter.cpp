#include "routing/index_graph_starter.hpp"

#include "routing/routing_exceptions.hpp"

namespace routing
{
// static
Segment constexpr IndexGraphStarter::kStartFakeSegment;
Segment constexpr IndexGraphStarter::kFinishFakeSegment;

IndexGraphStarter::IndexGraphStarter(IndexGraph & graph, FakeVertex const & start,
                                     FakeVertex const & finish)
  : m_graph(graph), m_start(start), m_finish(finish)
{
}

m2::PointD const & IndexGraphStarter::GetPoint(Segment const & segment, bool front)
{
  if (segment == kStartFakeSegment)
    return m_start.GetPoint();

  if (segment == kFinishFakeSegment)
    return m_finish.GetPoint();

  return m_graph.GetGeometry().GetPoint(segment.GetRoadPoint(front));
}

// static
size_t IndexGraphStarter::GetRouteNumPoints(vector<Segment> const & route)
{
  // if route contains start and finish fakes only, it doesn't have segment points.
  // TODO: add start and finish when RecounstructRoute will be reworked.
  if (route.size() <= 2)
    return 0;

  // -2 for fake start and finish.
  // +1 for front point of first segment.
  return route.size() - 1;
}

m2::PointD const & IndexGraphStarter::GetRoutePoint(vector<Segment> const & route,
                                                    size_t pointIndex)
{
  if (pointIndex == 0)
  {
    CHECK_GREATER(route.size(), 1, ());
    return GetPoint(route[1], false /* front */);
  }

  CHECK_LESS(pointIndex, route.size(), ());
  return GetPoint(route[pointIndex], true /* front */);
}

void IndexGraphStarter::GetEdgesList(Segment const & segment, bool isOutgoing,
                                     vector<SegmentEdge> & edges)
{
  edges.clear();

  if (segment == kStartFakeSegment)
  {
    GetFakeToNormalEdges(m_start, edges);
    return;
  }

  if (segment == kFinishFakeSegment)
  {
    GetFakeToNormalEdges(m_finish, edges);
    return;
  }

  m_graph.GetEdgeList(segment, isOutgoing, edges);
  GetNormalToFakeEdge(segment, m_start, kStartFakeSegment, isOutgoing, edges);
  GetNormalToFakeEdge(segment, m_finish, kFinishFakeSegment, isOutgoing, edges);
}

void IndexGraphStarter::GetFakeToNormalEdges(FakeVertex const & fakeVertex,
                                             vector<SegmentEdge> & edges)
{
  GetFakeToNormalEdge(fakeVertex, true /* forward */, edges);

  if (!m_graph.GetGeometry().GetRoad(fakeVertex.GetFeatureId()).IsOneWay())
    GetFakeToNormalEdge(fakeVertex, false /* forward */, edges);
}

void IndexGraphStarter::GetFakeToNormalEdge(FakeVertex const & fakeVertex, bool forward,
                                            vector<SegmentEdge> & edges)
{
  Segment const segment(fakeVertex.GetFeatureId(), fakeVertex.GetSegmentIdx(), forward);
  RoadPoint const & roadPoint = segment.GetRoadPoint(true /* front */);
  m2::PointD const & pointTo = m_graph.GetGeometry().GetPoint(roadPoint);
  double const weight = m_graph.GetEstimator().CalcHeuristic(fakeVertex.GetPoint(), pointTo);
  edges.emplace_back(segment, weight);
}

void IndexGraphStarter::GetNormalToFakeEdge(Segment const & segment, FakeVertex const & fakeVertex,
                                            Segment const & fakeSegment, bool isOutgoing,
                                            vector<SegmentEdge> & edges)
{
  if (!fakeVertex.Fits(segment))
    return;

  m2::PointD const & pointFrom = m_graph.GetGeometry().GetPoint(segment.GetRoadPoint(isOutgoing));
  double const weight = m_graph.GetEstimator().CalcHeuristic(pointFrom, fakeVertex.GetPoint());
  edges.emplace_back(fakeSegment, weight);
}
}  // namespace routing
