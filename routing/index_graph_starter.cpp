#include "routing/index_graph_starter.hpp"

#include "routing/fake_edges_container.hpp"

#include "geometry/distance.hpp"

#include "base/math.hpp"
#include "base/stl_helpers.hpp"
#include "base/stl_iterator.hpp"

namespace
{
using namespace routing;
using namespace std;

Segment GetReverseSegment(Segment const & segment)
{
  return Segment(segment.GetMwmId(), segment.GetFeatureId(), segment.GetSegmentIdx(),
                 !segment.IsForward());
}

Junction CalcProjectionToSegment(Segment const & segment, m2::PointD const & point,
                                 WorldGraph & graph)
{
  auto const & begin = graph.GetJunction(segment, false /* front */);
  auto const & end = graph.GetJunction(segment, true /* front */);
  m2::ProjectionToSection<m2::PointD> projection;
  projection.SetBounds(begin.GetPoint(), end.GetPoint());
  auto projectedPoint = projection(point);
  auto const distBeginToEnd = graph.CalcLeapWeight(begin.GetPoint(), end.GetPoint()).GetWeight();

  double constexpr kEpsMeters = 2.0;
  if (my::AlmostEqualAbs(distBeginToEnd, 0.0, kEpsMeters))
    return Junction(projectedPoint, begin.GetAltitude());

  auto const distBeginToProjection =
      graph.CalcLeapWeight(begin.GetPoint(), projectedPoint).GetWeight();
  auto const altitude = begin.GetAltitude() + (end.GetAltitude() - begin.GetAltitude()) *
                                                  distBeginToProjection / distBeginToEnd;
  return Junction(projectedPoint, altitude);
}

template<typename K, typename V>
bool IsIntersectionEmpty(map<K, V> const & lhs, map<K, V> const & rhs)
{
  auto const counter = set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                                        CounterIterator(), my::LessBy(&pair<K, V>::first));
  return counter.GetCount() == 0;
}
}  // namespace

namespace routing
{
// FakeGraph -------------------------------------------------------------------
void IndexGraphStarter::FakeGraph::AddFakeVertex(Segment const & existentSegment,
                                                 Segment const & newSegment,
                                                 FakeVertex const & newVertex, bool isOutgoing,
                                                 bool isPartOfReal, Segment const & realSegment)
{
  auto const & segmentFrom = isOutgoing ? existentSegment : newSegment;
  auto const & segmentTo = isOutgoing ? newSegment : existentSegment;
  m_outgoing[segmentFrom].insert(segmentTo);
  m_ingoing[segmentTo].insert(segmentFrom);
  m_segmentToVertex[newSegment] = newVertex;
  if (isPartOfReal)
  {
    m_realToFake[realSegment].insert(newSegment);
    m_fakeToReal[newSegment] = realSegment;
  }
}

Segment IndexGraphStarter::FakeGraph::GetSegment(FakeVertex const & vertex,
                                                 uint32_t & newNumber) const
{
  for (auto const & kv : m_segmentToVertex)
  {
    if (kv.second == vertex)
      return kv.first;
  }

  return GetFakeSegmentAndIncr(newNumber);
}

void IndexGraphStarter::FakeGraph::Append(FakeGraph const & rhs)
{
  CHECK(IsIntersectionEmpty(m_segmentToVertex, rhs.m_segmentToVertex), ("Fake segment ids are not unique"));
  m_segmentToVertex.insert(rhs.m_segmentToVertex.begin(), rhs.m_segmentToVertex.end());

  for (auto const & kv : rhs.m_outgoing)
    m_outgoing[kv.first].insert(kv.second.begin(), kv.second.end());

  for (auto const & kv : rhs.m_ingoing)
    m_ingoing[kv.first].insert(kv.second.begin(), kv.second.end());

  for (auto const & kv : rhs.m_realToFake)
    m_realToFake[kv.first].insert(kv.second.begin(), kv.second.end());

  m_fakeToReal.insert(rhs.m_fakeToReal.begin(), rhs.m_fakeToReal.end());
}

// IndexGraphStarter -----------------------------------------------------------
// static
IndexGraphStarter::FakeEnding IndexGraphStarter::MakeFakeEnding(Segment const & segment,
                                                                m2::PointD const & point,
                                                                WorldGraph & graph)
{
  IndexGraphStarter::FakeEnding ending;
  auto const & projectedJunction = CalcProjectionToSegment(segment, point, graph);
  ending.m_originJunction = Junction(point, projectedJunction.GetAltitude());
  ending.m_projections.push_back(Projection{segment, projectedJunction});
  return ending;
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

// static
size_t IndexGraphStarter::GetRouteNumPoints(vector<Segment> const & segments)
{
  CheckValidRoute(segments);
  return segments.size() + 1;
}

IndexGraphStarter::IndexGraphStarter(FakeEnding const & startEnding,
                                     FakeEnding const & finishEnding, uint32_t fakeNumerationStart,
                                     bool strictForward, WorldGraph & graph)
  : m_graph(graph)
{
  m_startId = fakeNumerationStart;
  AddStart(startEnding, finishEnding, strictForward, fakeNumerationStart);
  m_finishId = fakeNumerationStart;
  AddFinish(finishEnding, startEnding, fakeNumerationStart);
}

void IndexGraphStarter::Append(FakeEdgesContainer const & container)
{
  m_finishId = container.m_finishId;
  m_fake.Append(container.m_fake);
}

Junction const & IndexGraphStarter::GetStartJunction() const
{
  auto const & startSegment = GetFakeSegment(m_startId);
  auto const it = m_fake.m_segmentToVertex.find(startSegment);

  CHECK(it != m_fake.m_segmentToVertex.end(), ("Requested junction for invalid fake segment."));
  return it->second.GetJunctionFrom();
}

Junction const & IndexGraphStarter::GetFinishJunction() const
{
  auto const & finishSegment = GetFakeSegment(m_finishId);
  auto const it = m_fake.m_segmentToVertex.find(finishSegment);

  CHECK(it != m_fake.m_segmentToVertex.end(), ("Requested junction for invalid fake segment."));
  return it->second.GetJunctionTo();
}

bool IndexGraphStarter::ConvertToReal(Segment & segment) const
{
  if (!IsFakeSegment(segment))
    return true;

  auto const it = m_fake.m_fakeToReal.find(segment);
  if (it == m_fake.m_fakeToReal.end())
    return false;

  segment = it->second;
  return true;
}

Junction const & IndexGraphStarter::GetJunction(Segment const & segment, bool front) const
{
  if (!IsFakeSegment(segment))
    return m_graph.GetJunction(segment, front);

  auto const fakeVertexIt = m_fake.m_segmentToVertex.find(segment);

  CHECK(fakeVertexIt != m_fake.m_segmentToVertex.end(),
        ("Requested junction for invalid fake segment."));
  return front ? fakeVertexIt->second.GetJunctionTo() : fakeVertexIt->second.GetJunctionFrom();
}

Junction const & IndexGraphStarter::GetRouteJunction(vector<Segment> const & segments,
                                                     size_t pointIndex) const
{
  CHECK(!segments.empty(), ());
  CHECK_LESS_OR_EQUAL(
      pointIndex, segments.size(),
      ("Point with index", pointIndex, "does not exist in route with size", segments.size()));
  if (pointIndex == segments.size())
    return GetJunction(segments[pointIndex - 1], true /* front */);
  return GetJunction(segments[pointIndex], false);
}

m2::PointD const & IndexGraphStarter::GetPoint(Segment const & segment, bool front) const
{
  return GetJunction(segment, front).GetPoint();
}

set<NumMwmId> IndexGraphStarter::GetMwms() const
{
  set<NumMwmId> mwms;
  for (auto const & kv : m_fake.m_fakeToReal)
    mwms.insert(kv.second.GetMwmId());

  return mwms;
}

bool IndexGraphStarter::DoesRouteCrossNontransit(
    RoutingResult<Segment, RouteWeight> const & result) const
{
  uint32_t nontransitCrossAllowed = 0;
  auto isRealOrPart = [this](Segment const & segment) {
    if (!IsFakeSegment(segment))
      return true;
    return m_fake.m_fakeToReal.find(segment) != m_fake.m_fakeToReal.end();
  };
  auto isTransitAllowed = [this](Segment const & segment) {
    auto real = segment;
    bool const convertionResult = ConvertToReal(real);
    CHECK(convertionResult, ());
    return m_graph.GetRoadGeometry(real.GetMwmId(), real.GetFeatureId()).IsTransitAllowed();
  };

  auto const firstRealOrPart = find_if(result.m_path.begin(), result.m_path.end(), isRealOrPart);
  if (firstRealOrPart != result.m_path.end() && !isTransitAllowed(*firstRealOrPart))
    ++nontransitCrossAllowed;

  auto const lastRealOrPart = find_if(result.m_path.rbegin(), result.m_path.rend(), isRealOrPart);
  // If firstRealOrPart and lastRealOrPart point to the same segment increment
  // nontransitCrossAllowed once
  if (lastRealOrPart != result.m_path.rend() && &*lastRealOrPart != &*firstRealOrPart &&
      !isTransitAllowed(*lastRealOrPart))
  {
    ++nontransitCrossAllowed;
  }

  return nontransitCrossAllowed < result.m_distance.GetNontransitCross();
}

void IndexGraphStarter::GetEdgesList(Segment const & segment, bool isOutgoing,
                                     vector<SegmentEdge> & edges) const
{
  edges.clear();
  if (IsFakeSegment(segment))
  {
    auto const vertexIt = m_fake.m_segmentToVertex.find(segment);
    CHECK(vertexIt != m_fake.m_segmentToVertex.end(),
          ("Can not map fake segment", segment, "to fake vertex."));

    if (vertexIt->second.GetType() == FakeVertex::Type::PartOfReal)
    {
      auto const realIt = m_fake.m_fakeToReal.find(segment);
      CHECK(realIt != m_fake.m_fakeToReal.end(),
            ("Can not map fake segment", segment, "to real segment."));
      AddRealEdges(realIt->second, isOutgoing, edges);
    }

    auto const & adjacentEdges = isOutgoing ? m_fake.m_outgoing : m_fake.m_ingoing;

    auto const it = adjacentEdges.find(segment);
    if (it != adjacentEdges.end())
    {
      for (auto const & s : it->second)
        edges.emplace_back(s, CalcSegmentWeight(isOutgoing ? s : segment));
    }
  }
  else
  {
    AddRealEdges(segment, isOutgoing, edges);
  }

  AddFakeEdges(segment, edges);
}

RouteWeight IndexGraphStarter::CalcSegmentWeight(Segment const & segment) const
{
  if (!IsFakeSegment(segment))
  {
    return m_graph.CalcSegmentWeight(segment);
  }

  auto const fakeVertexIt = m_fake.m_segmentToVertex.find(segment);

  CHECK(fakeVertexIt != m_fake.m_segmentToVertex.end(),
        ("Requested junction for invalid fake segment."));
  return m_graph.CalcLeapWeight(fakeVertexIt->second.GetPointFrom(),
                                fakeVertexIt->second.GetPointTo());
}

RouteWeight IndexGraphStarter::CalcRouteSegmentWeight(vector<Segment> const & route,
                                                      size_t segmentIndex) const
{
  CHECK_LESS(
      segmentIndex, route.size(),
      ("Segment with index", segmentIndex, "does not exist in route with size", route.size()));
  return CalcSegmentWeight(route[segmentIndex]);
}

bool IndexGraphStarter::IsLeap(NumMwmId mwmId) const
{
  if (mwmId == kFakeNumMwmId)
    return false;

  for (auto const & kv : m_fake.m_fakeToReal)
  {
    if (kv.second.GetMwmId() == mwmId)
      return false;
  }

  return m_graph.LeapIsAllowed(mwmId);
}

void IndexGraphStarter::AddEnding(FakeEnding const & thisEnding, FakeEnding const & otherEnding,
                                  bool isStart, bool strictForward, uint32_t & fakeNumerationStart)
{
  Segment const dummy = Segment();

  map<Segment, Junction> otherSegments;
  for (auto const & p : otherEnding.m_projections)
  {
    otherSegments[p.m_segment] = p.m_junction;
    // We use |otherEnding| to generate proper fake edges in case both endings have projections
    // to the same segment. Direction of p.m_segment does not matter.
    otherSegments[GetReverseSegment(p.m_segment)] = p.m_junction;
  }

  // Add pure fake vertex
  auto const fakeSegment = GetFakeSegmentAndIncr(fakeNumerationStart);
  FakeVertex fakeVertex(thisEnding.m_originJunction, thisEnding.m_originJunction,
                        FakeVertex::Type::PureFake);
  m_fake.m_segmentToVertex[fakeSegment] = fakeVertex;
  for (auto const & projection : thisEnding.m_projections)
  {
    // Add projection edges
    auto const projectionSegment = GetFakeSegmentAndIncr(fakeNumerationStart);
    FakeVertex projectionVertex(isStart ? thisEnding.m_originJunction : projection.m_junction,
                                isStart ? projection.m_junction : thisEnding.m_originJunction,
                                FakeVertex::Type::PureFake);
    m_fake.AddFakeVertex(fakeSegment, projectionSegment, projectionVertex, isStart /* isOutgoing */,
                         false /* isPartOfReal */, dummy /* realSegment */);

    // Add fake parts of real
    auto frontJunction = m_graph.GetJunction(projection.m_segment, true /* front */);
    auto backJunction = m_graph.GetJunction(projection.m_segment, false /* front */);

    // Check whether we have projections to same real segment from both endings.
    auto const it = otherSegments.find(projection.m_segment);
    if (it != otherSegments.end())
    {
      auto const & otherJunction = it->second;
      auto const distBackToThis = m_graph.CalcLeapWeight(backJunction.GetPoint(),
                                                         projection.m_junction.GetPoint());
      auto const distBackToOther = m_graph.CalcLeapWeight(backJunction.GetPoint(),
                                                          otherJunction.GetPoint());
      if (distBackToThis < distBackToOther)
        frontJunction = otherJunction;
      else
        backJunction = otherJunction;
    }

    FakeVertex forwardPartOfReal(isStart ? projection.m_junction : backJunction,
                                 isStart ? frontJunction : projection.m_junction,
                                 FakeVertex::Type::PartOfReal);
    auto const fakeForwardSegment = m_fake.GetSegment(forwardPartOfReal, fakeNumerationStart);
    m_fake.AddFakeVertex(projectionSegment, fakeForwardSegment, forwardPartOfReal,
                         isStart /* isOutgoing */, true /* isPartOfReal */, projection.m_segment);

    bool const oneWay =
        m_graph
            .GetRoadGeometry(projection.m_segment.GetMwmId(), projection.m_segment.GetFeatureId())
            .IsOneWay();
    if (!strictForward && !oneWay)
    {
      auto const backwardSegment = GetReverseSegment(projection.m_segment);
      FakeVertex backwardPartOfReal(isStart ? projection.m_junction : frontJunction,
                                    isStart ? backJunction : projection.m_junction,
                                    FakeVertex::Type::PartOfReal);
      auto const fakeBackwardSegment = m_fake.GetSegment(backwardPartOfReal, fakeNumerationStart);
      m_fake.AddFakeVertex(projectionSegment, fakeBackwardSegment, backwardPartOfReal,
                           isStart /* isOutgoing */, true /* isPartOfReal */, backwardSegment);
    }
  }
}

void IndexGraphStarter::AddStart(FakeEnding const & startEnding, FakeEnding const & finishEnding,
                                 bool strictForward, uint32_t & fakeNumerationStart)
{
  AddEnding(startEnding, finishEnding, true /* isStart */, strictForward, fakeNumerationStart);
  }

  void IndexGraphStarter::AddFinish(FakeEnding const & finishEnding, FakeEnding const & startEnding,
                                    uint32_t & fakeNumerationStart)
  {
    AddEnding(finishEnding, startEnding, false /* isStart */, false /* strictForward */,
              fakeNumerationStart);
  }

  void IndexGraphStarter::AddFakeEdges(Segment const & segment, vector<SegmentEdge> & edges) const
  {
    vector<SegmentEdge> fakeEdges;
    for (auto const & edge : edges)
    {
      auto const it = m_fake.m_realToFake.find(edge.GetTarget());
      if (it == m_fake.m_realToFake.end())
        continue;
      for (auto const & s : it->second)
      {
        // Check fake segment is connected to source segment.
        if (GetJunction(s, false /* front */) == GetJunction(segment, true) ||
            GetJunction(s, true) == GetJunction(segment, false))
        {
          fakeEdges.emplace_back(s, edge.GetWeight());
        }
      }
    }
    edges.insert(edges.end(), fakeEdges.begin(), fakeEdges.end());
}

void IndexGraphStarter::AddRealEdges(Segment const & segment, bool isOutgoing,
                                     std::vector<SegmentEdge> & edges) const
{
  bool const isEnding = m_fake.m_realToFake.find(segment) != m_fake.m_realToFake.end();
  m_graph.GetEdgeList(segment, isOutgoing, IsLeap(segment.GetMwmId()), isEnding, edges);
}
}  // namespace routing
