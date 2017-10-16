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

void TransitGraph::GetTransitEdges(Segment const & segment, bool isOutgoing,
                                   vector<SegmentEdge> & edges,
                                   EdgeEstimator const & estimator) const
{
  CHECK(IsTransitSegment(segment), ("Nontransit segment passed to TransitGraph."));
  for (auto const & s : m_fake.GetEdges(segment, isOutgoing))
    edges.emplace_back(s, CalcSegmentWeight(isOutgoing ? s : segment, estimator));
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
                        vector<transit::Edge> const & edges, EdgeEstimator const & estimator,
                        NumMwmId numMwmId, IndexGraph & indexGraph)
{
  m_mwmId = numMwmId;
  map<transit::StopId, Junction> stopCoords;
  for (auto const & stop : stops)
    stopCoords[stop.GetId()] = Junction(stop.GetPoint(), feature::kDefaultAltitudeMeters);

  for (auto const & gate : gates)
  {
    // TODO (@t.yan) after https://github.com/mapsme/omim/pull/7240 merge
    // auto const gateSegment = gate.GetBestPedestrianSegment();
    // Segment real(numMwmId, gateSegment.GetFeatureId(), gateSegment.GetSegmentIdx(), gateSegment.GetForward());
    auto const ending =
        MakeFakeEnding(Segment() /* real */, gate.GetPoint(), estimator, indexGraph);
    if (gate.GetEntrance())
      AddGate(gate, ending, stopCoords, true /* isEnter */);
    if (gate.GetExit())
      AddGate(gate, ending, stopCoords, false /* isEnter */);
  }

  map<transit::StopId, set<transit::Edge>> outgoing;
  map<transit::StopId, set<transit::Edge>> ingoing;
  for (auto const & edge : edges)
  {
    AddEdge(edge, stopCoords);
    outgoing[edge.GetStop1Id()].insert(edge);
    ingoing[edge.GetStop2Id()].insert(edge);
  }

  AddConnections(outgoing, true /* isOutgoing */);
  AddConnections(ingoing, false /* isOutgoing */);
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

void TransitGraph::AddEdge(transit::Edge const & edge,
                           map<transit::StopId, Junction> const & stopCoords)
{
  auto const edgeSegment = GetNewTransitSegment();
  auto const startStopId = edge.GetStop1Id();
  auto const finishStopId = edge.GetStop2Id();
  auto const startStopIt = stopCoords.find(startStopId);
  CHECK(startStopIt != stopCoords.end(), ("Stop", startStopId, "does not exist."));
  auto const finishStopIt = stopCoords.find(finishStopId);
  CHECK(finishStopIt != stopCoords.end(), ("Stop", finishStopId, "does not exist."));
  FakeVertex edgeVertex(startStopIt->second, finishStopIt->second, FakeVertex::Type::PureFake);
  m_fake.AddStandaloneVertex(edgeSegment, edgeVertex);
  m_segmentToEdge[edgeSegment] = edge;
  m_edgeToSegment[edge] = edgeSegment;
  m_stopToBack[startStopId].insert(edgeSegment);
  m_stopToFront[finishStopId].insert(edgeSegment);
}

void TransitGraph::AddConnections(map<transit::StopId, set<transit::Edge>> const & connections,
                                  bool isOutgoing)
{
  for (auto const & connection : connections)
  {
    for (auto const & edge : connection.second)
    {
      auto const edgeIt = m_edgeToSegment.find(edge);
      CHECK(edgeIt != m_edgeToSegment.cend(), ("Unknown edge", edge));
      auto const & adjacentSegments = isOutgoing ? m_stopToFront : m_stopToBack;
      auto const segmentsIt = adjacentSegments.find(connection.first);
      if (segmentsIt == adjacentSegments.cend())
        continue;
      for (auto const & segment : segmentsIt->second)
      {
        m_fake.AddConnection(isOutgoing ? segment : edgeIt->second,
                             isOutgoing ? edgeIt->second : segment);
      }
    }
  }
}

bool TransitGraph::IsGate(Segment const & segment) const
{
  return m_segmentToGate.count(segment) > 0;
}

bool TransitGraph::IsEdge(Segment const & segment) const
{
  return m_segmentToEdge.count(segment) > 0;
}

transit::Edge const & TransitGraph::GetEdge(Segment const & segment) const
{
  auto const it = m_segmentToEdge.find(segment);
  CHECK(it != m_segmentToEdge.cend(), ("Unknown transit segment."));
  return it->second;
}

transit::Gate const & TransitGraph::GetGate(Segment const & segment) const
{
  auto const it = m_segmentToGate.find(segment);
  CHECK(it != m_segmentToGate.cend(), ("Unknown transit segment."));
  return it->second;
}
}  // namespace routing
