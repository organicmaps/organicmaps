#include "routing/transit_graph.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/index_graph.hpp"

#include "geometry/mercator.hpp"

namespace routing
{
namespace
{
using namespace std;
// TODO(o.khlopkova) Replace this constant with implementation of intervals calculation on the
// gtfs converter step.
size_t constexpr kDefaultIntervalS = 60 * 60;  // 1 hour.

LatLonWithAltitude const & GetStopJunction(map<transit::StopId, LatLonWithAltitude> const & stopCoords,
                                           transit::StopId stopId)
{
  auto const it = stopCoords.find(stopId);
  CHECK(it != stopCoords.cend(), ("Stop", stopId, "does not exist."));
  return it->second;
}
}  // namespace

// static
bool TransitGraph::IsTransitFeature(uint32_t featureId)
{
  return FakeFeatureIds::IsTransitFeature(featureId);
}

// static
bool TransitGraph::IsTransitSegment(Segment const & segment)
{
  return IsTransitFeature(segment.GetFeatureId());
}

TransitGraph::TransitGraph(::transit::TransitVersion transitVersion, NumMwmId numMwmId,
                           shared_ptr<EdgeEstimator> estimator)
  : m_transitVersion(transitVersion)
  , m_mwmId(numMwmId)
  , m_estimator(estimator)
{}

::transit::TransitVersion TransitGraph::GetTransitVersion() const
{
  return m_transitVersion;
}

LatLonWithAltitude const & TransitGraph::GetJunction(Segment const & segment, bool front) const
{
  CHECK(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));
  auto const & vertex = m_fake.GetVertex(segment);
  return front ? vertex.GetJunctionTo() : vertex.GetJunctionFrom();
}

RouteWeight TransitGraph::CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) const
{
  CHECK(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));

  if (m_transitVersion == ::transit::TransitVersion::OnlySubway)
  {
    if (IsGate(segment))
    {
      auto const weight = GetGate(segment).GetWeight();
      return RouteWeight(weight, 0 /* numPassThroughChanges */, 0 /* numAccessChanges */,
                         0 /* numAccessConditionalPenalties */, weight /* transitTime */);
    }

    if (IsEdge(segment))
    {
      auto const weight = GetEdge(segment).GetWeight();
      return RouteWeight(weight, 0 /* numPassThroughChanges */, 0 /* numAccessChanges */,
                         0 /* numAccessConditionalPenalties */, weight /* transitTime */);
    }
  }
  else if (m_transitVersion == ::transit::TransitVersion::AllPublicTransport)
  {
    if (IsGate(segment))
    {
      // TODO (o.khlopkova) Manage different weights for different stops linked to the gate.
      auto const weight = kDefaultIntervalS;
      return RouteWeight(weight /* weight */, 0 /* numPassThroughChanges */, 0 /* numAccessChanges */,
                         0 /* numAccessConditionalPenalties */, weight /* transitTime */);
    }

    if (IsEdge(segment))
    {
      auto const weight = GetEdgePT(segment).GetWeight();
      CHECK_NOT_EQUAL(weight, 0, (segment));
      return RouteWeight(weight /* weight */, 0 /* numPassThroughChanges */, 0 /* numAccessChanges */,
                         0 /* numAccessConditionalPenalties */, weight /* transitTime */);
    }
  }
  else
  {
    CHECK(false, (m_transitVersion));
    UNREACHABLE();
  }

  return RouteWeight(m_estimator->CalcOffroad(GetJunction(segment, false /* front */).GetLatLon(),
                                              GetJunction(segment, true /* front */).GetLatLon(), purpose));
}

RouteWeight TransitGraph::GetTransferPenalty(Segment const & from, Segment const & to) const
{
  if (m_transitVersion == ::transit::TransitVersion::OnlySubway)
  {
    // We need to wait transport and apply additional penalty only if we change to transit::Edge.
    if (!IsEdge(to))
      return GetAStarWeightZero<RouteWeight>();

    auto const & edgeTo = GetEdge(to);

    // We are changing to transfer and do not need to apply extra penalty here. We'll do it while
    // changing from transfer.
    if (edgeTo.GetTransfer())
      return GetAStarWeightZero<RouteWeight>();

    auto const lineIdTo = edgeTo.GetLineId();

    if (IsEdge(from) && GetEdge(from).GetLineId() == lineIdTo)
      return GetAStarWeightZero<RouteWeight>();

    // We need to apply extra penalty when:
    // 1. |from| is gate, |to| is edge
    // 2. |from| is transfer, |to| is edge
    // 3. |from| is edge, |to| is edge from another line directly connected to |from|.
    auto const it = m_transferPenaltiesSubway.find(lineIdTo);
    CHECK(it != m_transferPenaltiesSubway.cend(), ("Segment", to, "belongs to unknown line:", lineIdTo));
    return RouteWeight(it->second /* weight */, 0 /* nonPassThrougCross */, 0 /* numAccessChanges */,
                       0 /* numAccessConditionalPenalties */, it->second /* transitTime */);
  }
  else if (m_transitVersion == ::transit::TransitVersion::AllPublicTransport)
  {
    // We need to wait transport and apply additional penalty only if we change to transit::Edge.
    if (!IsEdge(to))
      return GetAStarWeightZero<RouteWeight>();

    auto const & edgeTo = GetEdgePT(to);

    // We are changing to transfer and do not need to apply extra penalty here. We'll do it while
    // changing from transfer.
    if (edgeTo.IsTransfer())
      return GetAStarWeightZero<RouteWeight>();

    auto const lineIdTo = edgeTo.GetLineId();

    if (IsEdge(from) && GetEdgePT(from).GetLineId() == lineIdTo)
      return GetAStarWeightZero<RouteWeight>();

    // We need to apply extra penalty when:
    // 1. |from| is gate, |to| is edge
    // 2. |from| is transfer, |to| is edge
    // 3. |from| is edge, |to| is edge from another line directly connected to |from|.
    auto const it = m_transferPenaltiesPT.find(lineIdTo);
    CHECK(it != m_transferPenaltiesPT.end(), ("Segment", to, "belongs to unknown line:", lineIdTo));
    // TODO(o.khlopkova) If we know exact start time for trip we can extract more precise headway.
    // We need to call GetFrequency(time).
    size_t const headwayS = it->second.GetFrequency() / 2;

    return RouteWeight(static_cast<double>(headwayS) /* weight */, 0 /* nonPassThroughCross */,
                       0 /* numAccessChanges */, 0 /* numAccessConditionalPenalties */, headwayS /* transitTime */);
  }

  CHECK(false, (m_transitVersion));
  UNREACHABLE();
}

void TransitGraph::GetTransitEdges(Segment const & segment, bool isOutgoing, EdgeListT & edges) const
{
  CHECK(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));
  for (auto const & s : m_fake.GetEdges(segment, isOutgoing))
  {
    auto const & from = isOutgoing ? segment : s;
    auto const & to = isOutgoing ? s : segment;
    edges.emplace_back(s, CalcSegmentWeight(to, EdgeEstimator::Purpose::Weight) + GetTransferPenalty(from, to));
  }
}

set<Segment> const & TransitGraph::GetFake(Segment const & real) const
{
  return m_fake.GetFake(real);
}

bool TransitGraph::FindReal(Segment const & fake, Segment & real) const
{
  return m_fake.FindReal(fake, real);
}

void TransitGraph::Fill(::transit::experimental::TransitData const & transitData, Endings const & stopEndings,
                        Endings const & gateEndings)
{
  CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::AllPublicTransport, ());

  for (auto const & line : transitData.GetLines())
    m_transferPenaltiesPT[line.GetId()] = line.GetSchedule();

  map<transit::StopId, LatLonWithAltitude> stopCoords;

  for (auto const & stop : transitData.GetStops())
  {
    stopCoords[stop.GetId()] =
        LatLonWithAltitude(mercator::ToLatLon(stop.GetPoint()), geometry::kDefaultAltitudeMeters);
  }

  StopToSegmentsMap stopToBack;
  StopToSegmentsMap stopToFront;
  StopToSegmentsMap outgoing;
  StopToSegmentsMap ingoing;

  // It's important to add transit edges first to ensure fake segment id for particular edge is edge
  // order in mwm. We use edge fake segments in cross-mwm section and they should be stable.
  auto const & edges = transitData.GetEdges();
  CHECK_EQUAL(m_fake.GetSize(), 0, ());
  for (size_t i = 0; i < edges.size(); ++i)
  {
    auto const & edge = edges[i];
    CHECK_NOT_EQUAL(edge.GetWeight(), transit::kInvalidWeight, ("Edge should have valid weight."));
    auto const edgeSegment = AddEdge(edge, stopCoords, stopToBack, stopToFront);
    // Checks fake feature ids have consecutive numeration starting from
    // FakeFeatureIds::kTransitGraphFeaturesStart.
    CHECK_EQUAL(edgeSegment.GetFeatureId(), i + FakeFeatureIds::kTransitGraphFeaturesStart, ());
    outgoing[edge.GetStop1Id()].insert(edgeSegment);
    ingoing[edge.GetStop2Id()].insert(edgeSegment);
  }
  CHECK_EQUAL(m_fake.GetSize(), edges.size(), ());

  for (auto const & gate : transitData.GetGates())
  {
    CHECK(!gate.GetStopsWithWeight().empty(), ("Gate should have valid weight.", gate));

    // Gate ending may have empty projections vector. It means gate is not connected to roads.
    auto const it = gateEndings.find(gate.GetOsmId());
    if (it != gateEndings.end())
    {
      if (gate.IsEntrance())
        AddGate(gate, it->second, stopCoords, true /* isEnter */, stopToBack, stopToFront);
      if (gate.IsExit())
        AddGate(gate, it->second, stopCoords, false /* isEnter */, stopToBack, stopToFront);
    }
  }

  for (auto const & stop : transitData.GetStops())
  {
    // Stop ending may have empty projections vector. It means stop is not connected to roads.
    auto const it = stopEndings.find(stop.GetId());
    if (it != stopEndings.end())
      AddStop(stop, it->second, stopCoords, stopToBack, stopToFront);
  }

  AddConnections(outgoing, stopToBack, stopToFront, true /* isOutgoing */);
  AddConnections(ingoing, stopToBack, stopToFront, false /* isOutgoing */);
}

void TransitGraph::Fill(transit::GraphData const & transitData, Endings const & gateEndings)
{
  CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::OnlySubway, (m_transitVersion));

  // Line has information about transit interval.
  // We assume arrival time has uniform distribution with min value |0| and max value |line.GetInterval()|.
  // Expected value of time to wait transport for particular line is |line.GetInterval() / 2|.
  for (auto const & line : transitData.GetLines())
    m_transferPenaltiesSubway[line.GetId()] = line.GetInterval() / 2;

  map<transit::StopId, LatLonWithAltitude> stopCoords;
  for (auto const & stop : transitData.GetStops())
    stopCoords[stop.GetId()] =
        LatLonWithAltitude(mercator::ToLatLon(stop.GetPoint()), geometry::kDefaultAltitudeMeters);

  StopToSegmentsMap stopToBack;
  StopToSegmentsMap stopToFront;
  StopToSegmentsMap outgoing;
  StopToSegmentsMap ingoing;

  // It's important to add transit edges first to ensure fake segment id for particular edge is edge order
  // in mwm. We use edge fake segments in cross-mwm section and they should be stable.
  auto const & edges = transitData.GetEdges();
  CHECK_EQUAL(m_fake.GetSize(), 0, ());
  for (size_t i = 0; i < edges.size(); ++i)
  {
    auto const & edge = edges[i];
    CHECK_NOT_EQUAL(edge.GetWeight(), transit::kInvalidWeight, ("Edge should have valid weight."));
    auto const edgeSegment = AddEdge(edge, stopCoords, stopToBack, stopToFront);
    // Checks fake feature ids have consecutive numeration starting from
    // FakeFeatureIds::kTransitGraphFeaturesStart.
    CHECK_EQUAL(edgeSegment.GetFeatureId(), i + FakeFeatureIds::kTransitGraphFeaturesStart, (i, edge));
    outgoing[edge.GetStop1Id()].insert(edgeSegment);
    ingoing[edge.GetStop2Id()].insert(edgeSegment);
  }
  CHECK_EQUAL(m_fake.GetSize(), edges.size(), ());

  for (auto const & gate : transitData.GetGates())
  {
    CHECK_NOT_EQUAL(gate.GetWeight(), transit::kInvalidWeight, ("Gate should have valid weight."));

    // Gate ending may have empty projections vector. It means gate is not connected to roads
    auto const it = gateEndings.find(gate.GetOsmId());
    if (it != gateEndings.cend())
    {
      if (gate.GetEntrance())
        AddGate(gate, it->second, stopCoords, true /* isEnter */, stopToBack, stopToFront);
      if (gate.GetExit())
        AddGate(gate, it->second, stopCoords, false /* isEnter */, stopToBack, stopToFront);
    }
  }

  AddConnections(outgoing, stopToBack, stopToFront, true /* isOutgoing */);
  AddConnections(ingoing, stopToBack, stopToFront, false /* isOutgoing */);
}

bool TransitGraph::IsGate(Segment const & segment) const
{
  if (m_transitVersion == ::transit::TransitVersion::OnlySubway)
    return m_segmentToGateSubway.count(segment) > 0;
  else if (m_transitVersion == ::transit::TransitVersion::AllPublicTransport)
    return m_segmentToGatePT.count(segment) > 0;
  CHECK(false, (m_transitVersion));
  UNREACHABLE();
}

bool TransitGraph::IsEdge(Segment const & segment) const
{
  if (m_transitVersion == ::transit::TransitVersion::OnlySubway)
    return m_segmentToEdgeSubway.count(segment) > 0;
  else if (m_transitVersion == ::transit::TransitVersion::AllPublicTransport)
    return m_segmentToEdgePT.count(segment) > 0;
  CHECK(false, (m_transitVersion));
  UNREACHABLE();
}

transit::Gate const & TransitGraph::GetGate(Segment const & segment) const
{
  auto const it = m_segmentToGateSubway.find(segment);
  CHECK(it != m_segmentToGateSubway.cend(), ("Unknown transit segment", segment));
  return it->second;
}

::transit::experimental::Gate const & TransitGraph::GetGatePT(Segment const & segment) const
{
  CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::AllPublicTransport, (segment));
  auto const it = m_segmentToGatePT.find(segment);
  CHECK(it != m_segmentToGatePT.cend(), ("Unknown transit segment", segment));
  return it->second;
}

transit::Edge const & TransitGraph::GetEdge(Segment const & segment) const
{
  auto const it = m_segmentToEdgeSubway.find(segment);
  CHECK(it != m_segmentToEdgeSubway.cend(), ("Unknown transit segment."));
  return it->second;
}

::transit::experimental::Edge const & TransitGraph::GetEdgePT(Segment const & segment) const
{
  CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::AllPublicTransport, (segment));
  auto const it = m_segmentToEdgePT.find(segment);
  CHECK(it != m_segmentToEdgePT.cend(), ("Unknown transit segment", segment));
  return it->second;
}

Segment TransitGraph::GetTransitSegment(uint32_t featureId) const
{
  CHECK(IsTransitFeature(featureId), ("Feature id is out of transit id interval."));
  // All transit segments are oneway forward segments.
  // Edge segment has tail in stop1 and head in stop2.
  // Gate segment has tail in gate and head in stop.
  // Pedestrian projection and parts of real have tail in |0| and head in |1|.
  // We rely on this rule in cross-mwm to have same behaviour of transit and
  // non-transit segments.
  return Segment(m_mwmId, featureId, 0 /* segmentIdx*/, true /* isForward */);
}

Segment TransitGraph::GetNewTransitSegment() const
{
  auto const featureId = m_fake.GetSize() + FakeFeatureIds::kTransitGraphFeaturesStart;
  CHECK_LESS_OR_EQUAL(featureId, numeric_limits<uint32_t>::max(), ());
  return GetTransitSegment(static_cast<uint32_t>(featureId));
}

void TransitGraph::AddGate(transit::Gate const & gate, FakeEnding const & ending,
                           map<transit::StopId, LatLonWithAltitude> const & stopCoords, bool isEnter,
                           StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront)
{
  CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::OnlySubway, (gate));

  Segment const dummy = Segment();
  for (auto const & projection : ending.m_projections)
  {
    // Add projection edges
    auto const projectionSegment = GetNewTransitSegment();
    FakeVertex projectionVertex(projection.m_segment.GetMwmId(),
                                isEnter ? projection.m_junction : ending.m_originJunction,
                                isEnter ? ending.m_originJunction : projection.m_junction, FakeVertex::Type::PureFake);
    m_fake.AddStandaloneVertex(projectionSegment, projectionVertex);

    // Add fake parts of real
    FakeVertex forwardPartOfReal(
        projection.m_segment.GetMwmId(), isEnter ? projection.m_segmentBack : projection.m_junction,
        isEnter ? projection.m_junction : projection.m_segmentFront, FakeVertex::Type::PartOfReal);
    auto const fakeForwardSegment = GetNewTransitSegment();
    m_fake.AddVertex(projectionSegment, fakeForwardSegment, forwardPartOfReal, !isEnter /* isOutgoing */,
                     true /* isPartOfReal */, projection.m_segment);

    if (!projection.m_isOneWay)
    {
      FakeVertex backwardPartOfReal(
          projection.m_segment.GetMwmId(), isEnter ? projection.m_segmentFront : projection.m_junction,
          isEnter ? projection.m_junction : projection.m_segmentBack, FakeVertex::Type::PartOfReal);
      auto const fakeBackwardSegment = GetNewTransitSegment();
      m_fake.AddVertex(projectionSegment, fakeBackwardSegment, backwardPartOfReal, !isEnter /* isOutgoing */,
                       true /* isPartOfReal */, projection.m_segment.GetReversed());
    }

    // Connect gate to stops
    for (auto const stopId : gate.GetStopIds())
    {
      auto const gateSegment = GetNewTransitSegment();
      auto const stopIt = stopCoords.find(stopId);
      CHECK(stopIt != stopCoords.end(), ("Stop", stopId, "does not exist."));
      FakeVertex gateVertex(projection.m_segment.GetMwmId(), isEnter ? ending.m_originJunction : stopIt->second,
                            isEnter ? stopIt->second : ending.m_originJunction, FakeVertex::Type::PureFake);
      m_fake.AddVertex(projectionSegment, gateSegment, gateVertex, isEnter /* isOutgoing */, false /* isPartOfReal */,
                       dummy /* realSegment */);
      m_segmentToGateSubway[gateSegment] = gate;
      if (isEnter)
        stopToFront[stopId].insert(gateSegment);
      else
        stopToBack[stopId].insert(gateSegment);
    }
  }
}

void TransitGraph::AddGate(::transit::experimental::Gate const & gate, FakeEnding const & ending,
                           std::map<transit::StopId, LatLonWithAltitude> const & stopCoords, bool isEnter,
                           StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront)
{
  CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::AllPublicTransport, (gate));

  Segment const dummy = Segment();
  for (auto const & projection : ending.m_projections)
  {
    // Add projection edges
    auto const projectionSegment = GetNewTransitSegment();
    FakeVertex projectionVertex(projection.m_segment.GetMwmId(),
                                isEnter ? projection.m_junction : ending.m_originJunction,
                                isEnter ? ending.m_originJunction : projection.m_junction, FakeVertex::Type::PureFake);
    m_fake.AddStandaloneVertex(projectionSegment, projectionVertex);

    // Add fake parts of real
    FakeVertex forwardPartOfReal(
        projection.m_segment.GetMwmId(), isEnter ? projection.m_segmentBack : projection.m_junction,
        isEnter ? projection.m_junction : projection.m_segmentFront, FakeVertex::Type::PartOfReal);
    auto const fakeForwardSegment = GetNewTransitSegment();
    m_fake.AddVertex(projectionSegment, fakeForwardSegment, forwardPartOfReal, !isEnter /* isOutgoing */,
                     true /* isPartOfReal */, projection.m_segment);

    if (!projection.m_isOneWay)
    {
      FakeVertex backwardPartOfReal(
          projection.m_segment.GetMwmId(), isEnter ? projection.m_segmentFront : projection.m_junction,
          isEnter ? projection.m_junction : projection.m_segmentBack, FakeVertex::Type::PartOfReal);
      auto const fakeBackwardSegment = GetNewTransitSegment();
      m_fake.AddVertex(projectionSegment, fakeBackwardSegment, backwardPartOfReal, !isEnter /* isOutgoing */,
                       true /* isPartOfReal */, projection.m_segment.GetReversed());
    }

    // Connect gate to stops
    for (auto const & timeFromGateToStop : gate.GetStopsWithWeight())
    {
      auto const stopId = timeFromGateToStop.m_stopId;
      auto const gateSegment = GetNewTransitSegment();
      auto const stopIt = stopCoords.find(stopId);
      CHECK(stopIt != stopCoords.end(), ("Stop", stopId, "does not exist."));
      FakeVertex gateVertex(projection.m_segment.GetMwmId(), isEnter ? ending.m_originJunction : stopIt->second,
                            isEnter ? stopIt->second : ending.m_originJunction, FakeVertex::Type::PureFake);
      m_fake.AddVertex(projectionSegment, gateSegment, gateVertex, isEnter /* isOutgoing */, false /* isPartOfReal */,
                       dummy /* realSegment */);
      m_segmentToGatePT[gateSegment] = gate;
      if (isEnter)
        stopToFront[stopId].insert(gateSegment);
      else
        stopToBack[stopId].insert(gateSegment);
    }
  }
}

void TransitGraph::AddStop(::transit::experimental::Stop const & stop, FakeEnding const & ending,
                           std::map<transit::StopId, LatLonWithAltitude> const & stopCoords,
                           StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront)
{
  CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::AllPublicTransport, (stop));

  Segment const dummy = Segment();
  for (bool isEnter : {true, false})
  {
    for (auto const & projection : ending.m_projections)
    {
      // Add projection edges
      auto const projectionSegment = GetNewTransitSegment();
      FakeVertex projectionVertex(
          projection.m_segment.GetMwmId(), isEnter ? projection.m_junction : ending.m_originJunction,
          isEnter ? ending.m_originJunction : projection.m_junction, FakeVertex::Type::PureFake);
      m_fake.AddStandaloneVertex(projectionSegment, projectionVertex);

      // Add fake parts of real
      FakeVertex forwardPartOfReal(
          projection.m_segment.GetMwmId(), isEnter ? projection.m_segmentBack : projection.m_junction,
          isEnter ? projection.m_junction : projection.m_segmentFront, FakeVertex::Type::PartOfReal);
      auto const fakeForwardSegment = GetNewTransitSegment();
      m_fake.AddVertex(projectionSegment, fakeForwardSegment, forwardPartOfReal, !isEnter /* isOutgoing */,
                       true /* isPartOfReal */, projection.m_segment);

      if (!projection.m_isOneWay)
      {
        FakeVertex backwardPartOfReal(
            projection.m_segment.GetMwmId(), isEnter ? projection.m_segmentFront : projection.m_junction,
            isEnter ? projection.m_junction : projection.m_segmentBack, FakeVertex::Type::PartOfReal);
        auto const fakeBackwardSegment = GetNewTransitSegment();
        m_fake.AddVertex(projectionSegment, fakeBackwardSegment, backwardPartOfReal, !isEnter /* isOutgoing */,
                         true /* isPartOfReal */, projection.m_segment.GetReversed());
      }

      // Connect stop to graph.
      auto const stopId = stop.GetId();
      auto const stopSegment = GetNewTransitSegment();
      auto const stopIt = stopCoords.find(stopId);
      CHECK(stopIt != stopCoords.end(), ("Stop", stopId, "does not exist."));
      FakeVertex stopVertex(projection.m_segment.GetMwmId(), isEnter ? ending.m_originJunction : stopIt->second,
                            isEnter ? stopIt->second : ending.m_originJunction, FakeVertex::Type::PureFake);
      m_fake.AddVertex(projectionSegment, stopSegment, stopVertex, isEnter /* isOutgoing */, false /* isPartOfReal */,
                       dummy /* realSegment */);
      m_segmentToStopPT[stopSegment] = stop;
      if (isEnter)
        stopToFront[stopId].insert(stopSegment);
      else
        stopToBack[stopId].insert(stopSegment);
    }
  }
}

Segment TransitGraph::AddEdge(transit::Edge const & edge, map<transit::StopId, LatLonWithAltitude> const & stopCoords,
                              StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront)
{
  CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::OnlySubway, (edge));

  auto const edgeSegment = GetNewTransitSegment();
  auto const stopFromId = edge.GetStop1Id();
  auto const stopToId = edge.GetStop2Id();
  FakeVertex edgeVertex(m_mwmId, GetStopJunction(stopCoords, stopFromId), GetStopJunction(stopCoords, stopToId),
                        FakeVertex::Type::PureFake);
  m_fake.AddStandaloneVertex(edgeSegment, edgeVertex);
  m_segmentToEdgeSubway[edgeSegment] = edge;
  stopToBack[stopFromId].insert(edgeSegment);
  stopToFront[stopToId].insert(edgeSegment);
  return edgeSegment;
}

Segment TransitGraph::AddEdge(::transit::experimental::Edge const & edge,
                              std::map<transit::StopId, LatLonWithAltitude> const & stopCoords,
                              StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront)
{
  CHECK_EQUAL(m_transitVersion, ::transit::TransitVersion::AllPublicTransport, (edge));

  auto const edgeSegment = GetNewTransitSegment();
  auto const stopFromId = edge.GetStop1Id();
  auto const stopToId = edge.GetStop2Id();
  FakeVertex edgeVertex(m_mwmId, GetStopJunction(stopCoords, stopFromId), GetStopJunction(stopCoords, stopToId),
                        FakeVertex::Type::PureFake);
  m_fake.AddStandaloneVertex(edgeSegment, edgeVertex);
  m_segmentToEdgePT[edgeSegment] = edge;
  stopToBack[stopFromId].insert(edgeSegment);
  stopToFront[stopToId].insert(edgeSegment);
  return edgeSegment;
}

void TransitGraph::AddConnections(StopToSegmentsMap const & connections, StopToSegmentsMap const & stopToBack,
                                  StopToSegmentsMap const & stopToFront, bool isOutgoing)
{
  for (auto const & connection : connections)
  {
    for (auto const & connectedSegment : connection.second)
    {
      auto const & adjacentSegments = isOutgoing ? stopToFront : stopToBack;
      auto const segmentsIt = adjacentSegments.find(connection.first);
      if (segmentsIt == adjacentSegments.cend())
        continue;
      for (auto const & segment : segmentsIt->second)
        m_fake.AddConnection(isOutgoing ? segment : connectedSegment, isOutgoing ? connectedSegment : segment);
    }
  }
}

void MakeGateEndings(vector<transit::Gate> const & gates, NumMwmId mwmId, IndexGraph & indexGraph,
                     TransitGraph::Endings & gateEndings)
{
  for (auto const & gate : gates)
  {
    auto const & gateSegment = gate.GetBestPedestrianSegment();
    if (!gateSegment.IsValid())
      continue;

    Segment const real(mwmId, gateSegment.GetFeatureId(), gateSegment.GetSegmentIdx(), gateSegment.GetForward());
    gateEndings.emplace(gate.GetOsmId(), MakeFakeEnding({real}, gate.GetPoint(), indexGraph));
  }
}

void MakeGateEndings(std::vector<::transit::experimental::Gate> const & gates, NumMwmId mwmId, IndexGraph & indexGraph,
                     TransitGraph::Endings & gateEndings)
{
  for (auto const & gate : gates)
  {
    for (auto const & gateSegment : gate.GetBestPedestrianSegments())
    {
      Segment const real(mwmId, gateSegment.GetFeatureId(), gateSegment.GetSegmentIdx(), gateSegment.IsForward());
      gateEndings.emplace(gate.GetOsmId(), MakeFakeEnding(real, gate.GetPoint(), indexGraph));
    }
  }
}

void MakeStopEndings(std::vector<::transit::experimental::Stop> const & stops, NumMwmId mwmId, IndexGraph & indexGraph,
                     TransitGraph::Endings & stopEndings)
{
  for (auto const & stop : stops)
  {
    for (auto const & stopSegment : stop.GetBestPedestrianSegments())
    {
      Segment const real(mwmId, stopSegment.GetFeatureId(), stopSegment.GetSegmentIdx(), stopSegment.IsForward());
      stopEndings.emplace(stop.GetId(), MakeFakeEnding(real, stop.GetPoint(), indexGraph));
    }
  }
}
}  // namespace routing
