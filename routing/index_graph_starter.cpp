#include "routing/index_graph_starter.hpp"

#include "routing/fake_edges_container.hpp"

#include <map>

namespace
{
using namespace routing;
using namespace std;

Segment GetReverseSegment(Segment const & segment)
{
  return Segment(segment.GetMwmId(), segment.GetFeatureId(), segment.GetSegmentIdx(),
                 !segment.IsForward());
}
}  // namespace

namespace routing
{
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
  auto const & startSegment = GetStartSegment();
  return m_fake.GetVertex(startSegment).GetJunctionFrom();
}

Junction const & IndexGraphStarter::GetFinishJunction() const
{
  auto const & finishSegment = GetFinishSegment();
  return m_fake.GetVertex(finishSegment).GetJunctionTo();
}

bool IndexGraphStarter::ConvertToReal(Segment & segment) const
{
  if (!IsFakeSegment(segment))
    return true;

  return m_fake.FindReal(Segment(segment), segment);
}

Junction const & IndexGraphStarter::GetJunction(Segment const & segment, bool front) const
{
  if (!IsFakeSegment(segment))
    return m_graph.GetJunction(segment, front);

  auto const & vertex = m_fake.GetVertex(segment);
  return front ? vertex.GetJunctionTo() : vertex.GetJunctionFrom();
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
  for (auto const & kv : m_fake.GetFakeToReal())
    mwms.insert(kv.second.GetMwmId());

  return mwms;
}

bool IndexGraphStarter::DoesRouteCrossNontransit(
    RoutingResult<Segment, RouteWeight> const & result) const
{
  int nontransitCrossAllowed = 0;
  auto isRealOrPart = [this](Segment const & segment) {
    if (!IsFakeSegment(segment))
      return true;
    Segment dummy;
    return m_fake.FindReal(segment, dummy);
  };
  auto isTransitAllowed = [this](Segment const & segment) {
    auto real = segment;
    bool const convertionResult = ConvertToReal(real);
    CHECK(convertionResult, ());
    return m_graph.IsTransitAllowed(real.GetMwmId(), real.GetFeatureId());
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
    Segment real;
    if (m_fake.FindReal(segment, real))
    {
      bool const haveSameFront = GetJunction(segment, true /* front */) == GetJunction(real, true);
      bool const haveSameBack = GetJunction(segment, false /* front */) == GetJunction(real, false);
      if ((isOutgoing && haveSameFront) || (!isOutgoing && haveSameBack))
        AddRealEdges(real, isOutgoing, edges);
    }

    for (auto const & s : m_fake.GetEdges(segment, isOutgoing))
      edges.emplace_back(s, CalcSegmentWeight(isOutgoing ? s : segment));
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

  auto const & vertex = m_fake.GetVertex(segment);
  return m_graph.CalcLeapWeight(vertex.GetPointFrom(), vertex.GetPointTo());
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

  for (auto const & kv : m_fake.GetFakeToReal())
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
  m_fake.AddStandaloneVertex(fakeSegment, fakeVertex);
  for (auto const & projection : thisEnding.m_projections)
  {
    // Add projection edges
    auto const projectionSegment = GetFakeSegmentAndIncr(fakeNumerationStart);
    FakeVertex projectionVertex(isStart ? thisEnding.m_originJunction : projection.m_junction,
                                isStart ? projection.m_junction : thisEnding.m_originJunction,
                                FakeVertex::Type::PureFake);
    m_fake.AddVertex(fakeSegment, projectionSegment, projectionVertex, isStart /* isOutgoing */,
                     false /* isPartOfReal */, dummy /* realSegment */);

    // Add fake parts of real
    auto frontJunction = projection.m_segmentFront;
    auto backJunction = projection.m_segmentBack;

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
    Segment fakeForwardSegment;
    if (!m_fake.FindSegment(forwardPartOfReal, fakeForwardSegment))
      fakeForwardSegment = GetFakeSegmentAndIncr(fakeNumerationStart);
    m_fake.AddVertex(projectionSegment, fakeForwardSegment, forwardPartOfReal,
                     isStart /* isOutgoing */, true /* isPartOfReal */, projection.m_segment);

    if (!strictForward && !projection.m_isOneWay)
    {
      auto const backwardSegment = GetReverseSegment(projection.m_segment);
      FakeVertex backwardPartOfReal(isStart ? projection.m_junction : frontJunction,
                                    isStart ? backJunction : projection.m_junction,
                                    FakeVertex::Type::PartOfReal);
      Segment fakeBackwardSegment;
      if (!m_fake.FindSegment(backwardPartOfReal, fakeBackwardSegment))
        fakeBackwardSegment = GetFakeSegmentAndIncr(fakeNumerationStart);
      m_fake.AddVertex(projectionSegment, fakeBackwardSegment, backwardPartOfReal,
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
    for (auto const & s : m_fake.GetFake(edge.GetTarget()))
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
  bool const isEnding = !m_fake.GetFake(segment).empty();
  m_graph.GetEdgeList(segment, isOutgoing, IsLeap(segment.GetMwmId()), isEnding, edges);
}
}  // namespace routing
