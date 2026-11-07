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
// IndexGraphStarter::Ending -----------------------------------------------------------------------
void IndexGraphStarter::Ending::FillMwmIds()
{
  for (auto const & segment : m_real)
    m_mwmIds.emplace(segment.GetMwmId());
}

// IndexGraphStarter -------------------------------------------------------------------------------
// static
void IndexGraphStarter::CheckValidRoute(std::vector<Segment> const & segments)
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
  if (!IsFakeSegment(segment))
    return true;
  auto const state = segment.GetPassThroughState();
  Segment real;
  if (!m_fake.FindReal(segment.WithoutPassThroughState(), real))
    return false;
  real.SetPassThroughState(state);
  segment = real;
  return true;
}

LatLonWithAltitude const & IndexGraphStarter::GetJunction(Segment const & segment, bool front) const
{
  if (IsRegionsGraphMode() && !IsFakeSegment(segment))
    return m_regionsGraph->GetJunction(segment, front);

  if (IsGuidesSegment(segment))
    return m_guides.GetJunction(segment.WithoutPassThroughState(), front);

  if (!IsFakeSegment(segment))
    return m_graph.GetJunction(segment, front);

  auto const & vertex = m_fake.GetVertex(segment.WithoutPassThroughState());
  return front ? vertex.GetJunctionTo() : vertex.GetJunctionFrom();
}

LatLonWithAltitude const & IndexGraphStarter::GetRouteJunction(std::vector<Segment> const & segments,
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
  if (!m_fake.FindReal(segment.WithoutPassThroughState(), real))
    return {};

  return m_graph.GetRoutingOptions(real);
}

std::set<NumMwmId> IndexGraphStarter::GetMwms() const
{
  std::set<NumMwmId> mwms;
  mwms.insert(m_start.m_mwmIds.begin(), m_start.m_mwmIds.end());
  mwms.insert(m_finish.m_mwmIds.begin(), m_finish.m_mwmIds.end());
  return mwms;
}

bool IndexGraphStarter::CheckLength(RouteWeight const & weight)
{
  // Allow one interior non-pass-through episode; the initial and final episodes are free.
  return weight.GetNumPassThroughChanges() <= 2 && m_graph.CheckLength(weight, m_startToFinishDistanceM);
}

void IndexGraphStarter::EnablePassThroughStateRouting(bool enable)
{
  m_passThroughStateRouting = enable;
}

SmallList<PassThroughState> IndexGraphStarter::GetPassThroughStates(bool isService)
{
  if (isService)
    return {PassThroughState::InitialService, PassThroughState::NoPaidExit, PassThroughState::OnePaidExit};
  return {PassThroughState::NoPaidExit, PassThroughState::OnePaidExit};
}

std::optional<PassThroughState> IndexGraphStarter::AdvancePassThroughState(PassThroughState state, bool exitsService)
{
  ASSERT_NOT_EQUAL(state, PassThroughState::None, ());
  if (!exitsService)
    return state;

  // The initial service episode and the final one are free. Only completed interior episodes count.
  switch (state)
  {
  case PassThroughState::InitialService: return PassThroughState::NoPaidExit;
  case PassThroughState::NoPaidExit: return PassThroughState::OnePaidExit;
  case PassThroughState::OnePaidExit: return std::nullopt;
  case PassThroughState::None: UNREACHABLE();
  }
  UNREACHABLE();
}

int8_t IndexGraphStarter::GetPassThroughPenalty(PassThroughState from, PassThroughState to)
{
  return from == PassThroughState::NoPaidExit && to == PassThroughState::OnePaidExit ? 2 : 0;
}

namespace
{
template <typename Append>
void ForEachPassThroughState(PassThroughState source, bool isOutgoing, bool exitsService, bool targetService,
                             Append && append)
{
  if (isOutgoing)
  {
    if (auto const next = IndexGraphStarter::AdvancePassThroughState(source, exitsService))
      append(*next, IndexGraphStarter::GetPassThroughPenalty(source, *next));
  }
  else
  {
    for (auto const state : IndexGraphStarter::GetPassThroughStates(targetService))
      if (IndexGraphStarter::AdvancePassThroughState(state, exitsService) == source)
        append(state, IndexGraphStarter::GetPassThroughPenalty(state, source));
  }
}
}  // namespace

void IndexGraphStarter::ApplyPassThroughState(Segment const & source, bool isOutgoing, EdgeListT & edges) const
{
  EdgeListT result;
  auto const start = GetStartSegment();
  auto const finish = GetFinishSegment();
  auto const physicalSource = source.WithoutPassThroughState();
  bool const sourceService = IsNoPassThroughAllowed(physicalSource);
  for (auto const & edge : edges)
  {
    auto const target = edge.GetTarget().WithoutPassThroughState();
    auto const from = isOutgoing ? physicalSource : target;
    auto const to = isOutgoing ? target : physicalSource;
    bool const targetService = IsNoPassThroughAllowed(target);
    bool const exitsService = isOutgoing ? sourceService && !targetService : targetService && !sourceService;
    auto const append = [&](PassThroughState state, int8_t charged)
    {
      auto labelled = target;
      labelled.SetPassThroughState(state);
      auto const raw = edge.GetWeight().GetNumPassThroughChanges();
      result.emplace_back(labelled, edge.GetWeight() + RouteWeight(0.0, charged - raw, 0, 0, 0.0));
    };
    if (from == start)
    {
      bool const toService = isOutgoing ? targetService : sourceService;
      auto const initial = toService ? PassThroughState::InitialService : PassThroughState::NoPaidExit;
      if (isOutgoing)
        append(initial, 0);
      else if (source.GetPassThroughState() == initial)
        append(PassThroughState::None, 0);
    }
    else if (to == finish)
    {
      if (isOutgoing)
        append(PassThroughState::None, 0);
      else
        for (auto const state : GetPassThroughStates(targetService))
          append(state, 0);
    }
    else
    {
      ForEachPassThroughState(source.GetPassThroughState(), isOutgoing, exitsService, targetService, append);
    }
  }
  edges = std::move(result);
}

void IndexGraphStarter::ApplyPassThroughState(Segment const & source, bool isOutgoing, JointEdgeListT & edges,
                                              WeightListT & parentWeights) const
{
  if (!m_passThroughStateRouting)
    return;
  JointEdgeListT result;
  WeightListT weights;
  bool const sourceService = IsNoPassThroughAllowed(source);
  ASSERT_EQUAL(edges.size(), parentWeights.size(), ());
  for (size_t i = 0; i < edges.size(); ++i)
  {
    ASSERT(!edges[i].GetTarget().IsFake(), (edges[i].GetTarget()));
    auto const target = edges[i].GetTarget().GetSegment(true);
    bool const targetService = IsNoPassThroughAllowed(target);
    bool const exitsService = isOutgoing ? sourceService && !targetService : targetService && !sourceService;
    auto const append = [&](PassThroughState state, int8_t charged)
    {
      auto edge = edges[i];
      auto parentWeight = parentWeights[i];
      edge.GetTarget().SetPassThroughState(state);
      auto & weight = isOutgoing ? edge.GetWeight() : parentWeight;
      weight += RouteWeight(0.0, charged - weight.GetNumPassThroughChanges(), 0, 0, 0.0);
      // Outgoing fake ending joints use the first-child weight instead of the full joint weight.
      if (isOutgoing)
        parentWeight += RouteWeight(0.0, charged - parentWeight.GetNumPassThroughChanges(), 0, 0, 0.0);
      result.push_back(edge);
      weights.push_back(parentWeight);
    };
    // Twin roads may have different country-specific pass-through rules.
    ForEachPassThroughState(source.GetPassThroughState(), isOutgoing, exitsService, targetService, append);
  }
  edges = std::move(result);
  parentWeights = std::move(weights);
}

void IndexGraphStarter::CreditEndingPassThrough(Segment const & segment, bool isOutgoing, EdgeListT & edges) const
{
  // Apply the whole credit on the forward expansion of the fake start, so a forward partial that
  // already left the start and re-entered the finish neighbourhood is not over-penalized while the
  // bidirectional waves haven't met yet (a split start/finish credit makes the wave settle on a
  // worse all-non-pass-through detour).
  if (!(isOutgoing && segment == GetStartSegment()))
    return;

  int8_t const credit = (HasNoPassThroughAllowed(m_start) ? 1 : 0) + (HasNoPassThroughAllowed(m_finish) ? 1 : 0);
  if (credit == 0)
    return;

  RouteWeight const delta(0.0 /* weight */, credit /* numPassThroughChanges */, 0 /* numAccessChanges */,
                          0 /* numAccessConditionalPenalties */, 0.0 /* transitTime */);
  for (auto & edge : edges)
    edge.GetWeight() = edge.GetWeight() - delta;
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
    if (m_fake.FindReal(segment.WithoutPassThroughState(), real))
    {
      real.SetPassThroughState(segment.GetPassThroughState());
      bool const haveSameFront = GetJunction(segment, true /* front */) == GetJunction(real, true);
      bool const haveSameBack = GetJunction(segment, false /* front */) == GetJunction(real, false);
      if ((isOutgoing && haveSameFront) || (!isOutgoing && haveSameBack))
      {
        if (IsGuidesSegment(real))
        {
          m_guides.GetEdgeList(real.WithoutPassThroughState(), isOutgoing, edges, ingoingSegmentWeight);
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

    for (auto const & s : m_fake.GetEdges(segment.WithoutPassThroughState(), isOutgoing))
      edges.emplace_back(s, isOutgoing ? CalcSegmentWeight(s, EdgeEstimator::Purpose::Weight) : ingoingSegmentWeight);
  }
  else if (IsGuidesSegment(segment))
  {
    m_guides.GetEdgeList(segment.WithoutPassThroughState(), isOutgoing, edges, ingoingSegmentWeight);
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
  if (m_passThroughStateRouting)
    ApplyPassThroughState(segment, isOutgoing, edges);
  else
    CreditEndingPassThrough(segment, isOutgoing, edges);
}

RouteWeight IndexGraphStarter::CalcGuidesSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) const
{
  if (purpose == EdgeEstimator::Purpose::Weight)
    return m_guides.CalcSegmentWeight(segment.WithoutPassThroughState());

  auto [from, to] = m_guides.GetFromTo(segment.WithoutPassThroughState());
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

  auto const & vertex = m_fake.GetVertex(segment.WithoutPassThroughState());
  Segment real;
  if (m_fake.FindReal(segment.WithoutPassThroughState(), real))
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

double IndexGraphStarter::CalculateETA(Segment const & from, Segment const & to, time_t arrivalTime) const
{
  // We don't distinguish fake segment weight and fake segment transit time.
  if (IsFakeSegment(to))
    return CalcSegmentWeight(to, EdgeEstimator::Purpose::ETA).GetWeight();

  if (IsFakeSegment(from))
    return FakeFeatureIds::IsTransitFeature(to.GetFeatureId()) ? m_graph.CalculateETA(from, to, arrivalTime)
                                                               : CalculateETAWithoutPenalty(to);

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

  return m_graph.CalculateETA(from, to, arrivalTime);
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

  std::map<Segment, std::vector<LatLonWithAltitude>> otherSegments;
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

void IndexGraphStarter::ConnectGateAccessesToTransit(GateAccessesT const & gates, bool isStart)
{
  if (gates.empty())
    return;

  Segment const originSegment = isStart ? GetStartSegment() : GetFinishSegment();
  LatLonWithAltitude const origin = GetJunction(originSegment, true /* front */);

  for (auto const & gate : gates)
  {
    auto const & projection = gate.m_projection;
    FakeVertex const projectionVertex(projection.m_segment.GetMwmId(), isStart ? origin : projection.m_junction,
                                      isStart ? projection.m_junction : origin, FakeVertex::Type::PureFake);

    Segment projectionSegment;
    CHECK(m_fake.FindSegment(projectionVertex, projectionSegment), (projection.m_segment, gate.m_entrySegment));

    // Hop from the checkpoint's gate snap candidate onto the gate's transit boarding entry (handled
    // by the world graph). |m_entrySegment| is a transit segment outside this fake graph, so use
    // AddOuterConnection.
    if (isStart)
      m_fake.AddOuterConnection(projectionSegment, gate.m_entrySegment);
    else
      m_fake.AddOuterConnection(gate.m_entrySegment, projectionSegment);
  }
}

void IndexGraphStarter::AddEnding(FakeEnding const & thisEnding, FakeEnding const & otherEnding, bool isStart,
                                  bool strictForward)
{
  Segment const dummy = Segment();

  std::map<Segment, LatLonWithAltitude> otherSegments;
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
    auto & ending = isStart ? m_start : m_finish;
    ending.m_real.insert(projection.m_segment);

    // Add projection edges
    auto const projectionSegment = GetFakeSegmentAndIncr();
    ending.m_projections.emplace(projectionSegment, projection.m_segment);
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
                                                   std::vector<std::pair<FakeVertex, Segment>> const & partsOfReal)
{
  m_fake.ConnectLoopToGuideSegments(loop, realSegment, realFrom, realTo, partsOfReal);
}

HighwayCategory IndexGraphStarter::GetHighwayCategory(Segment seg) const
{
  if (IsFakeSegment(seg))
  {
    Segment real;
    if (m_fake.FindReal(seg.WithoutPassThroughState(), real))
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
    for (auto const & s : m_fake.GetFake(edge.GetTarget().WithoutPassThroughState()))
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

bool IndexGraphStarter::IsNoPassThroughAllowed(Segment const & segment) const
{
  auto real = segment.WithoutPassThroughState();
  if (IsFakeSegment(real))
  {
    if (auto const it = m_start.m_projections.find(real); it != m_start.m_projections.end())
      real = it->second;
    else if (auto const it = m_finish.m_projections.find(real); it != m_finish.m_projections.end())
      real = it->second;
    else if (!ConvertToReal(real))
      return false;
  }
  return !IsRegionsGraphMode() && !IsGuidesSegment(real) &&
         !m_graph.IsPassThroughAllowed(real.GetMwmId(), real.GetFeatureId());
}

bool IndexGraphStarter::HasNoPassThroughAllowed(Ending const & ending) const
{
  return std::any_of(ending.m_real.cbegin(), ending.m_real.cend(),
                     [this](Segment const & segment) { return IsNoPassThroughAllowed(segment); });
}

Segment IndexGraphStarter::GetFakeSegmentAndIncr()
{
  CHECK_LESS(m_fakeNumerationStart, std::numeric_limits<uint32_t>::max(), ());
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
