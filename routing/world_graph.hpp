#pragma once

#include "routing/cross_mwm_graph.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
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

  void GetEdgeList(Segment const & segment, bool isOutgoing, bool isLeap,
                   std::vector<SegmentEdge> & edges);

  IndexGraph & GetIndexGraph(NumMwmId numMwmId) { return m_loader->GetIndexGraph(numMwmId); }
  EdgeEstimator const & GetEstimator() const { return *m_estimator; }

  m2::PointD const & GetPoint(Segment const & segment, bool front);
  RoadGeometry const & GetRoadGeometry(NumMwmId mwmId, uint32_t featureId);

  // Clear memory used by loaded index graphs.
  void ClearIndexGraphs() { m_loader->Clear(); }
  void SetMode(Mode mode) { m_mode = mode; }
  Mode GetMode() const { return m_mode; }

  // Interface for AStarAlgorithm:
  void GetOutgoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges);
  void GetIngoingEdgesList(Segment const & segment, vector<SegmentEdge> & edges);
  double HeuristicCostEstimate(Segment const & from, Segment const & to);

  template <typename Fn>
  void ForEachTransition(NumMwmId numMwmId, bool isEnter, Fn && fn)
  {
    m_crossMwmGraph->ForEachTransition(numMwmId, isEnter, std::forward<Fn>(fn));
  }

  bool IsTransition(Segment const & s, bool isOutgoing)
  {
    return m_crossMwmGraph->IsTransition(s, isOutgoing);
  }

private:
  void GetTwins(Segment const & s, bool isOutgoing, std::vector<SegmentEdge> & edges);

  std::unique_ptr<CrossMwmGraph> m_crossMwmGraph;
  std::unique_ptr<IndexGraphLoader> m_loader;
  std::shared_ptr<EdgeEstimator> m_estimator;
  std::vector<Segment> m_twins;
  Mode m_mode = Mode::NoLeaps;
};

std::string DebugPrint(WorldGraph::Mode mode);
}  // namespace routing
