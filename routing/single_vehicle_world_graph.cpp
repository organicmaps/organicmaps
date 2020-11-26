#include "routing/single_vehicle_world_graph.hpp"

#include "routing/base/astar_algorithm.hpp"

#include <algorithm>
#include <utility>

#include "base/assert.hpp"

namespace routing
{
using namespace std;

template <>
SingleVehicleWorldGraph::AStarParents<Segment>::ParentType
SingleVehicleWorldGraph::AStarParents<Segment>::kEmpty = {};

template <>
SingleVehicleWorldGraph::AStarParents<JointSegment>::ParentType
SingleVehicleWorldGraph::AStarParents<JointSegment>::kEmpty = {};

SingleVehicleWorldGraph::SingleVehicleWorldGraph(unique_ptr<CrossMwmGraph> crossMwmGraph,
                                                 unique_ptr<IndexGraphLoader> loader,
                                                 shared_ptr<EdgeEstimator> estimator,
                                                 MwmHierarchyHandler && hierarchyHandler)
  : m_crossMwmGraph(move(crossMwmGraph))
  , m_loader(move(loader))
  , m_estimator(move(estimator))
  , m_hierarchyHandler(std::move(hierarchyHandler))
{
  CHECK(m_loader, ());
  CHECK(m_estimator, ());
}

void SingleVehicleWorldGraph::CheckAndProcessTransitFeatures(Segment const & parent,
                                                             vector<JointEdge> & jointEdges,
                                                             vector<RouteWeight> & parentWeights,
                                                             bool isOutgoing)
{
  bool opposite = !isOutgoing;
  vector<JointEdge> newCrossMwmEdges;

  NumMwmId const mwmId = parent.GetMwmId();

  for (size_t i = 0; i < jointEdges.size(); ++i)
  {
    JointSegment const & target = jointEdges[i].GetTarget();

    NumMwmId const edgeMwmId = target.GetMwmId();

    if (!m_crossMwmGraph->IsFeatureTransit(edgeMwmId, target.GetFeatureId()))
      continue;

    auto & currentIndexGraph = GetIndexGraph(mwmId);

    vector<Segment> twins;
    m_crossMwmGraph->GetTwinFeature(target.GetSegment(true /* start */), isOutgoing, twins);
    for (auto const & twin : twins)
    {
      NumMwmId const twinMwmId = twin.GetMwmId();

      uint32_t const twinFeatureId = twin.GetFeatureId();

      Segment const start(twinMwmId, twinFeatureId, target.GetSegmentId(!opposite), target.IsForward());

      auto & twinIndexGraph = GetIndexGraph(twinMwmId);

      vector<uint32_t> lastPoints;
      twinIndexGraph.GetLastPointsForJoint({start}, isOutgoing, lastPoints);
      ASSERT_EQUAL(lastPoints.size(), 1, ());

      if (auto edge = currentIndexGraph.GetJointEdgeByLastPoint(parent,
                                                                target.GetSegment(!opposite),
                                                                isOutgoing, lastPoints.back()))
      {
        newCrossMwmEdges.emplace_back(*edge);
        newCrossMwmEdges.back().GetTarget().SetFeatureId(twinFeatureId);
        newCrossMwmEdges.back().GetTarget().SetMwmId(twinMwmId);
        newCrossMwmEdges.back().GetWeight() +=
            m_hierarchyHandler.GetCrossBorderPenalty(mwmId, twinMwmId);

        parentWeights.emplace_back(parentWeights[i]);
      }
    }
  }

  jointEdges.insert(jointEdges.end(), newCrossMwmEdges.begin(), newCrossMwmEdges.end());
}

void SingleVehicleWorldGraph::GetEdgeList(
    astar::VertexData<Segment, RouteWeight> const & vertexData, bool isOutgoing,
    bool useRoutingOptions, bool useAccessConditional, vector<SegmentEdge> & edges)
{
  CHECK_NOT_EQUAL(m_mode, WorldGraphMode::LeapsOnly, ());

  ASSERT(m_parentsForSegments.forward && m_parentsForSegments.backward,
         ("m_parentsForSegments was not initialized in SingleVehicleWorldGraph."));
  auto const & segment = vertexData.m_vertex;
  auto & parents = isOutgoing ? *m_parentsForSegments.forward : *m_parentsForSegments.backward;
  IndexGraph & indexGraph = m_loader->GetIndexGraph(segment.GetMwmId());
  indexGraph.GetEdgeList(vertexData, isOutgoing, useRoutingOptions, edges, parents);

  if (m_mode != WorldGraphMode::SingleMwm && m_crossMwmGraph && m_crossMwmGraph->IsTransition(segment, isOutgoing))
    GetTwins(segment, isOutgoing, useRoutingOptions, edges);
}

void SingleVehicleWorldGraph::GetEdgeList(
    astar::VertexData<JointSegment, RouteWeight> const & parentVertexData, Segment const & parent,
    bool isOutgoing, bool useAccessConditional, vector<JointEdge> & jointEdges,
    vector<RouteWeight> & parentWeights)
{
  // Fake segments aren't processed here. All work must be done
  // on the IndexGraphStarterJoints abstraction-level.
  if (!parent.IsRealSegment())
    return;

  ASSERT(m_parentsForJoints.forward && m_parentsForJoints.backward,
         ("m_parentsForJoints was not initialized in SingleVehicleWorldGraph."));
  auto & parents = isOutgoing ? *m_parentsForJoints.forward : *m_parentsForJoints.backward;
  auto & indexGraph = GetIndexGraph(parent.GetMwmId());
  indexGraph.GetEdgeList(parentVertexData, parent, isOutgoing, jointEdges, parentWeights, parents);

  if (m_mode != WorldGraphMode::JointSingleMwm)
    CheckAndProcessTransitFeatures(parent, jointEdges, parentWeights, isOutgoing);
}

LatLonWithAltitude const & SingleVehicleWorldGraph::GetJunction(Segment const & segment,
                                                                         bool front)
{
  return GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId())
         .GetJunction(segment.GetPointId(front));
}

ms::LatLon const & SingleVehicleWorldGraph::GetPoint(Segment const & segment, bool front)
{
  return GetJunction(segment, front).GetLatLon();
}

bool SingleVehicleWorldGraph::IsOneWay(NumMwmId mwmId, uint32_t featureId)
{
  return GetRoadGeometry(mwmId, featureId).IsOneWay();
}

bool SingleVehicleWorldGraph::IsPassThroughAllowed(NumMwmId mwmId, uint32_t featureId)
{
  return GetRoadGeometry(mwmId, featureId).IsPassThroughAllowed();
}

RouteWeight SingleVehicleWorldGraph::HeuristicCostEstimate(ms::LatLon const & from,
                                                           ms::LatLon const & to)
{
  return RouteWeight(m_estimator->CalcHeuristic(from, to));
}


RouteWeight SingleVehicleWorldGraph::CalcSegmentWeight(Segment const & segment,
                                                       EdgeEstimator::Purpose purpose)
{
  return RouteWeight(m_estimator->CalcSegmentWeight(
      segment, GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId()), purpose));
}

RouteWeight SingleVehicleWorldGraph::CalcLeapWeight(ms::LatLon const & from,
                                                    ms::LatLon const & to,
                                                    NumMwmId mwmId) const
{
  return RouteWeight(m_estimator->CalcLeapWeight(from, to, mwmId));
}

RouteWeight SingleVehicleWorldGraph::CalcOffroadWeight(ms::LatLon const & from,
                                                       ms::LatLon const & to,
                                                       EdgeEstimator::Purpose purpose) const
{
  return RouteWeight(m_estimator->CalcOffroad(from, to, purpose));
}

double SingleVehicleWorldGraph::CalculateETA(Segment const & from, Segment const & to)
{
  if (from.GetMwmId() != to.GetMwmId())
    return CalculateETAWithoutPenalty(to);

  auto & indexGraph = m_loader->GetIndexGraph(from.GetMwmId());
  return indexGraph.CalculateEdgeWeight(EdgeEstimator::Purpose::ETA, true /* isOutgoing */, from, to).GetWeight();
}

double SingleVehicleWorldGraph::CalculateETAWithoutPenalty(Segment const & segment)
{
  return m_estimator->CalcSegmentWeight(segment,
                                        GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId()),
                                        EdgeEstimator::Purpose::ETA);
}

vector<Segment> const & SingleVehicleWorldGraph::GetTransitions(NumMwmId numMwmId, bool isEnter)
{
  return m_crossMwmGraph->GetTransitions(numMwmId, isEnter);
}

unique_ptr<TransitInfo> SingleVehicleWorldGraph::GetTransitInfo(Segment const &) { return {}; }

vector<RouteSegment::SpeedCamera> SingleVehicleWorldGraph::GetSpeedCamInfo(Segment const & segment)
{
  ASSERT(segment.IsRealSegment(), ());
  return m_loader->GetSpeedCameraInfo(segment);
}

RoadGeometry const & SingleVehicleWorldGraph::GetRoadGeometry(NumMwmId mwmId, uint32_t featureId)
{
  return m_loader->GetGeometry(mwmId).GetRoad(featureId);
}

void SingleVehicleWorldGraph::GetTwinsInner(Segment const & segment, bool isOutgoing,
                                            vector<Segment> & twins)
{
  m_crossMwmGraph->GetTwins(segment, isOutgoing, twins);
}

bool SingleVehicleWorldGraph::IsRoutingOptionsGood(Segment const & segment)
{
  auto const & geometry = GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId());
  return geometry.SuitableForOptions(m_avoidRoutingOptions);
}

RoutingOptions SingleVehicleWorldGraph::GetRoutingOptions(Segment const & segment)
{
  ASSERT(segment.IsRealSegment(), ());

  auto const & geometry = GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId());
  return geometry.GetRoutingOptions();
}

void SingleVehicleWorldGraph::SetAStarParents(bool forward, Parents<Segment> & parents)
{
  if (forward)
    m_parentsForSegments.forward = &parents;
  else
    m_parentsForSegments.backward = &parents;
}

void SingleVehicleWorldGraph::SetAStarParents(bool forward, Parents<JointSegment> & parents)
{
  if (forward)
    m_parentsForJoints.forward = &parents;
  else
    m_parentsForJoints.backward = &parents;
}

void SingleVehicleWorldGraph::DropAStarParents()
{
  m_parentsForJoints.backward = &AStarParents<JointSegment>::kEmpty;
  m_parentsForJoints.forward = &AStarParents<JointSegment>::kEmpty;

  m_parentsForSegments.backward = &AStarParents<Segment>::kEmpty;
  m_parentsForSegments.forward = &AStarParents<Segment>::kEmpty;
}

template <typename VertexType>
NumMwmId GetCommonMwmInChain(vector<VertexType> const & chain)
{
  NumMwmId mwmId = kFakeNumMwmId;
  for (size_t i = 0; i < chain.size(); ++i)
  {
    if (chain[i].GetMwmId() == kFakeNumMwmId)
      continue;

    if (mwmId != kFakeNumMwmId && mwmId != chain[i].GetMwmId())
      return kFakeNumMwmId;

    mwmId = chain[i].GetMwmId();
  }

  return mwmId;
}

template <typename VertexType>
bool
SingleVehicleWorldGraph::AreWavesConnectibleImpl(Parents<VertexType> const & forwardParents,
                                                 VertexType const & commonVertex,
                                                 Parents<VertexType> const & backwardParents,
                                                 function<uint32_t(VertexType const &)> && fakeFeatureConverter)
{
  if (IsRegionsGraphMode())
    return true;

  vector<VertexType> chain;
  auto const fillUntilNextFeatureId = [&](VertexType const & cur, auto const & parents)
  {
    auto startFeatureId = cur.GetFeatureId();
    auto it = parents.find(cur);
    while (it != parents.end() && it->second.GetFeatureId() == startFeatureId)
    {
      chain.emplace_back(it->second);
      it = parents.find(it->second);
    }

    if (it == parents.end())
      return false;

    chain.emplace_back(it->second);
    return true;
  };

  auto const fillParents = [&](VertexType const & start, auto const & parents)
  {
    VertexType current = start;
    static uint32_t constexpr kStepCountOneSide = 3;
    for (uint32_t i = 0; i < kStepCountOneSide; ++i)
    {
      if (!fillUntilNextFeatureId(current, parents))
        break;

      current = chain.back();
    }
  };

  fillParents(commonVertex, forwardParents);

  reverse(chain.begin(), chain.end());
  chain.emplace_back(commonVertex);

  fillParents(commonVertex, backwardParents);

  if (fakeFeatureConverter)
  {
    for (size_t i = 0; i < chain.size(); ++i)
    {
      if (!chain[i].IsRealSegment())
        chain[i].SetFeatureId(fakeFeatureConverter(chain[i]));
    }
  }

  NumMwmId const mwmId = GetCommonMwmInChain(chain);
  if (mwmId == kFakeNumMwmId)
    return true;

  Parents<VertexType> parents;
  for (size_t i = 1; i < chain.size(); ++i)
    parents[chain[i]] = chain[i - 1];

  auto & currentIndexGraph = GetIndexGraph(mwmId);
  for (size_t i = 1; i < chain.size(); ++i)
  {
    auto const & parent = chain[i - 1];
    uint32_t const parentFeatureId = chain[i - 1].GetFeatureId();
    uint32_t const currentFeatureId = chain[i].GetFeatureId();

    if (parentFeatureId != currentFeatureId &&
        currentIndexGraph.IsRestricted(parent, parentFeatureId, currentFeatureId,
                                       true /* isOutgoing */, parents))
    {
      return false;
    }
  }

  return true;
}

bool SingleVehicleWorldGraph::AreWavesConnectible(Parents<Segment> & forwardParents,
                                                  Segment const & commonVertex,
                                                  Parents<Segment> & backwardParents,
                                                  function<uint32_t(Segment const &)> && fakeFeatureConverter)
{
  return AreWavesConnectibleImpl(forwardParents, commonVertex, backwardParents, move(fakeFeatureConverter));
}

bool SingleVehicleWorldGraph::AreWavesConnectible(Parents<JointSegment> & forwardParents,
                                                  JointSegment const & commonVertex,
                                                  Parents<JointSegment> & backwardParents,
                                                  function<uint32_t(JointSegment const &)> && fakeFeatureConverter)
{
  return AreWavesConnectibleImpl(forwardParents, commonVertex, backwardParents, move(fakeFeatureConverter));
}
}  // namespace routing
