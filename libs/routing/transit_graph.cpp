#include "routing/transit_graph.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/index_graph.hpp"

#include "geometry/mercator.hpp"

namespace routing
{
namespace
{
LatLonWithAltitude const & GetStopJunction(std::map<transit::StopId, LatLonWithAltitude> const & stopCoords,
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

TransitGraph::TransitGraph(NumMwmId numMwmId, std::shared_ptr<EdgeEstimator> estimator)
  : m_mwmId(numMwmId)
  , m_estimator(estimator)
{}

LatLonWithAltitude const & TransitGraph::GetJunction(Segment const & segment, bool front) const
{
  ASSERT(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));
  auto const & vertex = m_fake.GetVertex(segment);
  return front ? vertex.GetJunctionTo() : vertex.GetJunctionFrom();
}

RouteWeight TransitGraph::CalcSegmentWeight(Segment const & segment, EdgeEstimator::Purpose purpose) const
{
  ASSERT(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));

  double time;
  if (auto const * gate = FindGate(segment))
  {
    time = gate->m_gate.GetWeight();
  }
  else if (auto const * edge = FindEdge(segment))
  {
    time = edge->GetWeight();
    if (!edge->GetTransfer())
      return RouteWeight(time, 0 /* numPassThroughChanges */, 0 /* numAccessChanges */,
                         0 /* numAccessConditionalPenalties */, time /* transitTime */);
  }
  else
  {
    // Gate projections connect a platform or entrance to its road projection. Use realistic offroad
    // time so a few metres of road width do not dominate station choice; apply walking bias separately.
    // Parts of real roads are priced by TransitWorldGraph as a fraction of the real segment weight.
    ASSERT(m_fake.GetVertex(segment).GetType() == FakeVertex::Type::PureFake, (segment));
    time = m_estimator->CalcOffroad(GetJunction(segment, false /* front */).GetLatLon(),
                                    GetJunction(segment, true /* front */).GetLatLon(), EdgeEstimator::Purpose::ETA);
  }

  double const weight =
      purpose == EdgeEstimator::Purpose::Weight ? time * m_estimator->GetTransitWalkWeightFactor() : time;
  return RouteWeight(weight);
}

transit::LineId TransitGraph::GetRideLineId(Segment const & segment) const
{
  auto const * edge = FindEdge(segment);
  return edge && !edge->GetTransfer() ? edge->GetLineId() : transit::kInvalidLineId;
}

RouteWeight TransitGraph::GetTransferPenalty(transit::LineId fromLineId, Segment const & to) const
{
  auto const * edgeTo = FindEdge(to);
  if (!edgeTo || edgeTo->GetTransfer())
    return GetAStarWeightZero<RouteWeight>();

  auto const lineIdTo = edgeTo->GetLineId();
  if (fromLineId == lineIdTo)
    return GetAStarWeightZero<RouteWeight>();

  // Boarding after a walking connection or a line change incurs the line's mean wait.
  auto const it = m_transferPenaltiesSubway.find(lineIdTo);
  CHECK(it != m_transferPenaltiesSubway.cend(), ("Segment", to, "belongs to unknown line:", lineIdTo));
  // The alternative route adds a fixed penalty per boarding to the routing weight only (not to
  // transitTime); it is zero for the primary route.
  double const penalty = it->second + m_estimator->GetTransitAltBoardingPenaltyS();
  return RouteWeight(penalty /* weight */, 0 /* nonPassThrougCross */, 0 /* numAccessChanges */,
                     0 /* numAccessConditionalPenalties */, it->second /* transitTime */);
}

std::set<Segment> const & TransitGraph::GetFake(Segment const & real) const
{
  return m_fake.GetFake(real);
}

void TransitGraph::GetGatesNear(m2::PointD const & point, double radiusM, bool isEnter, GateAccessesT & out) const
{
  for (auto const & access : m_gateAccesses)
  {
    if (access.m_isEnter != isEnter)
      continue;
    if (mercator::DistanceOnEarth(point, mercator::FromLatLon(access.m_projection.m_junction.GetLatLon())) <= radiusM)
      out.push_back(access);
  }
}

bool TransitGraph::FindReal(Segment const & fake, Segment & real) const
{
  return m_fake.FindReal(fake, real);
}

void TransitGraph::Fill(transit::GraphData const & transitData, Endings const & gateEndings)
{
  // Line has information about transit interval.
  // We assume arrival time has uniform distribution with min value |0| and max value |line.GetInterval()|.
  // Expected value of time to wait transport for particular line is |line.GetInterval() / 2|.
  for (auto const & line : transitData.GetLines())
    m_transferPenaltiesSubway[line.GetId()] = line.GetInterval() / 2.0;

  std::map<transit::StopId, LatLonWithAltitude> stopCoords;
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

TransitGraph::GateInfo const * TransitGraph::FindGate(Segment const & segment) const
{
  auto const it = m_segmentToGateSubway.find(segment);
  return it != m_segmentToGateSubway.cend() ? &it->second : nullptr;
}

transit::Edge const * TransitGraph::FindEdge(Segment const & segment) const
{
  auto const it = m_segmentToEdgeSubway.find(segment);
  return it != m_segmentToEdgeSubway.cend() ? &it->second : nullptr;
}

Segment TransitGraph::GetTransitSegment(uint32_t featureId) const
{
  CHECK(IsTransitFeature(featureId), ("Feature id is out of transit id interval."));
  // All transit segments are oneway forward segments.
  // Edge segment has tail in stop1 and head in stop2.
  // Gate segments follow the traversal direction, from gate to stop or from stop to gate.
  // Pedestrian projection and parts of real have tail in |0| and head in |1|.
  // We rely on this rule in cross-mwm to have same behaviour of transit and
  // non-transit segments.
  return Segment(m_mwmId, featureId, 0 /* segmentIdx*/, true /* isForward */);
}

Segment TransitGraph::GetNewTransitSegment() const
{
  auto const featureId = m_fake.GetSize() + FakeFeatureIds::kTransitGraphFeaturesStart;
  CHECK_LESS_OR_EQUAL(featureId, std::numeric_limits<uint32_t>::max(), ());
  return GetTransitSegment(static_cast<uint32_t>(featureId));
}

void TransitGraph::AddGate(transit::Gate const & gate, FakeEnding const & ending,
                           std::map<transit::StopId, LatLonWithAltitude> const & stopCoords, bool isEnter,
                           StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront)
{
  Segment const dummy = Segment();
  for (auto const & projection : ending.m_projections)
  {
    // Add projection edges
    auto const projectionSegment = GetNewTransitSegment();
    FakeVertex projectionVertex(projection.m_segment.GetMwmId(),
                                isEnter ? projection.m_junction : ending.m_originJunction,
                                isEnter ? ending.m_originJunction : projection.m_junction, FakeVertex::Type::PureFake);
    m_fake.AddStandaloneVertex(projectionSegment, projectionVertex);

    // Record the gate's pedestrian access so nearby checkpoints can use it as a snap candidate.
    m_gateAccesses.emplace_back(projection, projectionSegment, isEnter);

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
      m_segmentToGateSubway.emplace(gateSegment, GateInfo{gate, isEnter});
      if (isEnter)
        stopToFront[stopId].insert(gateSegment);
      else
        stopToBack[stopId].insert(gateSegment);
    }
  }
}

Segment TransitGraph::AddEdge(transit::Edge const & edge,
                              std::map<transit::StopId, LatLonWithAltitude> const & stopCoords,
                              StopToSegmentsMap & stopToBack, StopToSegmentsMap & stopToFront)
{
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

void MakeGateEndings(std::vector<transit::Gate> const & gates, NumMwmId mwmId, IndexGraph & indexGraph,
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

}  // namespace routing
