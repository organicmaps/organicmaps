#include "routing/index_graph_starter.hpp"

#include "geometry/distance.hpp"

namespace
{
using namespace routing;
using namespace std;

m2::PointD CalcProjectionToSegment(Segment const & segment, m2::PointD const & point,
                                   WorldGraph & graph)
{
  m2::ProjectionToSection<m2::PointD> projection;
  projection.SetBounds(graph.GetPoint(segment, false /* front */),
                       graph.GetPoint(segment, true /* front */));
  return projection(point);
}
}  // namespace

namespace routing
{
// static
Segment constexpr IndexGraphStarter::kStartFakeSegment;
Segment constexpr IndexGraphStarter::kFinishFakeSegment;

IndexGraphStarter::IndexGraphStarter(FakeVertex const & start, FakeVertex const & finish,
                                     WorldGraph & graph)
  : m_graph(graph)
  , m_start(start.GetSegment(),
            CalcProjectionToSegment(start.GetSegment(), start.GetPoint(), graph),
            start.GetStrictForward())
  , m_finish(finish.GetSegment(),
             CalcProjectionToSegment(finish.GetSegment(), finish.GetPoint(), graph),
             finish.GetStrictForward())
{
}

m2::PointD const & IndexGraphStarter::GetPoint(Segment const & segment, bool front)
{
  if (segment == kStartFakeSegment)
    return m_start.GetPoint();

  if (segment == kFinishFakeSegment)
    return m_finish.GetPoint();

  return m_graph.GetPoint(segment, front);
}

// static
void IndexGraphStarter::CheckValidRoute(vector<Segment> const &segments)
{
  // Valid route contains at least 3 segments:
  // start fake, finish fake and at least one normal nearest segment.
  CHECK_GREATER_OR_EQUAL(segments.size(), 3, ());
  CHECK_EQUAL(segments.front(), kStartFakeSegment, ());
  CHECK_EQUAL(segments.back(), kFinishFakeSegment, ());
}

// static
vector<Segment>::const_iterator IndexGraphStarter::GetNonFakeStart(vector<Segment> const & segments)
{
  CheckValidRoute(segments);
  // See CheckValidRoute comment.
  return segments.begin() + 1;
}

// static
vector<Segment>::const_iterator IndexGraphStarter::GetNonFakeFinish(
    vector<Segment> const & segments)
{
  CheckValidRoute(segments);
  // See CheckValidRoute comment.
  return segments.end() - 2;
}

// static
size_t IndexGraphStarter::GetRouteNumPoints(vector<Segment> const & segments)
{
  CheckValidRoute(segments);
  return segments.size() - 1;
}

m2::PointD const & IndexGraphStarter::GetRoutePoint(vector<Segment> const & segments,
                                                    size_t pointIndex)
{
  if (pointIndex == 0)
    return m_start.GetPoint();

  if (pointIndex + 1 == GetRouteNumPoints(segments))
    return m_finish.GetPoint();

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

  if (m_graph.GetMode() == WorldGraph::Mode::LeapsOnly && (m_start.Fits(segment) || m_finish.Fits(segment)))
  {
    ConnectLeapToTransitions(segment, isOutgoing, edges);
    return;
  }

  m_graph.GetEdgeList(segment, isOutgoing, IsLeap(segment.GetMwmId()), edges);
  GetNormalToFakeEdge(segment, m_start, kStartFakeSegment, isOutgoing, edges);
  GetNormalToFakeEdge(segment, m_finish, kFinishFakeSegment, isOutgoing, edges);
}

void IndexGraphStarter::GetFakeToNormalEdges(FakeVertex const & fakeVertex, bool isOutgoing,
                                             vector<SegmentEdge> & edges)
{
  if (fakeVertex.GetStrictForward())
  {
    GetFakeToNormalEdge(fakeVertex, fakeVertex.GetSegment().IsForward(), edges);
  }
  else
  {
    GetFakeToNormalEdge(fakeVertex, true /* forward */, edges);

    if (!m_graph.GetRoadGeometry(fakeVertex.GetMwmId(), fakeVertex.GetFeatureId()).IsOneWay())
      GetFakeToNormalEdge(fakeVertex, false /* forward */, edges);
  }
}

void IndexGraphStarter::GetFakeToNormalEdge(FakeVertex const & fakeVertex, bool forward,
                                            vector<SegmentEdge> & edges)
{
  auto const segment = fakeVertex.GetSegmentWithDirection(forward);
  m2::PointD const & pointTo = GetPoint(segment, true /* front */);
  double const weight = m_graph.GetEstimator().CalcLeapWeight(fakeVertex.GetPoint(), pointTo);
  edges.emplace_back(segment, weight);
}

void IndexGraphStarter::GetNormalToFakeEdge(Segment const & segment, FakeVertex const & fakeVertex,
                                            Segment const & fakeSegment, bool isOutgoing,
                                            vector<SegmentEdge> & edges)
{
  m2::PointD const & pointFrom = GetPoint(segment, isOutgoing);
  if (segment.GetMwmId() == fakeVertex.GetMwmId() &&
      m_graph.GetMode() == WorldGraph::Mode::LeapsOnly)
  {
    if (m_graph.IsTransition(segment, isOutgoing))
    {
      edges.emplace_back(fakeSegment,
                         m_graph.GetEstimator().CalcLeapWeight(pointFrom, fakeVertex.GetPoint()));
    }
    return;
  }

  if (fakeVertex.Fits(segment))
  {
    edges.emplace_back(fakeSegment,
                       m_graph.GetEstimator().CalcLeapWeight(pointFrom, fakeVertex.GetPoint()));
  }
}

void IndexGraphStarter::ConnectLeapToTransitions(Segment const & segment, bool isOutgoing,
                                                 vector<SegmentEdge> & edges)
{
  edges.clear();
  m2::PointD const & segmentPoint = GetPoint(segment, true /* front */);

  // Note. If |isOutgoing| == true it's necessary to add edges which connect the start with all
  // exits of its mwm. So |isEnter| below should be set to false.
  // If |isOutgoing| == false all enters of the finish mwm should be connected with the finish point.
  // So |isEnter| below should be set to true.
  m_graph.ForEachTransition(
      segment.GetMwmId(), !isOutgoing /* isEnter */, [&](Segment const & transition) {
        edges.emplace_back(transition, m_graph.GetEstimator().CalcLeapWeight(
          segmentPoint, GetPoint(transition, isOutgoing)));
      });
}
}  // namespace routing
