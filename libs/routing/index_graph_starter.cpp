#include "routing/index_graph_starter.hpp"

#include "routing/fake_edges_container.hpp"
#include "routing/regions_sparse_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "coding/point_coding.hpp"

#include "geometry/distance_on_sphere.hpp"

#include <algorithm>
#include <map>

namespace routing
{
using namespace std;

// IndexGraphStarter::Ending -----------------------------------------------------------------------
void IndexGraphStarter::Ending::FillMwmIds()
{
  for (auto const & segment : m_real)
    m_mwmIds.emplace(segment.GetMwmId());
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

IndexGraphStarter::IndexGraphStarter(FakeEnding const & startEnding, FakeEnding const & finishEnding,
                                     uint32_t fakeNumerationStart, bool strictForward, WorldGraph & graph)
  : m_graph(graph)
{
  m_fakeNumerationStart = fakeNumerationStart;

  m_start.m_id = m_fakeNumerationStart;
  AddStart(startEnding, finishEnding, strictForward);
  m_finish.m_id = m_fakeNumerationStart;
  AddFinish(finishEnding, startEnding);

  m_otherEndings.push_back(startEnding);
  m_otherEndings.push_back(finishEnding);

  auto const startPoint = GetPoint(GetStartSegment(), false /* front */);
  auto const finishPoint = GetPoint(GetFinishSegment(), true /* front */);
  m_startToFinishDistanceM = ms::DistanceOnEarth(startPoint, finishPoint);
}

void IndexGraphStarter::Append(FakeEdgesContainer const & container)
{
  m_finish = container.m_finish;
  m_fake.Append(container.m_fake);

  // It's important to calculate distance after m_fake.Append() because
  // we don't have finish segment in fake graph before m_fake.Append().
  auto const startPoint = GetPoint(GetStartSegment(), false /* front */);
  auto const finishPoint = GetPoint(GetFinishSegment(), true /* front */);
  m_startToFinishDistanceM = ms::DistanceOnEarth(startPoint, finishPoint);
  m_fakeNumerationStart += container.m_fake.GetSize();
}

void IndexGraphStarter::SetGuides(GuidesGraph const & guides)
{
  m_guides = guides;
}

void IndexGraphStarter::SetRegionsGraphMode(std::shared_ptr<RegionsSparseGraph> regionsSparseGraph)
{
  m_regionsGraph = std::move(regionsSparseGraph);
  m_graph.SetRegionsGraphMode(true);
}

LatLonWithAltitude const & IndexGraphStarter::GetStartJunction() const
{
  auto const & startSegment = GetStartSegment();
  return m_fake.GetVertex(startSegment).GetJunctionFrom();
}

LatLonWithAltitude const & IndexGraphStarter::GetFinishJunction() const
{
  auto const & finishSegment = GetFinishSegment();
  return m_fake.GetVertex(finishSegment).GetJunctionTo();
}

bool IndexGraphStarter::ConvertToReal(Segment & segment) const
{
  return IsFakeSegment(segment) ? m_fake.FindReal(segment, segment) : true;
}

LatLonWithAltitude const & IndexGraphStarter::GetJunction(Segment const & segment, bool front) const
{
  if (IsRegionsGraphMode() && !IsFakeSegment(segment))
    return m_regionsGraph->GetJunction(segment, front);

  if (IsGuidesSegment(segment))
    return m_guides.GetJunction(segment, front);

  if (!IsFakeSegment(segment))
    return m_graph.GetJunction(segment, front);

  auto const & vertex = m_fake.GetVertex(segment);
  return front ? vertex.GetJunctionTo() : vertex.GetJunctionFrom();
}

LatLonWithAltitude const & IndexGraphStarter::GetRouteJunction(vector<Segment> const & segments,
                                                               size_t pointIndex) const
{
  CHECK(!segments.empty(), ());
  CHECK_LESS_OR_EQUAL(pointIndex, segments.size(),
                      ("Point with index", pointIndex, "does not exist in route with size", segments.size()));
  if (pointIndex == segments.size())
    return GetJunction(segments[pointIndex - 1], true /* front */);
  return GetJunction(segments[pointIndex], false);
}

ms::LatLon const & IndexGraphStarter::GetPoint(Segment const & segment, bool front) const
{
  return GetJunction(segment, front).GetLatLon();
}

bool IndexGraphStarter::IsRoutingOptionsGood(Segment const & segment) const
{
  return m_graph.IsRoutingOptionsGood(segment);
}

RoutingOptions IndexGraphStarter::GetRoutingOptions(Segment const & segment) const
{
  if (segment.IsRealSegment())
    return m_graph.GetRoutingOptions(segment);

  Segment real;
  if (!m_fake.FindReal(segment, real))
    return {};

  return m_graph.GetRoutingOptions(real);
}

set<NumMwmId> IndexGraphStarter::GetMwms() const
{
  set<NumMwmId> mwms;
  mwms.insert(m_start.m_mwmIds.begin(), m_start.m_mwmIds.end());
  mwms.insert(m_finish.m_mwmIds.begin(), m_finish.m_mwmIds.end());
  return mwms;
}

bool IndexGraphStarter::CheckLength(RouteWeight const & weight)
{
  // Avoid transit through living neighbourhoods.
  // Assume that start or end belongs to the living neighbourhood (HasNoPassThroughAllowed),
  // if at least one of the projections is non-pass-through road (service, living_street).
  // Interesting example here: https://github.com/organicmaps/organicmaps/issues/1008

  // Allow pass-through <-> non-pass-through zone changes if start or end belongs
  // to no-pass-through zone (have to leave or arrive at living neighbourhood).

  /// @todo By VNG: Additional 2 changes to relax this restriction. Or should enchance this mechanics to allow
  /// pass through say living_street into lower class road (track/road/unclassified?).
  /// This additional penalty will be included in RouteWeight::GetIntegratedWeight().
  /// @see RussiaMoscowNotCrossingTollRoadTest

  int8_t const changesAllowed =
      2 + (HasNoPassThroughAllowed(m_start) ? 1 : 0) + (HasNoPassThroughAllowed(m_finish) ? 1 : 0);

  return weight.GetNumPassThroughChanges() <= changesAllowed && m_graph.CheckLength(weight, m_startToFinishDistanceM);
}

void IndexGraphStarter::GetEdgesList(astar::VertexData<Vertex, Weight> const & vertexData, bool isOutgoing,
                                     bool useAccessConditional, EdgeListT & edges) const
{
  edges.clear();
  CHECK_NOT_EQUAL(m_graph.GetMode(), WorldGraphMode::LeapsOnly, ());

  auto const & segment = vertexData.m_vertex;
  // Weight used only when isOutgoing = false for passing to m_guides and placing to |edges|.
  RouteWeight ingoingSegmentWeight;
  if (!isOutgoing)
    ingoingSegmentWeight = CalcSegmentWeight(segment, EdgeEstimator::Purpose::Weight);

  if (IsFakeSegment(segment))
  {
    Segment real;
    if (m_fake.FindReal(segment, real))
    {
      bool const haveSameFront = GetJunction(segment, true /* front */) == GetJunction(real, true);
      bool const haveSameBack = GetJunction(segment, false /* front */) == GetJunction(real, false);
      if ((isOutgoing && haveSameFront) || (!isOutgoing && haveSameBack))
      {
        if (IsGuidesSegment(real))
        {
          m_guides.GetEdgeList(real, isOutgoing, edges, ingoingSegmentWeight);
        }
        else if (IsRegionsGraphMode())
        {
          m_regionsGraph->GetEdgeList(real, isOutgoing, edges, GetJunction(segment, true /* front */).GetLatLon());
        }
        else
        {
          astar::VertexData const replacedFakeSegment(real, vertexData.m_realDistance);
          m_graph.GetEdgeList(replacedFakeSegment, isOutgoing, true /* useRoutingOptions */, useAccessConditional,
                              edges);
        }
      }
    }

    for (auto const & s : m_fake.GetEdges(segment, isOutgoing))
      edges.emplace_back(s, isOutgoing ? CalcSegmentWeight(s, EdgeEstimator::Purpose::Weight) : ingoingSegmentWeight);
  }
  else if (IsGuidesSegment(segment))
  {
    m_guides.GetEdgeList(segment, isOutgoing, edges, ingoingSegmentWeight);
  }
  else if (IsRegionsGraphMode())
  {
    m_regionsGraph->GetEdgeList(segment, isOutgoing, edges, GetJunction(segment, true /* front */).GetLatLon());
  }
  else
  {
    m_graph.GetEdgeList(vertexData, isOutgoing, true /* useRoutingOptions */, useAccessConditional, edges);
  }

  AddFakeEdges(segment, isOutgoing, edges);
}

RouteWeight IndexGraphStarter::CalcGuidesSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) const
{
  if (purpose == EdgeEstimator::Purpose::Weight)
    return m_guides.CalcSegmentWeight(segment);

  auto [from, to] = m_guides.GetFromTo(segment);
  return m_graph.CalcOffroadWeight(from.GetLatLon(), to.GetLatLon(), purpose);
}

RouteWeight IndexGraphStarter::CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) const
{
  if (IsGuidesSegment(segment))
    return CalcGuidesSegmentWeight(segment, purpose);

  if (IsRegionsGraphMode() && !IsFakeSegment(segment))
    return m_regionsGraph->CalcSegmentWeight(segment);

  if (!IsFakeSegment(segment))
    return m_graph.CalcSegmentWeight(segment, purpose);

  auto const & vertex = m_fake.GetVertex(segment);
  Segment real;
  if (m_fake.FindReal(segment, real))
  {
    double const partLen = ms::DistanceOnEarth(vertex.GetPointFrom(), vertex.GetPointTo());

    if (IsRegionsGraphMode())
      return RouteWeight(partLen);

    double const fullLen = ms::DistanceOnEarth(GetPoint(real, false), GetPoint(real, true));
    // Note 1. |fullLen| == 0.0 in case of Segment(s) with the same ends.
    // Note 2. There is the following logic behind |return 0.0 * m_graph.CalcSegmentWeight(real, ...);|:
    // it's necessary to return a instance of the structure |RouteWeight| with zero wight.
    // Theoretically it may be differ from |RouteWeight(0)| because some road access block
    // may be kept in it and it is up to |RouteWeight| to know how to multiply by zero.

    Weight weight;
    if (IsGuidesSegment(real))
      weight = CalcGuidesSegmentWeight(real, purpose);
    else
      weight = m_graph.CalcSegmentWeight(real, purpose);
    if (fullLen == 0.0)
      return 0.0 * weight;

    return (partLen / fullLen) * weight;
  }

  return m_graph.CalcOffroadWeight(vertex.GetPointFrom(), vertex.GetPointTo(), purpose);
}

double IndexGraphStarter::CalculateETA(Segment const & from, Segment const & to) const
{
  // We don't distinguish fake segment weight and fake segment transit time.
  if (IsFakeSegment(to))
    return CalcSegmentWeight(to, EdgeEstimator::Purpose::ETA).GetWeight();

  if (IsFakeSegment(from))
    return CalculateETAWithoutPenalty(to);

  if (IsGuidesSegment(from) || IsGuidesSegment(to))
  {
    double res = 0.0;
    if (IsGuidesSegment(from))
      res += CalcGuidesSegmentWeight(from, EdgeEstimator::Purpose::ETA).GetWeight();
    else
      res += CalculateETAWithoutPenalty(from);

    if (IsGuidesSegment(to))
      res += CalcGuidesSegmentWeight(to, EdgeEstimator::Purpose::ETA).GetWeight();
    else
      res += CalculateETAWithoutPenalty(to);

    return res;
  }

  if (IsRegionsGraphMode())
    return m_regionsGraph->CalcSegmentWeight(from).GetWeight() + m_regionsGraph->CalcSegmentWeight(to).GetWeight();

  return m_graph.CalculateETA(from, to);
}

double IndexGraphStarter::CalculateETAWithoutPenalty(Segment const & segment) const
{
  if (IsFakeSegment(segment))
    return CalcSegmentWeight(segment, EdgeEstimator::Purpose::ETA).GetWeight();

  if (IsGuidesSegment(segment))
    return CalcGuidesSegmentWeight(segment, EdgeEstimator::Purpose::ETA).GetWeight();

  if (IsRegionsGraphMode())
    return m_regionsGraph->CalcSegmentWeight(segment).GetWeight();

  return m_graph.CalculateETAWithoutPenalty(segment);
}

void IndexGraphStarter::AddEnding(FakeEnding const & thisEnding)
{
  Segment const dummy = Segment();

  map<Segment, vector<LatLonWithAltitude>> otherSegments;
  for (auto const & ending : m_otherEndings)
  {
    for (auto const & p : ending.m_projections)
    {
      otherSegments[p.m_segment].push_back(p.m_junction);
      // We use |otherEnding| to generate proper fake edges in case both endings have projections
      // to the same segment. Direction of p.m_segment does not matter.
      otherSegments[p.m_segment.GetReversed()].push_back(p.m_junction);
    }
  }

  // Add pure fake vertex
  auto const fakeSegment = GetFakeSegmentAndIncr();
  FakeVertex fakeVertex(kFakeNumMwmId, thisEnding.m_originJunction, thisEnding.m_originJunction,
                        FakeVertex::Type::PureFake);
  m_fake.AddStandaloneVertex(fakeSegment, fakeVertex);
  for (bool isStart : {true, false})
  {
    for (auto const & projection : thisEnding.m_projections)
    {
      // Add projection edges
      auto const projectionSegment = GetFakeSegmentAndIncr();
      FakeVertex projectionVertex(
          projection.m_segment.GetMwmId(), isStart ? thisEnding.m_originJunction : projection.m_junction,
          isStart ? projection.m_junction : thisEnding.m_originJunction, FakeVertex::Type::PureFake);
      m_fake.AddVertex(fakeSegment, projectionSegment, projectionVertex, isStart /* isOutgoing */,
                       false /* isPartOfReal */, dummy /* realSegment */);

      // Add fake parts of real
      auto frontJunction = projection.m_segmentFront;
      auto backJunction = projection.m_segmentBack;

      // Check whether we have projections to same real segment from both endings.
      auto const it = otherSegments.find(projection.m_segment);
      if (it != otherSegments.end())
      {
        ASSERT(!it->second.empty(), ());

        LatLonWithAltitude otherJunction;
        double distBackToOther = 1.0E8;
        for (auto const & coord : it->second)
        {
          double const curDist = ms::DistanceOnEarth(backJunction.GetLatLon(), coord.GetLatLon());
          if (curDist < distBackToOther)
          {
            distBackToOther = curDist;
            otherJunction = coord;
          }
        }

        double const distBackToThis = ms::DistanceOnEarth(backJunction.GetLatLon(), projection.m_junction.GetLatLon());

        if (distBackToThis < distBackToOther)
          frontJunction = otherJunction;
        else if (distBackToOther < distBackToThis)
          backJunction = otherJunction;
        else
          frontJunction = backJunction = otherJunction;
      }

      FakeVertex forwardPartOfReal(projection.m_segment.GetMwmId(), isStart ? projection.m_junction : backJunction,
                                   isStart ? frontJunction : projection.m_junction, FakeVertex::Type::PartOfReal);
      Segment fakeForwardSegment;
      if (!m_fake.FindSegment(forwardPartOfReal, fakeForwardSegment))
        fakeForwardSegment = GetFakeSegmentAndIncr();
      m_fake.AddVertex(projectionSegment, fakeForwardSegment, forwardPartOfReal, isStart /* isOutgoing */,
                       true /* isPartOfReal */, projection.m_segment);

      if (!projection.m_isOneWay)
      {
        auto const backwardSegment = projection.m_segment.GetReversed();
        FakeVertex backwardPartOfReal(backwardSegment.GetMwmId(), isStart ? projection.m_junction : frontJunction,
                                      isStart ? backJunction : projection.m_junction, FakeVertex::Type::PartOfReal);
        Segment fakeBackwardSegment;
        if (!m_fake.FindSegment(backwardPartOfReal, fakeBackwardSegment))
          fakeBackwardSegment = GetFakeSegmentAndIncr();
        m_fake.AddVertex(projectionSegment, fakeBackwardSegment, backwardPartOfReal, isStart /* isOutgoing */,
                         true /* isPartOfReal */, backwardSegment);
      }
    }
  }

  m_otherEndings.push_back(thisEnding);
}

void IndexGraphStarter::AddEnding(FakeEnding const & thisEnding, FakeEnding const & otherEnding, bool isStart,
                                  bool strictForward)
{
  Segment const dummy = Segment();

  map<Segment, LatLonWithAltitude> otherSegments;
  for (auto const & p : otherEnding.m_projections)
  {
    otherSegments[p.m_segment] = p.m_junction;
    // We use |otherEnding| to generate proper fake edges in case both endings have projections
    // to the same segment. Direction of p.m_segment does not matter.
    otherSegments[p.m_segment.GetReversed()] = p.m_junction;
  }

  // Add pure fake vertex
  auto const fakeSegment = GetFakeSegmentAndIncr();
  FakeVertex fakeVertex(kFakeNumMwmId, thisEnding.m_originJunction, thisEnding.m_originJunction,
                        FakeVertex::Type::PureFake);
  m_fake.AddStandaloneVertex(fakeSegment, fakeVertex);
  for (auto const & projection : thisEnding.m_projections)
  {
    if (isStart)
      m_start.m_real.insert(projection.m_segment);
    else
      m_finish.m_real.insert(projection.m_segment);

    // Add projection edges
    auto const projectionSegment = GetFakeSegmentAndIncr();
    FakeVertex projectionVertex(
        projection.m_segment.GetMwmId(), isStart ? thisEnding.m_originJunction : projection.m_junction,
        isStart ? projection.m_junction : thisEnding.m_originJunction, FakeVertex::Type::PureFake);
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
      auto const distBackToThis = ms::DistanceOnEarth(backJunction.GetLatLon(), projection.m_junction.GetLatLon());
      auto const distBackToOther = ms::DistanceOnEarth(backJunction.GetLatLon(), otherJunction.GetLatLon());
      if (distBackToThis < distBackToOther)
        frontJunction = otherJunction;
      else if (distBackToOther < distBackToThis)
        backJunction = otherJunction;
      else
        frontJunction = backJunction = otherJunction;
    }

    FakeVertex forwardPartOfReal(projection.m_segment.GetMwmId(), isStart ? projection.m_junction : backJunction,
                                 isStart ? frontJunction : projection.m_junction, FakeVertex::Type::PartOfReal);
    Segment fakeForwardSegment;
    if (!m_fake.FindSegment(forwardPartOfReal, fakeForwardSegment))
      fakeForwardSegment = GetFakeSegmentAndIncr();
    m_fake.AddVertex(projectionSegment, fakeForwardSegment, forwardPartOfReal, isStart /* isOutgoing */,
                     true /* isPartOfReal */, projection.m_segment);

    if (!strictForward && !projection.m_isOneWay)
    {
      auto const backwardSegment = projection.m_segment.GetReversed();
      FakeVertex backwardPartOfReal(backwardSegment.GetMwmId(), isStart ? projection.m_junction : frontJunction,
                                    isStart ? backJunction : projection.m_junction, FakeVertex::Type::PartOfReal);
      Segment fakeBackwardSegment;
      if (!m_fake.FindSegment(backwardPartOfReal, fakeBackwardSegment))
        fakeBackwardSegment = GetFakeSegmentAndIncr();
      m_fake.AddVertex(projectionSegment, fakeBackwardSegment, backwardPartOfReal, isStart /* isOutgoing */,
                       true /* isPartOfReal */, backwardSegment);
    }
  }
}

void IndexGraphStarter::ConnectLoopToGuideSegments(FakeVertex const & loop, Segment const & realSegment,
                                                   LatLonWithAltitude realFrom, LatLonWithAltitude realTo,
                                                   vector<pair<FakeVertex, Segment>> const & partsOfReal)
{
  m_fake.ConnectLoopToGuideSegments(loop, realSegment, realFrom, realTo, partsOfReal);
}

HighwayCategory IndexGraphStarter::GetHighwayCategory(Segment seg) const
{
  if (IsFakeSegment(seg))
  {
    Segment real;
    if (m_fake.FindReal(seg, real))
      seg = real;
    else
      return HighwayCategory::Unknown;
  }

  auto const hwType = m_graph.GetIndexGraph(seg.GetMwmId()).GetRoadGeometry(seg.GetFeatureId()).GetHighwayType();
  if (!hwType)
    return HighwayCategory::Unknown;

  switch (*hwType)
  {
  case HighwayType::HighwayMotorway:
  case HighwayType::HighwayMotorwayLink:
  case HighwayType::HighwayTrunk:
  case HighwayType::HighwayTrunkLink: return HighwayCategory::Major;
  case HighwayType::HighwayPrimary:
  case HighwayType::HighwayPrimaryLink: return HighwayCategory::Primary;
  case HighwayType::HighwaySecondary:
  case HighwayType::HighwaySecondaryLink:
  case HighwayType::HighwayTertiary:
  case HighwayType::HighwayTertiaryLink: return HighwayCategory::Usual;
  case HighwayType::RouteFerry:
  case HighwayType::RouteShuttleTrain: return HighwayCategory::Transit;
  default: return HighwayCategory::Minor;
  }
}

void IndexGraphStarter::AddStart(FakeEnding const & startEnding, FakeEnding const & finishEnding, bool strictForward)
{
  AddEnding(startEnding, finishEnding, true /* isStart */, strictForward);
  m_start.FillMwmIds();
}

void IndexGraphStarter::AddFinish(FakeEnding const & finishEnding, FakeEnding const & startEnding)
{
  AddEnding(finishEnding, startEnding, false /* isStart */, false /* strictForward */);
  m_finish.FillMwmIds();
}

void IndexGraphStarter::AddFakeEdges(Segment const & segment, bool isOutgoing, EdgeListT & edges) const
{
  EdgeListT fakeEdges;
  for (auto const & edge : edges)
  {
    for (auto const & s : m_fake.GetFake(edge.GetTarget()))
    {
      //     |segment|       |s|
      //  *------------>*----------->
      bool const sIsOutgoing = GetJunction(segment, true /* front */) == GetJunction(s, false /* front */);

      //        |s|       |segment|
      //  *------------>*----------->
      bool const sIsIngoing = GetJunction(s, true /* front */) == GetJunction(segment, false /* front */);

      if ((isOutgoing && sIsOutgoing) || (!isOutgoing && sIsIngoing))
      {
        // For ingoing edges we use source weight which is the same for |s| and for |edge| and is
        // already calculated.
        fakeEdges.emplace_back(s, isOutgoing ? CalcSegmentWeight(s, EdgeEstimator::Purpose::Weight) : edge.GetWeight());
      }
    }
  }
  edges.append(fakeEdges.begin(), fakeEdges.end());
}

bool IndexGraphStarter::HasNoPassThroughAllowed(Ending const & ending) const
{
  if (IsRegionsGraphMode())
    return false;

  return any_of(ending.m_real.cbegin(), ending.m_real.cend(), [this](Segment const & s)
  {
    if (IsGuidesSegment(s))
      return false;

    return !m_graph.IsPassThroughAllowed(s.GetMwmId(), s.GetFeatureId());
  });
}

Segment IndexGraphStarter::GetFakeSegmentAndIncr()
{
  CHECK_LESS(m_fakeNumerationStart, numeric_limits<uint32_t>::max(), ());
  return GetFakeSegment(m_fakeNumerationStart++);
}

RouteWeight IndexGraphStarter::GetAStarWeightEpsilon()
{
  // Epsilon for double calculations.
  auto constexpr kEps = RouteWeight(1e-6);
  // We store points with |kMwmPointAccuracy|. In case of cross mwm routing we couldn't
  // distinguish the point geometry changing in |kMwmPointAccuracy| radius of the same segments from
  // mwms with different versions. So let's use such epsilon to maintain the A* invariant.
  return kEps + m_graph.HeuristicCostEstimate(ms::LatLon(0.0, 0.0), ms::LatLon(0.0, kMwmPointAccuracy));
}
}  // namespace routing
