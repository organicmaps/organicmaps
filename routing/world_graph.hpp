#pragma once

#include "routing/cross_mwm_graph.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/segment.hpp"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace routing
{
class WorldGraph final
{
public:
  // AStarAlgorithm types aliases:
  using TVertexType = IndexGraph::TVertexType;
  using TEdgeType = IndexGraph::TEdgeType;
  using TWeightType = IndexGraph::TWeightType;

  // CheckGraphConnectivity() types aliases:
  using Vertex = TVertexType;
  using Edge = TEdgeType;

  enum class Mode
  {
    LeapsOnly,  // Mode for building a cross mwm route containing only leaps. In case of start and
                // finish they (start and finish) will be connected with all transition segments of
                // their mwm with leap (fake) edges.
    LeapsIfPossible,  // Mode for building cross mwm and single mwm routes. In case of cross mwm route
                      // if they are neighboring mwms the route will be made without leaps.
                      // If not the route is made with leaps for intermediate mwms.
    NoLeaps,  // Mode for building route and getting outgoing/ingoing edges without leaps at all.
  };

  WorldGraph(std::unique_ptr<CrossMwmGraph> crossMwmGraph, std::unique_ptr<IndexGraphLoader> loader,
             std::shared_ptr<EdgeEstimator> estimator);

  // |isEnding| == true iff |segment| is first or last segment of the route. Needed because first and
  // last segments may need special processing.
  void GetEdgeList(Segment const & segment, bool isOutgoing, bool isLeap,
                   bool isEnding, std::vector<SegmentEdge> & edges);

  IndexGraph & GetIndexGraphForTests(NumMwmId numMwmId) { return m_loader->GetIndexGraph(numMwmId); }
  void SetSingleMwmMode(NumMwmId numMwmId) { m_mwmId = numMwmId; }
  void UnsetSingleMwmMode() { m_mwmId = kFakeNumMwmId; }

  Junction const & GetJunction(Segment const & segment, bool front);
  m2::PointD const & GetPoint(Segment const & segment, bool front);
  RoadGeometry const & GetRoadGeometry(NumMwmId mwmId, uint32_t featureId);

  // Clear memory used by loaded index graphs.
  void ClearIndexGraphs() { m_loader->Clear(); }
  void SetMode(Mode mode) { m_mode = mode; }
  Mode GetMode() const { return m_mode; }

  // Interface for AStarAlgorithm:
  void GetOutgoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges);
  void GetIngoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges);

  RouteWeight HeuristicCostEstimate(Segment const & from, Segment const & to);
  RouteWeight HeuristicCostEstimate(m2::PointD const & from, m2::PointD const & to);
  RouteWeight CalcSegmentWeight(Segment const & segment);
  RouteWeight CalcLeapWeight(m2::PointD const & from, m2::PointD const & to) const;
  bool LeapIsAllowed(NumMwmId mwmId) const;

private:
  void GetTwins(Segment const & s, bool isOutgoing, std::vector<SegmentEdge> & edges);
  bool IsInSingleMwmMode() { return m_mwmId != kFakeNumMwmId; }

  std::unique_ptr<CrossMwmGraph> m_crossMwmGraph;
  std::unique_ptr<IndexGraphLoader> m_loader;
  std::shared_ptr<EdgeEstimator> m_estimator;
  std::vector<Segment> m_twins;
  Mode m_mode = Mode::NoLeaps;
  NumMwmId m_mwmId = kFakeNumMwmId;
};

std::string DebugPrint(WorldGraph::Mode mode);
}  // namespace routing
