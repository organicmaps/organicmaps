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

void FillNumMwmIds(set<Segment> const & segments, set<NumMwmId> & mwms)
{
  for (auto const & s : segments)
    mwms.insert(s.GetMwmId());
}
}  // namespace

namespace routing
{
// IndexGraphStarter::Ending -----------------------------------------------------------------------
bool IndexGraphStarter::Ending::OverlapsWithMwm(NumMwmId mwmId) const
{
  auto containsSegment = [mwmId](Segment const & s) { return s.GetMwmId() == mwmId; };
  return any_of(m_real.cbegin(), m_real.end(), containsSegment);
}

// IndexGraphStarter -------------------------------------------------------------------------------
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
  FillNumMwmIds(m_start.m_real, mwms);
  FillNumMwmIds(m_finish.m_real, mwms);
  return mwms;
}

set<NumMwmId> IndexGraphStarter::GetStartMwms() const
{
  set<NumMwmId> mwms;
  FillNumMwmIds(m_start.m_real, mwms);
  return mwms;
}

set<NumMwmId> IndexGraphStarter::GetFinishMwms() const
{
  set<NumMwmId> mwms;
  FillNumMwmIds(m_finish.m_real, mwms);
  return mwms;
}

bool IndexGraphStarter::CheckLength(RouteWeight const & weight)
{
  // We allow 1 pass-through/non-pass-through zone changes per ending located in
  // non-pass-through zone to allow user to leave this zone.
  int8_t const numPassThroughChangesAllowed =
      (StartPassThroughAllowed() ? 0 : 1) + (FinishPassThroughAllowed() ? 0 : 1);

  return weight.GetNumPassThroughChanges() <= numPassThroughChangesAllowed &&
         m_graph.CheckLength(weight, m_startToFinishDistanceM);
}

void IndexGraphStarter::GetEdgesList(Segment const & segment, bool isOutgoing,
                                     vector<SegmentEdge> & edges) const
{
  edges.clear();

  // If mode is LeapsOnly we need to connect start/finish segment to transitions.
  if (m_graph.GetMode() == WorldGraph::Mode::LeapsOnly)
  {
    // Ingoing edges listing is not supported in LeapsOnly mode because we do not have enough
    // information to calculate |segment| weight. See https://jira.mail.ru/browse/MAPSME-5743 for details.
    CHECK(isOutgoing, ("Ingoing edges listing is not supported in LeapsOnly mode."));
    m2::PointD const & segmentPoint = GetPoint(segment, true /* front */);
    if (segment == GetStartSegment())
    {
      set<NumMwmId> seen;
      for (auto const & real : m_start.m_real)
      {
        auto const mwmId = real.GetMwmId();
        if (seen.insert(mwmId).second)
        {
          // Connect start to all exits (|isEnter| == false).
          for (auto const & s : m_graph.GetTransitions(mwmId, false /* isEnter */))
            edges.emplace_back(s, m_graph.CalcLeapWeight(segmentPoint, GetPoint(s, true /* front */)));
        }
      }
      return;
    }

    // Edge from finish mwm enter to finish.
    if (m_finish.OverlapsWithMwm(segment.GetMwmId()))
    {
      edges.emplace_back(GetFinishSegment(), m_graph.CalcLeapWeight(
                                             segmentPoint, GetPoint(GetFinishSegment(), true /* front */ )));
      return;
    }

    m_graph.GetEdgeList(segment, isOutgoing, edges);
    return;
  }

  if (IsFakeSegment(segment))
  {
    Segment real;
    if (m_fake.FindReal(segment, real))
    {
      bool const haveSameFront = GetJunction(segment, true /* front */) == GetJunction(real, true);
      bool const haveSameBack = GetJunction(segment, false /* front */) == GetJunction(real, false);
      if ((isOutgoing && haveSameFront) || (!isOutgoing && haveSameBack))
        m_graph.GetEdgeList(real, isOutgoing, edges);
    }

    for (auto const & s : m_fake.GetEdges(segment, isOutgoing))
      edges.emplace_back(s, CalcSegmentWeight(isOutgoing ? s : segment));
  }
  else
  {
    m_graph.GetEdgeList(segment, isOutgoing, edges);
  }

  AddFakeEdges(segment, isOutgoing, edges);
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
    // Note. |fullLen| == 0.0 in case of Segment(s) with the same ends.
    if (fullLen == 0.0)
      return 0.0 * m_graph.CalcSegmentWeight(real);

    return (partLen / fullLen) * m_graph.CalcSegmentWeight(real);
  }

  return m_graph.CalcOffroadWeight(vertex.GetPointFrom(), vertex.GetPointTo());
}

double IndexGraphStarter::CalcSegmentETA(Segment const & segment) const
{
  // We don't distinguish fake segment weight and fake segment transit time.
  if (IsFakeSegment(segment))
    return CalcSegmentWeight(segment).GetWeight();

  return m_graph.CalcSegmentETA(segment);
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
      else if (distBackToOther < distBackToThis)
        backJunction = otherJunction;
      else
        frontJunction = backJunction = otherJunction;
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

void IndexGraphStarter::AddFakeEdges(Segment const & segment, bool isOutgoing, vector<SegmentEdge> & edges) const
{
  vector<SegmentEdge> fakeEdges;
  for (auto const & edge : edges)
  {
    for (auto const & s : m_fake.GetFake(edge.GetTarget()))
    {
      //     |segment|       |s|
      //  *------------>*----------->
      bool const sIsOutgoing =
          GetJunction(segment, true /*front */) == GetJunction(s, false /* front */);

      //        |s|       |segment|
      //  *------------>*----------->
      bool const sIsIngoing =
          GetJunction(s, true /*front */) == GetJunction(segment, false /* front */);

      if ((isOutgoing && sIsOutgoing) || (!isOutgoing && sIsIngoing))
      {
        // For ingoing edges we use source weight which is the same for |s| and for |edge| and is
        // already calculated.
        fakeEdges.emplace_back(s, isOutgoing ? CalcSegmentWeight(s) : edge.GetWeight());
      }
    }
  }
  edges.insert(edges.end(), fakeEdges.begin(), fakeEdges.end());
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
}  // namespace routing
