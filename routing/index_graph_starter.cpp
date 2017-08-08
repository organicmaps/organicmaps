#include "routing/index_graph_starter.hpp"

#include "geometry/distance.hpp"

namespace
{
using namespace routing;
using namespace std;

Segment InvertDirection(Segment const & segment)
{
  return Segment(segment.GetMwmId(), segment.GetFeatureId(), segment.GetSegmentIdx(),
                 !segment.IsForward());
}

Junction InterpolateJunction(Segment const & segment, m2::PointD const & point, WorldGraph & graph)
{
  Junction const & begin = graph.GetJunction(segment, false /* front */);
  Junction const & end = graph.GetJunction(segment, true /* front */);

  m2::PointD const segmentDir = end.GetPoint() - begin.GetPoint();
  if (segmentDir.IsAlmostZero())
    return Junction(point, begin.GetAltitude());

  m2::PointD const pointDir = point - begin.GetPoint();

  double const ratio = m2::DotProduct(segmentDir, pointDir) / segmentDir.SquaredLength();
  if (ratio <= 0.0)
    return Junction(point, begin.GetAltitude());

  if (ratio >= 1.0)
    return Junction(point, end.GetAltitude());

  return Junction(point, static_cast<feature::TAltitude>(
                             (1.0 - ratio) * static_cast<double>(begin.GetAltitude()) +
                             ratio * (static_cast<double>(end.GetAltitude()))));
}

Junction CalcProjectionToSegment(Segment const & segment, Junction const & junction,
                                 WorldGraph & graph)
{
  m2::ProjectionToSection<m2::PointD> projection;
  projection.SetBounds(graph.GetPoint(segment, false /* front */),
                       graph.GetPoint(segment, true /* front */));
  return Junction(projection(junction.GetPoint()), junction.GetAltitude());
}
}  // namespace

namespace routing
{
void IndexGraphStarter::AddFakeVertex(Segment const & existentSegment, Segment const & newSegment,
                                      FakeVertex const & newVertex, bool isOutgoing,
                                      bool isPartOfReal, Segment const & realSegment)
{
  auto const & segmentFrom = isOutgoing ? existentSegment : newSegment;
  auto const & segmentTo = isOutgoing ? newSegment : existentSegment;
  m_fake.m_outgoing[segmentFrom].insert(segmentTo);
  m_fake.m_ingoing[segmentTo].insert(segmentFrom);
  m_fake.m_segmentToVertex[newSegment] = newVertex;
  if (isPartOfReal)
  {
    m_fake.m_realToFake[realSegment].insert(newSegment);
    m_fake.m_fakeToReal[newSegment] = realSegment;
  }
}

Segment IndexGraphStarter::GetSegment(FakeVertex const & vertex, uint32_t & newNumber) const
{
  for (auto const & v : m_fake.m_segmentToVertex)
  {
    if (v.second == vertex)
      return v.first;
  }

  return GetFakeSegment(newNumber++);
}

void IndexGraphStarter::AddEnding(FakeEnding const & thisEnding, FakeEnding const & otherEnding,
                                  bool isStart, uint32_t & fakeNumerationStart, bool strictForward)
{
  CHECK_EQUAL(thisEnding.m_projectionJunctions.size(), thisEnding.m_projectionSegments.size(), ());
  CHECK_EQUAL(otherEnding.m_projectionJunctions.size(), otherEnding.m_projectionSegments.size(), ());
  Segment dummy;

  map<Segment, Junction> otherSegments;
  for (size_t i = 0; i < otherEnding.m_projectionSegments.size(); ++i)
    otherSegments[otherEnding.m_projectionSegments[i]] = otherEnding.m_projectionJunctions[i];

  // Add pure fake vertex
  auto fakeSegment = GetFakeSegment(fakeNumerationStart++);
  FakeVertex fakeVertex(thisEnding.m_originJunction, thisEnding.m_originJunction,
                        FakeVertex::Type::PureFake);
  m_fake.m_segmentToVertex[fakeSegment] = fakeVertex;
  for (uint32_t i = 0; i < thisEnding.m_projectionJunctions.size(); ++i)
  {
    // Add projection edges
    auto projectionSegment = GetFakeSegment(fakeNumerationStart++);
    FakeVertex projection(
        isStart ? thisEnding.m_originJunction : thisEnding.m_projectionJunctions[i],
        isStart ? thisEnding.m_projectionJunctions[i] : thisEnding.m_originJunction,
        FakeVertex::Type::Projection);
    AddFakeVertex(fakeSegment, projectionSegment, projection, isStart /* isOutgoing */,
                  false /* isPartOfReal */, dummy /* realSegment */);

    // Add fake parts of real
    auto frontJunction = m_graph.GetJunction(thisEnding.m_projectionSegments[i],
                                             thisEnding.m_projectionSegments[i].IsForward());
    auto backJunction = m_graph.GetJunction(thisEnding.m_projectionSegments[i],
                                            !thisEnding.m_projectionSegments[i].IsForward());

    // Check whether we have projections to same real segment from both endings.
    auto const & it = otherSegments.find(thisEnding.m_projectionSegments[i]);
    if (it != otherSegments.end())
    {
      auto const & otherJunction = it->second;
      auto distBackToThis = m_graph.GetEstimator().CalcLeapWeight(
          backJunction.GetPoint(), thisEnding.m_projectionJunctions[i].GetPoint());
      auto distBackToOther =
          m_graph.GetEstimator().CalcLeapWeight(backJunction.GetPoint(), otherJunction.GetPoint());
      distBackToThis < distBackToOther ? frontJunction = otherJunction
                                       : backJunction = otherJunction;
    }

    FakeVertex forwardPartOfReal(isStart ? thisEnding.m_projectionJunctions[i] : backJunction,
                                 isStart ? frontJunction : thisEnding.m_projectionJunctions[i],
                                 FakeVertex::Type::PartOfReal);
    auto fakeForwardSegment = GetSegment(forwardPartOfReal, fakeNumerationStart);
    AddFakeVertex(projectionSegment, fakeForwardSegment, forwardPartOfReal,
                  isStart /* isOutgoing */, true /* isPartOfReal */,
                  thisEnding.m_projectionSegments[i]);

    bool oneWay = m_graph
                      .GetRoadGeometry(thisEnding.m_projectionSegments[i].GetMwmId(),
                                       thisEnding.m_projectionSegments[i].GetFeatureId())
                      .IsOneWay();
    if (!strictForward && !oneWay)
    {
      Segment backwardSegment = InvertDirection(thisEnding.m_projectionSegments[i]);
      FakeVertex backwardPartOfReal(isStart ? thisEnding.m_projectionJunctions[i] : frontJunction,
                                    isStart ? backJunction : thisEnding.m_projectionJunctions[i],
                                    FakeVertex::Type::PartOfReal);
      auto fakeBackwardSegment = GetSegment(backwardPartOfReal, fakeNumerationStart);
      AddFakeVertex(projectionSegment, fakeBackwardSegment, backwardPartOfReal,
                    isStart /* isOutgoing */, true /* isPartOfReal */, backwardSegment);
    }
  }
}

void IndexGraphStarter::AddStart(FakeEnding const & startEnding, FakeEnding const & finishEnding,
                                 uint32_t & fakeNumerationStart, bool strictForward)
{
  AddEnding(startEnding, finishEnding, true /* isStart */, fakeNumerationStart, strictForward);
}
void IndexGraphStarter::AddFinish(FakeEnding const & finishEnding, FakeEnding const & startEnding,
                                  uint32_t & fakeNumerationStart)
{
  AddEnding(finishEnding, startEnding, false /* isStart */, fakeNumerationStart,
            false /* strictForward */);
}

IndexGraphStarter::IndexGraphStarter(FakeEnding const & startEnding,
                                     FakeEnding const & finishEnding, uint32_t fakeNumerationStart,
                                     bool strictForward, WorldGraph & graph)
  : m_graph(graph)
{
  m_startId = fakeNumerationStart;
  AddStart(startEnding, finishEnding, fakeNumerationStart, strictForward);
  m_finishId = fakeNumerationStart;
  AddFinish(finishEnding, startEnding, fakeNumerationStart);
}

IndexGraphStarter::IndexGraphStarter(vector<IndexGraphStarter> starters)
  : m_graph(starters.front().m_graph)
{
  m_startId = starters.front().m_startId;
  m_finishId = starters.back().m_finishId;

  for (auto const & starter : starters)
  {
    m_fake.m_segmentToVertex.insert(starter.m_fake.m_segmentToVertex.begin(),
                                    starter.m_fake.m_segmentToVertex.end());

    for (auto const & s : starter.m_fake.m_outgoing)
      m_fake.m_outgoing[s.first].insert(s.second.begin(), s.second.end());

    for (auto const & s : starter.m_fake.m_ingoing)
      m_fake.m_ingoing[s.first].insert(s.second.begin(), s.second.end());

    for (auto const & s : starter.m_fake.m_realToFake)
      m_fake.m_realToFake[s.first].insert(s.second.begin(), s.second.end());

    m_fake.m_fakeToReal.insert(starter.m_fake.m_fakeToReal.begin(),
                               starter.m_fake.m_fakeToReal.end());
  }
}

void IndexGraphStarter::Append(FakeEdgesContainer const & container)
{
  m_finishId = container.m_finishId;

  m_fake.m_segmentToVertex.insert(container.m_fake.m_segmentToVertex.begin(),
                                  container.m_fake.m_segmentToVertex.end());

  for (auto const & s : container.m_fake.m_outgoing)
    m_fake.m_outgoing[s.first].insert(s.second.begin(), s.second.end());

  for (auto const & s : container.m_fake.m_ingoing)
    m_fake.m_ingoing[s.first].insert(s.second.begin(), s.second.end());

  for (auto const & s : container.m_fake.m_realToFake)
    m_fake.m_realToFake[s.first].insert(s.second.begin(), s.second.end());

  m_fake.m_fakeToReal.insert(container.m_fake.m_fakeToReal.begin(),
                             container.m_fake.m_fakeToReal.end());
}

// static
IndexGraphStarter::FakeEnding IndexGraphStarter::MakeFakeEnding(Segment const & segment,
                                                                m2::PointD const & point,
                                                                WorldGraph & graph)
{
  IndexGraphStarter::FakeEnding ending;
  ending.m_originJunction = InterpolateJunction(segment, point, graph);
  ending.m_projectionJunctions.push_back(
      CalcProjectionToSegment(segment, ending.m_originJunction, graph));
  ending.m_projectionSegments.push_back(segment);
  return ending;
}

Junction const & IndexGraphStarter::GetStartJunction() const
{
  auto const & startSegment = GetFakeSegment(m_startId);

  auto it = m_fake.m_segmentToVertex.find(startSegment);

  CHECK(it != m_fake.m_segmentToVertex.end(), ("Requested junction for invalid fake segment."));
  return it->second.GetJunctionFrom();
}

Junction const & IndexGraphStarter::GetFinishJunction() const
{
  auto const & finishSegment = GetFakeSegment(m_finishId);

  auto it = m_fake.m_segmentToVertex.find(finishSegment);

  CHECK(it != m_fake.m_segmentToVertex.end(), ("Requested junction for invalid fake segment."));
  return it->second.GetJunctionTo();
}

bool IndexGraphStarter::ConvertToReal(Segment & segment) const
{
  if (!IsFakeSegment(segment))
    return true;

  auto const & it = m_fake.m_fakeToReal.find(segment);
  if (it == m_fake.m_fakeToReal.end())
    return false;

  segment = it->second;
  return true;
}

Junction const & IndexGraphStarter::GetJunction(Segment const & segment, bool front) const
{
  if (!IsFakeSegment(segment))
    return m_graph.GetJunction(segment, front);

  auto fakeVertexIt = m_fake.m_segmentToVertex.find(segment);

  CHECK(fakeVertexIt != m_fake.m_segmentToVertex.end(),
        ("Requested junction for invalid fake segment."));
  return front ? fakeVertexIt->second.GetJunctionTo() : fakeVertexIt->second.GetJunctionFrom();
}

m2::PointD const & IndexGraphStarter::GetPoint(Segment const & segment, bool front) const
{
  return GetJunction(segment, front).GetPoint();
}

// static
void IndexGraphStarter::CheckValidRoute(vector<Segment> const & segments)
{
  // Valid route contains at least 3 segments:
  // start fake, finish fake and at least one normal nearest segment.
  CHECK_GREATER_OR_EQUAL(segments.size(), 3, ());
  CHECK(IsFakeSegment(segments.front()), ());
  CHECK(IsFakeSegment(segments.back()), ());
}

set<NumMwmId> IndexGraphStarter::GetMwms() const
{
  set<NumMwmId> mwms;
  for (auto const & s : m_fake.m_fakeToReal)
    mwms.insert(s.second.GetMwmId());

  return mwms;
}

// static
size_t IndexGraphStarter::GetRouteNumPoints(vector<Segment> const & segments)
{
  CheckValidRoute(segments);
  return segments.size() + 1;
}

Junction const & IndexGraphStarter::GetRouteJunction(vector<Segment> const & segments,
                                                     size_t pointIndex) const
{
  CHECK_LESS_OR_EQUAL(
      pointIndex, segments.size(),
      ("Point with index", pointIndex, "does not exist in route with size", segments.size()));
  if (pointIndex == segments.size())
    return GetJunction(segments[pointIndex - 1], true);
  return GetJunction(segments[pointIndex], false);
}

double IndexGraphStarter::CalcSegmentWeight(Segment const & segment) const
{
  if (!IsFakeSegment(segment))
    return m_graph.GetEstimator().CalcSegmentWeight(
        segment, m_graph.GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId()));

  auto fakeVertexIt = m_fake.m_segmentToVertex.find(segment);

  CHECK(fakeVertexIt != m_fake.m_segmentToVertex.end(),
        ("Requested junction for invalid fake segment."));
  return m_graph.GetEstimator().CalcLeapWeight(fakeVertexIt->second.GetPointFrom(),
                                               fakeVertexIt->second.GetPointTo());
}

double IndexGraphStarter::CalcRouteSegmentWeight(vector<Segment> const & route,
                                                 size_t segmentIndex) const
{
  CHECK_LESS(
      segmentIndex, route.size(),
      ("Segment with index", segmentIndex, "does not exist in route with size", route.size()));
  return CalcSegmentWeight(route[segmentIndex]);
}

bool IndexGraphStarter::IsLeap(NumMwmId mwmId) const
{
  bool inProjectedMwmIds = false;
  for (auto const & s : m_fake.m_fakeToReal)
  {
    if (s.second.GetMwmId() == mwmId)
    {
      inProjectedMwmIds = true;
      break;
    }
  }

  return mwmId != kFakeNumMwmId && !inProjectedMwmIds &&
         m_graph.GetEstimator().LeapIsAllowed(mwmId);
}

void IndexGraphStarter::AddFakeEdges(vector<SegmentEdge> & edges) const
{
  for (auto const & edge : edges)
  {
    auto const & it = m_fake.m_realToFake.find(edge.GetTarget());
    if (it == m_fake.m_realToFake.end())
      continue;
    for (auto const & s : it->second)
      edges.emplace_back(s, RouteWeight(edge.GetWeight()));
  }
}

void IndexGraphStarter::AddRealEdges(Segment const & segment, bool isOutgoing,
                                     std::vector<SegmentEdge> & edges) const
{
  if (m_graph.GetMode() == WorldGraph::Mode::LeapsOnly &&
      m_fake.m_realToFake.find(segment) != m_fake.m_realToFake.end())
  {
    ConnectLeapToTransitions(segment, isOutgoing, edges);
    return;
  }

  m_graph.GetEdgeList(segment, isOutgoing, IsLeap(segment.GetMwmId()), edges);
}

void IndexGraphStarter::GetEdgesList(Segment const & segment, bool isOutgoing,
                                     vector<SegmentEdge> & edges) const
{
  edges.clear();
  if (IsFakeSegment(segment))
  {
    auto const & vertexIt = m_fake.m_segmentToVertex.find(segment);
    CHECK(vertexIt != m_fake.m_segmentToVertex.end(),
          ("Can not map fake segment", segment, "to fake vertex."));

    if (vertexIt->second.GetType() == FakeVertex::Type::PartOfReal)
    {
      auto const & realIt = m_fake.m_fakeToReal.find(segment);
      CHECK(realIt != m_fake.m_fakeToReal.end(),
            ("Can not map fake segment", segment, "to real segment."));
      AddRealEdges(realIt->second, isOutgoing, edges);
    }

    auto const & fakeGraph = isOutgoing ? m_fake.m_outgoing : m_fake.m_ingoing;

    auto it = fakeGraph.find(segment);
    if (it != fakeGraph.end())
    {
      for (auto const & s : it->second)
        edges.emplace_back(s, RouteWeight(CalcSegmentWeight(isOutgoing ? s : segment)));
    }
  }
  else
  {
    AddRealEdges(segment, isOutgoing, edges);
  }

  AddFakeEdges(edges);
}

void IndexGraphStarter::ConnectLeapToTransitions(Segment const & segment, bool isOutgoing,
                                                 vector<SegmentEdge> & edges) const
{
  edges.clear();
  m2::PointD const & segmentPoint = GetPoint(segment, true /* front */);

  // Note. If |isOutgoing| == true it's necessary to add edges which connect the start with all
  // exits of its mwm. So |isEnter| below should be set to false.
  // If |isOutgoing| == false all enters of the finish mwm should be connected with the finish point.
  // So |isEnter| below should be set to true.
  m_graph.ForEachTransition(
      segment.GetMwmId(), !isOutgoing /* isEnter */, [&](Segment const & transition) {
        edges.emplace_back(transition, RouteWeight(m_graph.GetEstimator().CalcLeapWeight(
                                           segmentPoint, GetPoint(transition, isOutgoing))));
      });
}
}  // namespace routing
