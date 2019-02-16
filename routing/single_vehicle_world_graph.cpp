#include "routing/single_vehicle_world_graph.hpp"

#include <utility>

namespace routing
{
using namespace std;

SingleVehicleWorldGraph::SingleVehicleWorldGraph(unique_ptr<CrossMwmGraph> crossMwmGraph,
                                                 unique_ptr<IndexGraphLoader> loader,
                                                 shared_ptr<EdgeEstimator> estimator)
  : m_crossMwmGraph(move(crossMwmGraph)), m_loader(move(loader)), m_estimator(estimator)
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
  for (size_t i = 0; i < jointEdges.size(); ++i)
  {
    JointSegment const & target = jointEdges[i].GetTarget();
    if (!m_crossMwmGraph->IsFeatureTransit(target.GetMwmId(), target.GetFeatureId()))
      continue;

    auto & currentIndexGraph = GetIndexGraph(parent.GetMwmId());

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

      if (auto edge = currentIndexGraph.GetJointEdgeByLastPoint(parent, target.GetSegment(!opposite),
                                                                isOutgoing, lastPoints.back()))
      {
        newCrossMwmEdges.emplace_back(*edge);
        newCrossMwmEdges.back().GetTarget().GetFeatureId() = twinFeatureId;
        newCrossMwmEdges.back().GetTarget().GetMwmId() = twinMwmId;

        parentWeights.emplace_back(parentWeights[i]);
      }
    }
  }

  jointEdges.insert(jointEdges.end(), newCrossMwmEdges.begin(), newCrossMwmEdges.end());
}

void SingleVehicleWorldGraph::GetEdgeList(Segment const & parent, bool isOutgoing,
                                          vector<JointEdge> & jointEdges,
                                          vector<RouteWeight> & parentWeights)
{
  // Fake segments aren't processed here. All work must be done
  // on the IndexGraphStarterJoints abstraction-level.
  if (!parent.IsRealSegment())
    return;

  auto & indexGraph = GetIndexGraph(parent.GetMwmId());
  indexGraph.GetEdgeList(parent, isOutgoing, jointEdges, parentWeights);

  if (m_mode != WorldGraph::Mode::JointSingleMwm)
    CheckAndProcessTransitFeatures(parent, jointEdges, parentWeights, isOutgoing);
}

void SingleVehicleWorldGraph::GetEdgeList(Segment const & segment, bool isOutgoing,
                                          vector<SegmentEdge> & edges)
{
  if (m_mode == Mode::LeapsOnly)
  {
    CHECK(m_crossMwmGraph, ());
    // Ingoing edges listing is not supported for leaps because we do not have enough information
    // to calculate |segment| weight. See https://jira.mail.ru/browse/MAPSME-5743 for details.
    CHECK(isOutgoing, ("Ingoing edges listing is not supported for LeapsOnly mode."));
    if (m_crossMwmGraph->IsTransition(segment, isOutgoing))
      GetTwins(segment, isOutgoing, edges);
    else
      m_crossMwmGraph->GetOutgoingEdgeList(segment, edges);
    return;
  }

  IndexGraph & indexGraph = m_loader->GetIndexGraph(segment.GetMwmId());
  indexGraph.GetEdgeList(segment, isOutgoing, edges);

  if (m_mode != Mode::SingleMwm && m_crossMwmGraph && m_crossMwmGraph->IsTransition(segment, isOutgoing))
    GetTwins(segment, isOutgoing, edges);
}

Junction const & SingleVehicleWorldGraph::GetJunction(Segment const & segment, bool front)
{
  return GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId())
      .GetJunction(segment.GetPointId(front));
}

m2::PointD const & SingleVehicleWorldGraph::GetPoint(Segment const & segment, bool front)
{
  return GetJunction(segment, front).GetPoint();
}

bool SingleVehicleWorldGraph::IsOneWay(NumMwmId mwmId, uint32_t featureId)
{
  return GetRoadGeometry(mwmId, featureId).IsOneWay();
}

bool SingleVehicleWorldGraph::IsPassThroughAllowed(NumMwmId mwmId, uint32_t featureId)
{
  return GetRoadGeometry(mwmId, featureId).IsPassThroughAllowed();
}

void SingleVehicleWorldGraph::GetOutgoingEdgesList(Segment const & segment,
                                                   vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, true /* isOutgoing */, edges);
}

void SingleVehicleWorldGraph::GetIngoingEdgesList(Segment const & segment,
                                                  vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, false /* isOutgoing */, edges);
}

RouteWeight SingleVehicleWorldGraph::HeuristicCostEstimate(Segment const & from, Segment const & to)
{
  return HeuristicCostEstimate(GetPoint(from, true /* front */), GetPoint(to, true /* front */));
}


RouteWeight SingleVehicleWorldGraph::HeuristicCostEstimate(Segment const & from, m2::PointD const & to)
{
  return HeuristicCostEstimate(GetPoint(from, true /* front */), to);
}

RouteWeight SingleVehicleWorldGraph::HeuristicCostEstimate(m2::PointD const & from,
                                                           m2::PointD const & to)
{
  return RouteWeight(m_estimator->CalcHeuristic(from, to));
}

RouteWeight SingleVehicleWorldGraph::CalcSegmentWeight(Segment const & segment)
{
  return RouteWeight(m_estimator->CalcSegmentWeight(
      segment, GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId())));
}

RouteWeight SingleVehicleWorldGraph::CalcLeapWeight(m2::PointD const & from,
                                                    m2::PointD const & to) const
{
  return RouteWeight(m_estimator->CalcLeapWeight(from, to));
}

RouteWeight SingleVehicleWorldGraph::CalcOffroadWeight(m2::PointD const & from,
                                                       m2::PointD const & to) const
{
  return RouteWeight(m_estimator->CalcOffroadWeight(from, to));
}

double SingleVehicleWorldGraph::CalcSegmentETA(Segment const & segment)
{
  return m_estimator->CalcSegmentETA(segment, GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId()));
}

bool SingleVehicleWorldGraph::LeapIsAllowed(NumMwmId mwmId) const
{
  return m_estimator->LeapIsAllowed(mwmId);
}

vector<Segment> const & SingleVehicleWorldGraph::GetTransitions(NumMwmId numMwmId, bool isEnter)
{
  return m_crossMwmGraph->GetTransitions(numMwmId, isEnter);
}

unique_ptr<TransitInfo> SingleVehicleWorldGraph::GetTransitInfo(Segment const &) { return {}; }

vector<RouteSegment::SpeedCamera> SingleVehicleWorldGraph::GetSpeedCamInfo(Segment const & segment)
{
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
}  // namespace routing
