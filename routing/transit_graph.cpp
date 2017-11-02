#include "routing/transit_graph.hpp"

#include "indexer/feature_altitude.hpp"

namespace routing
{
namespace
{
using namespace std;

Segment GetReverseSegment(Segment const & segment)
{
  return Segment(segment.GetMwmId(), segment.GetFeatureId(), segment.GetSegmentIdx(),
                 !segment.IsForward());
}

Junction const & GetStopJunction(map<transit::StopId, Junction> const & stopCoords,
                                 transit::StopId stopId)
{
  auto const it = stopCoords.find(stopId);
  CHECK(it != stopCoords.cend(), ("Stop", stopId, "does not exist."));
  return it->second;
}
}  // namespace

// static
uint32_t constexpr TransitGraph::kTransitFeatureId;

// static
bool TransitGraph::IsTransitFeature(uint32_t featureId) { return featureId == kTransitFeatureId; }

// static
bool TransitGraph::IsTransitSegment(Segment const & segment)
{
  return IsTransitFeature(segment.GetFeatureId());
}

Junction const & TransitGraph::GetJunction(Segment const & segment, bool front) const
{
  CHECK(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));
  auto const & vertex = m_fake.GetVertex(segment);
  return front ? vertex.GetJunctionTo() : vertex.GetJunctionFrom();
}

RouteWeight TransitGraph::CalcSegmentWeight(Segment const & segment,
                                            EdgeEstimator const & estimator) const
{
  CHECK(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));
  if (IsGate(segment))
    return RouteWeight(GetGate(segment).GetWeight(), 0 /* nontransitCross */);

  if (IsEdge(segment))
    return RouteWeight(GetEdge(segment).GetWeight(), 0 /* nontransitCross */);

  // Estimate weight for part of real and projections.
  return RouteWeight(estimator.CalcLeapWeight(GetJunction(segment, false /* front */).GetPoint(),
                                              GetJunction(segment, true /* front */).GetPoint()),
                     0 /* nontransitCross */);
}

RouteWeight TransitGraph::GetTransferPenalty(Segment const & from, Segment const & to) const
{
  if (!IsEdge(from) || !IsEdge(to))
    return RouteWeight(0 /* weight */, 0 /* nontransitCross */);

  auto lineIdFrom = GetEdge(from).GetLineId();
  auto lineIdTo = GetEdge(to).GetLineId();

  if (lineIdFrom == lineIdTo)
    return RouteWeight(0 /* weight */, 0 /* nontransitCross */);

  auto const it = m_transferPenalties.find(lineIdTo);
  CHECK(it != m_transferPenalties.cend(), ("Segment", to, "belongs to unknown line:", lineIdTo));
  return RouteWeight(it->second, 0 /* nontransitCross */);
}

void TransitGraph::GetTransitEdges(Segment const & segment, bool isOutgoing,
                                   vector<SegmentEdge> & edges,
                                   EdgeEstimator const & estimator) const
{
  CHECK(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));
  for (auto const & s : m_fake.GetEdges(segment, isOutgoing))
  {
    auto const & from = isOutgoing ? segment : s;
    auto const & to = isOutgoing ? s : segment;
    edges.emplace_back(s, CalcSegmentWeight(to, estimator) + GetTransferPenalty(from, to));
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

void TransitGraph::Fill(vector<transit::Stop> const & stops, vector<transit::Gate> const & gates,
                        vector<transit::Edge> const & edges, vector<transit::Line> const & lines,
                        EdgeEstimator const & estimator, NumMwmId numMwmId, IndexGraph & indexGraph)
{
  m_mwmId = numMwmId;

  // Line has information about transit interval. Average time to wait transport for particular line
  // is half of this line interval.
  for (auto const & line : lines)
    m_transferPenalties[line.GetId()] = line.GetInterval() / 2;

  map<transit::StopId, Junction> stopCoords;
  for (auto const & stop : stops)
    stopCoords[stop.GetId()] = Junction(stop.GetPoint(), feature::kDefaultAltitudeMeters);

  for (auto const & gate : gates)
  {
    CHECK_NOT_EQUAL(gate.GetWeight(), transit::kInvalidWeight, ("Gate should have valid weight."));
    auto const gateSegment = gate.GetBestPedestrianSegment();
    Segment real(numMwmId, gateSegment.GetFeatureId(), gateSegment.GetSegmentIdx(), gateSegment.GetForward());
    auto const ending =
        MakeFakeEnding(real, gate.GetPoint(), estimator, indexGraph);
    if (gate.GetEntrance())
      AddGate(gate, ending, stopCoords, true /* isEnter */);
    if (gate.GetExit())
      AddGate(gate, ending, stopCoords, false /* isEnter */);
  }

  map<transit::StopId, set<Segment>> outgoing;
  map<transit::StopId, set<Segment>> ingoing;
  for (auto const & edge : edges)
  {
    CHECK_NOT_EQUAL(edge.GetWeight(), transit::kInvalidWeight, ("Edge should have valid weight."));
    auto const edgeSegment = AddEdge(edge, stopCoords);
    outgoing[edge.GetStop1Id()].insert(edgeSegment);
    ingoing[edge.GetStop2Id()].insert(edgeSegment);
  }

  AddConnections(outgoing, true /* isOutgoing */);
  AddConnections(ingoing, false /* isOutgoing */);
}

bool TransitGraph::IsGate(Segment const & segment) const
{
  return m_segmentToGate.count(segment) > 0;
}

bool TransitGraph::IsEdge(Segment const & segment) const
{
  return m_segmentToEdge.count(segment) > 0;
}

transit::Gate const & TransitGraph::GetGate(Segment const & segment) const
{
  auto const it = m_segmentToGate.find(segment);
  CHECK(it != m_segmentToGate.cend(), ("Unknown transit segment."));
  return it->second;
}

transit::Edge const & TransitGraph::GetEdge(Segment const & segment) const
{
  auto const it = m_segmentToEdge.find(segment);
  CHECK(it != m_segmentToEdge.cend(), ("Unknown transit segment."));
  return it->second;
}

Segment TransitGraph::GetTransitSegment(uint32_t segmentIdx) const
{
  return Segment(m_mwmId, kTransitFeatureId, segmentIdx, false);
}

Segment TransitGraph::GetNewTransitSegment() const
{
  CHECK_LESS_OR_EQUAL(m_fake.GetSize(), std::numeric_limits<uint32_t>::max(), ());
  return GetTransitSegment(static_cast<uint32_t>(m_fake.GetSize()) /* segmentIdx */);
}

void TransitGraph::AddGate(transit::Gate const & gate, FakeEnding const & ending,
                           map<transit::StopId, Junction> const & stopCoords, bool isEnter)
{
  Segment const dummy = Segment();
  for (auto const & projection : ending.m_projections)
  {
    // Add projection edges
    auto const projectionSegment = GetNewTransitSegment();
    FakeVertex projectionVertex(isEnter ? projection.m_junction : ending.m_originJunction,
                                isEnter ? ending.m_originJunction : projection.m_junction,
                                FakeVertex::Type::PureFake);
    m_fake.AddStandaloneVertex(projectionSegment, projectionVertex);

    // Add fake parts of real
    FakeVertex forwardPartOfReal(isEnter ? projection.m_segmentBack : projection.m_junction,
                                 isEnter ? projection.m_junction : projection.m_segmentFront,
                                 FakeVertex::Type::PartOfReal);
    auto const fakeForwardSegment = GetNewTransitSegment();
    m_fake.AddVertex(projectionSegment, fakeForwardSegment, forwardPartOfReal,
                     !isEnter /* isOutgoing */, true /* isPartOfReal */, projection.m_segment);

    if (!projection.m_isOneWay)
    {
      FakeVertex backwardPartOfReal(isEnter ? projection.m_segmentFront : projection.m_junction,
                                    isEnter ? projection.m_junction : projection.m_segmentBack,
                                    FakeVertex::Type::PartOfReal);
      auto const fakeBackwardSegment = GetNewTransitSegment();
      m_fake.AddVertex(projectionSegment, fakeBackwardSegment, backwardPartOfReal,
                       !isEnter /* isOutgoing */, true /* isPartOfReal */,
                       GetReverseSegment(projection.m_segment));
    }

    // Connect gate to stops
    for (auto const stopId : gate.GetStopIds())
    {
      auto const gateSegment = GetNewTransitSegment();
      auto const stopIt = stopCoords.find(stopId);
      CHECK(stopIt != stopCoords.end(), ("Stop", stopId, "does not exist."));
      FakeVertex gateVertex(isEnter ? ending.m_originJunction : stopIt->second,
                            isEnter ? stopIt->second : ending.m_originJunction,
                            FakeVertex::Type::PureFake);
      m_fake.AddVertex(projectionSegment, gateSegment, gateVertex, isEnter /* isOutgoing */,
                       false /* isPartOfReal */, dummy /* realSegment */);
      m_segmentToGate[gateSegment] = gate;
      if (isEnter)
        m_stopToFront[stopId].insert(gateSegment);
      else
        m_stopToBack[stopId].insert(gateSegment);
    }
  }
}

Segment TransitGraph::AddEdge(transit::Edge const & edge,
                              map<transit::StopId, Junction> const & stopCoords)
{
  auto const edgeSegment = GetNewTransitSegment();
  auto const stopFromId = edge.GetStop1Id();
  auto const stopToId = edge.GetStop2Id();
  FakeVertex edgeVertex(GetStopJunction(stopCoords, stopFromId),
                        GetStopJunction(stopCoords, stopToId), FakeVertex::Type::PureFake);
  m_fake.AddStandaloneVertex(edgeSegment, edgeVertex);
  m_segmentToEdge[edgeSegment] = edge;
  m_stopToBack[stopFromId].insert(edgeSegment);
  m_stopToFront[stopToId].insert(edgeSegment);
  return edgeSegment;
}

void TransitGraph::AddConnections(map<transit::StopId, set<Segment>> const & connections,
                                  bool isOutgoing)
{
  for (auto const & connection : connections)
  {
    for (auto const & connectedSegment : connection.second)
    {
      auto const & adjacentSegments = isOutgoing ? m_stopToFront : m_stopToBack;
      auto const segmentsIt = adjacentSegments.find(connection.first);
      if (segmentsIt == adjacentSegments.cend())
        continue;
      for (auto const & segment : segmentsIt->second)
      {
        m_fake.AddConnection(isOutgoing ? segment : connectedSegment,
                             isOutgoing ? connectedSegment : segment);
      }
    }
  }
}
}  // namespace routing
