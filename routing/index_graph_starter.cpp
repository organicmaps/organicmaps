#include "routing/index_graph_starter.hpp"

#include "routing/fake_edges_container.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
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
  m_start.m_id = fakeNumerationStart;
  AddStart(startEnding, finishEnding, strictForward, fakeNumerationStart);
  m_finish.m_id = fakeNumerationStart;
  AddFinish(finishEnding, startEnding, fakeNumerationStart);
  auto const startPoint = GetPoint(GetStartSegment(), false /* front */);
  auto const finishPoint = GetPoint(GetFinishSegment(), true /* front */);
  m_startToFinishDistanceM = MercatorBounds::DistanceOnEarth(startPoint, finishPoint);
}

void IndexGraphStarter::Append(FakeEdgesContainer const & container)
{
  m_finish = container.m_finish;
  m_fake.Append(container.m_fake);

  // It's important to calculate distance after m_fake.Append() because
  // we don't have finish segment in fake graph before m_fake.Append().
  auto const startPoint = GetPoint(GetStartSegment(), false /* front */);
  auto const finishPoint = GetPoint(GetFinishSegment(), true /* front */);
  m_startToFinishDistanceM = MercatorBounds::DistanceOnEarth(startPoint, finishPoint);
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
  for (auto const & s : m_start.m_real)
    mwms.insert(s.GetMwmId());
  for (auto const & s : m_finish.m_real)
    mwms.insert(s.GetMwmId());

  return mwms;
}

bool IndexGraphStarter::CheckLength(RouteWeight const & weight)
{
  // We allow 1 pass-through/non-pass-through crossing per ending located in
  // non-pass-through zone to allow user to leave this zone.
  int const nonPassThroughCrossAllowed =
      (StartPassThroughAllowed() ? 0 : 1) + (FinishPassThroughAllowed() ? 0 : 1);

  return weight.GetNonPassThroughCross() <= nonPassThroughCrossAllowed &&
         m_graph.CheckLength(weight, m_startToFinishDistanceM);
}

void IndexGraphStarter::GetEdgesList(Segment const & segment, bool isOutgoing,
                                     vector<SegmentEdge> & edges) const
{
  edges.clear();

  // If mode is LeapsOnly we need to connect start/finish segment to transitions.
  if (m_graph.GetMode() == WorldGraph::Mode::LeapsOnly)
  {
     m2::PointD const & segmentPoint = GetPoint(segment, true /* front */);
     if ((segment == GetStartSegment() && isOutgoing) || (segment == GetFinishSegment() && !isOutgoing))
     {
       // Note. If |isOutgoing| == true it's necessary to add edges which connect the start with all
       // exits of its mwm. So |isEnter| below should be set to false.
       // If |isOutgoing| == false all enters of the finish mwm should be connected with the finish
       // point. So |isEnter| below should be set to true.
       auto const mwms = GetEndingMwms(isOutgoing ? m_start : m_finish);
       for (auto const & mwm : mwms)
       {
         for (auto const & s : m_graph.GetTransitions(mwm, !isOutgoing /* isEnter */))
         {
           // @todo(t.yan) fix weight for ingoing edges https://jira.mail.ru/browse/MAPSME-5953
           edges.emplace_back(s, m_graph.CalcLeapWeight(segmentPoint, GetPoint(s, isOutgoing)));
         }
       }
       return;
     }
     // Edge from finish mwm enter to finish.
     if (GetSegmentLocation(segment) == SegmentLocation::FinishMwm && isOutgoing)
     {
       edges.emplace_back(GetFinishSegment(), m_graph.CalcLeapWeight(
                                              segmentPoint, GetPoint(GetFinishSegment(), true /* front */ )));
       return;
     }
     // Ingoing edge from start mwm exit to start.
     if (GetSegmentLocation(segment) == SegmentLocation::StartMwm && !isOutgoing)
     {
       // @todo(t.yan) fix weight for ingoing edges https://jira.mail.ru/browse/MAPSME-5953
       edges.emplace_back(
           GetStartSegment(),
           m_graph.CalcLeapWeight(segmentPoint, GetPoint(GetStartSegment(), true /* front */)));
       return;
     }
  }

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
  Segment real;
  if (m_fake.FindReal(segment, real))
  {
    auto const partLen = MercatorBounds::DistanceOnEarth(vertex.GetPointFrom(), vertex.GetPointTo());
    auto const fullLen = MercatorBounds::DistanceOnEarth(GetPoint(real, false /* front */),
                                                         GetPoint(real, true /* front */));
    CHECK_GREATER(fullLen, 0.0, ());
    return partLen / fullLen * m_graph.CalcSegmentWeight(real);
  }

  return m_graph.CalcOffroadWeight(vertex.GetPointFrom(), vertex.GetPointTo());
}

RouteWeight IndexGraphStarter::CalcRouteSegmentWeight(vector<Segment> const & route,
                                                      size_t segmentIndex) const
{
  CHECK_LESS(
      segmentIndex, route.size(),
      ("Segment with index", segmentIndex, "does not exist in route with size", route.size()));
  return CalcSegmentWeight(route[segmentIndex]);
}

bool IndexGraphStarter::IsLeap(Segment const & segment) const
{
  return !IsFakeSegment(segment) && GetSegmentLocation(segment) == SegmentLocation::OtherMwm &&
         m_graph.LeapIsAllowed(segment.GetMwmId());
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
    if (isStart)
      m_start.m_real.insert(projection.m_segment);
    else
      m_finish.m_real.insert(projection.m_segment);

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
      auto const distBackToThis = MercatorBounds::DistanceOnEarth(backJunction.GetPoint(),
                                                                  projection.m_junction.GetPoint());
      auto const distBackToOther = MercatorBounds::DistanceOnEarth(backJunction.GetPoint(),
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
  m_graph.GetEdgeList(segment, isOutgoing, IsLeap(segment), edges);
}

bool IndexGraphStarter::EndingPassThroughAllowed(Ending const & ending)
{
  return any_of(ending.m_real.cbegin(), ending.m_real.cend(),
                [this](Segment const & s) {
                  return m_graph.IsPassThroughAllowed(s.GetMwmId(),
                                                      s.GetFeatureId());
                });
}

bool IndexGraphStarter::StartPassThroughAllowed()
{
  return EndingPassThroughAllowed(m_start);
}

bool IndexGraphStarter::FinishPassThroughAllowed()
{
  return EndingPassThroughAllowed(m_finish);
}

set<NumMwmId> IndexGraphStarter::GetEndingMwms(Ending const & ending) const
{
  set<NumMwmId> res;
  for (auto const & s : ending.m_real)
    res.insert(s.GetMwmId());
  return res;
}

IndexGraphStarter::SegmentLocation IndexGraphStarter::GetSegmentLocation(Segment const & segment) const
{
  CHECK(!IsFakeSegment(segment),());
  auto const mwmId = segment.GetMwmId();
  auto containsSegment = [mwmId](Segment const & s) { return s.GetMwmId() == mwmId; };

  bool const sameMwmWithStart =
      any_of(m_start.m_real.cbegin(), m_start.m_real.end(), containsSegment);
  bool const sameMwmWithFinish =
      any_of(m_finish.m_real.cbegin(), m_finish.m_real.end(), containsSegment);
  if (sameMwmWithStart && sameMwmWithFinish)
    return SegmentLocation::StartFinishMwm;
  if (sameMwmWithStart)
    return SegmentLocation::StartMwm;
  if (sameMwmWithFinish)
    return SegmentLocation::FinishMwm;
  return SegmentLocation::OtherMwm;
}
}  // namespace routing
