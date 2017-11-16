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

void SingleVehicleWorldGraph::GetEdgeList(Segment const & segment, bool isOutgoing, bool isLeap,
                                          bool isEnding, vector<SegmentEdge> & edges)
{
  // If mode is LeapsOnly and |isEnding| == true we need to connect segment to transitions.
  // If |isOutgoing| == true connects |segment| with all exits of mwm.
  // If |isOutgoing| == false connects all enters to mwm with |segment|.
  if (m_mode == Mode::LeapsOnly && isEnding)
  {
    edges.clear();
    m2::PointD const & segmentPoint = GetPoint(segment, true /* front */);

    // Note. If |isOutgoing| == true it's necessary to add edges which connect the start with all
    // exits of its mwm. So |isEnter| below should be set to false.
    // If |isOutgoing| == false all enters of the finish mwm should be connected with the finish
    // point. So |isEnter| below should be set to true.
    m_crossMwmGraph->ForEachTransition(
        segment.GetMwmId(), !isOutgoing /* isEnter */, [&](Segment const & transition) {
          edges.emplace_back(
              transition, RouteWeight(m_estimator->CalcLeapWeight(segmentPoint,
                                                                  GetPoint(transition, isOutgoing)),
                                      0 /* nonPassThroughCross */));
        });
    return;
  }

  if (m_mode != Mode::NoLeaps && m_mode != Mode::SingleMwm && (isLeap || m_mode == Mode::LeapsOnly))
  {
    CHECK(m_crossMwmGraph, ());
    if (m_crossMwmGraph->IsTransition(segment, isOutgoing))
      GetTwins(segment, isOutgoing, edges);
    else
      m_crossMwmGraph->GetEdgeList(segment, isOutgoing, edges);
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
  GetEdgeList(segment, true /* isOutgoing */, false /* isLeap */, false /* isEnding */, edges);
}

void SingleVehicleWorldGraph::GetIngoingEdgesList(Segment const & segment,
                                                  vector<SegmentEdge> & edges)
{
  edges.clear();
  GetEdgeList(segment, false /* isOutgoing */, false /* isLeap */, false /* isEnding */, edges);
}

RouteWeight SingleVehicleWorldGraph::HeuristicCostEstimate(Segment const & from, Segment const & to)
{
  return HeuristicCostEstimate(GetPoint(from, true /* front */), GetPoint(to, true /* front */));
}

RouteWeight SingleVehicleWorldGraph::HeuristicCostEstimate(m2::PointD const & from,
                                                           m2::PointD const & to)
{
  return RouteWeight(m_estimator->CalcHeuristic(from, to), 0 /* nonPassThroughCross */);
}

RouteWeight SingleVehicleWorldGraph::CalcSegmentWeight(Segment const & segment)
{
  return RouteWeight(m_estimator->CalcSegmentWeight(
                         segment, GetRoadGeometry(segment.GetMwmId(), segment.GetFeatureId())),
                     0 /* nonPassThroughCross */);
}

RouteWeight SingleVehicleWorldGraph::CalcOffroadWeight(m2::PointD const & from,
                                                       m2::PointD const & to) const
{
  return RouteWeight(m_estimator->CalcOffroadWeight(from, to), 0 /* nonPassThroughCross */);
}

bool SingleVehicleWorldGraph::LeapIsAllowed(NumMwmId mwmId) const
{
  return m_estimator->LeapIsAllowed(mwmId);
}

unique_ptr<TransitInfo> SingleVehicleWorldGraph::GetTransitInfo(Segment const &) { return {}; }

RoadGeometry const & SingleVehicleWorldGraph::GetRoadGeometry(NumMwmId mwmId, uint32_t featureId)
{
  return m_loader->GetIndexGraph(mwmId).GetGeometry().GetRoad(featureId);
}

void SingleVehicleWorldGraph::GetTwins(Segment const & segment, bool isOutgoing,
                                       vector<SegmentEdge> & edges)
{
  m_twins.clear();
  m_crossMwmGraph->GetTwins(segment, isOutgoing, m_twins);
  for (Segment const & twin : m_twins)
  {
    m2::PointD const & from = GetPoint(segment, true /* front */);
    m2::PointD const & to = GetPoint(twin, true /* front */);
    // Weight is usually zero because twins correspond the same feature
    // in different mwms. But if we have mwms with different versions and feature
    // was moved in one of them we can have nonzero weight here.
    double const weight = m_estimator->CalcHeuristic(from, to);
    edges.emplace_back(twin, RouteWeight(weight, 0 /* nonPassThroughCross */));
  }
}
}  // namespace routing
