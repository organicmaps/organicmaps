#pragma once

#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/road_graph.hpp"
#include "routing/segment.hpp"
#include "routing/transit_info.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/point2d.hpp"

#include <memory>
#include <string>
#include <vector>

namespace routing
{
class WorldGraph
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
    NoLeaps,    // Mode for building route and getting outgoing/ingoing edges without leaps at all.
    SingleMwm,  // Mode for building route and getting outgoing/ingoing edges within mwm source
                // segment belongs to.
  };

  // |isEnding| == true iff |segment| is first or last segment of the route. Needed because first and
  // last segments may need special processing.
  virtual void GetEdgeList(Segment const & segment, bool isOutgoing, bool isLeap, bool isEnding,
                           std::vector<SegmentEdge> & edges) = 0;

  virtual Junction const & GetJunction(Segment const & segment, bool front) = 0;
  virtual m2::PointD const & GetPoint(Segment const & segment, bool front) = 0;
  virtual bool IsOneWay(NumMwmId mwmId, uint32_t featureId) = 0;

  // Checks feature is allowed for through pass.
  // TODO (t.yan) it's better to use "transit" only for public transport. We should rename
  // IsTransitAllowed to something like IsThroughPassAllowed everywhere.
  virtual bool IsTransitAllowed(NumMwmId mwmId, uint32_t featureId) = 0;

  // Clear memory used by loaded graphs.
  virtual void ClearCachedGraphs() = 0;
  virtual void SetMode(Mode mode) = 0;
  virtual Mode GetMode() const = 0;

  // Interface for AStarAlgorithm:
  virtual void GetOutgoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges) = 0;
  virtual void GetIngoingEdgesList(Segment const & segment, std::vector<SegmentEdge> & edges) = 0;

  virtual RouteWeight HeuristicCostEstimate(Segment const & from, Segment const & to) = 0;
  virtual RouteWeight HeuristicCostEstimate(m2::PointD const & from, m2::PointD const & to) = 0;
  virtual RouteWeight CalcSegmentWeight(Segment const & segment) = 0;
  virtual RouteWeight CalcOffroadWeight(m2::PointD const & from, m2::PointD const & to) const = 0;
  virtual bool LeapIsAllowed(NumMwmId mwmId) const = 0;

  // Returns transit-specific information for segment. For nontransit segments returns nullptr.
  virtual std::unique_ptr<TransitInfo> GetTransitInfo(Segment const & segment) = 0;
};

std::string DebugPrint(WorldGraph::Mode mode);
}  // namespace routing
