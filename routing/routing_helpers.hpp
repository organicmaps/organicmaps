#pragma once

#include "routing/directions_engine.hpp"
#include "routing/index_road_graph.hpp"
#include "routing/road_graph.hpp"
#include "routing/route.hpp"
#include "routing/traffic_stash.hpp"

#include "traffic/traffic_info.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/segment2d.hpp"

#include "base/cancellable.hpp"

#include <memory>
#include <queue>
#include <set>
#include <vector>

namespace routing
{
inline double KMPH2MPS(double kmph) { return kmph * 1000.0 / (60 * 60); }

template <typename Types>
bool IsCarRoad(Types const & types)
{
  return CarModel::AllLimitsInstance().HasRoadType(types);
}

/// \returns true when there exists a routing mode where the feature with |types| can be used.
template <typename Types>
bool IsRoad(Types const & types)
{
  return IsCarRoad(types) || PedestrianModel::AllLimitsInstance().HasRoadType(types) ||
         BicycleModel::AllLimitsInstance().HasRoadType(types);
}

void FillSegmentInfo(std::vector<Segment> const & segments, std::vector<Junction> const & junctions,
                     Route::TTurns const & turns, Route::TStreets const & streets,
                     Route::TTimes const & times, std::shared_ptr<TrafficStash> const & trafficStash,
                     std::vector<RouteSegment> & routeSegment);

void ReconstructRoute(IDirectionsEngine & engine, IndexRoadGraph const & graph,
                      std::shared_ptr<TrafficStash> const & trafficStash,
                      base::Cancellable const & cancellable, std::vector<Junction> const & path,
                      Route::TTimes && times, Route & route);

/// \brief Converts |edge| to |segment|.
/// \returns Segment() if mwm of |edge| is not alive.
Segment ConvertEdgeToSegment(NumMwmIds const & numMwmIds, Edge const & edge);

/// \returns true if |segment| crosses any side of |rect|.
bool SegmentCrossesRect(m2::Segment2D const & segment, m2::RectD const & rect);

// \returns true if any part of polyline |junctions| lay in |rect| and false otherwise.
bool RectCoversPolyline(IRoadGraph::JunctionVec const & junctions, m2::RectD const & rect);

/// \brief Checks is edge connected with world graph. Function does BFS while it finds some number
/// of edges,
/// if graph ends before this number is reached then junction is assumed as not connected to the
/// world graph.
template <typename Graph>
bool CheckGraphConnectivity(typename Graph::Vertex const & start, bool isOutgoing,
                            bool useRoutingOptions, size_t limit, Graph & graph,
                            std::set<typename Graph::Vertex> & marked)
{
  std::queue<typename Graph::Vertex> q;
  q.push(start);

  marked.insert(start);

  std::vector<typename Graph::Edge> edges;
  while (!q.empty() && marked.size() < limit)
  {
    auto const u = q.front();
    q.pop();

    edges.clear();

    // Note. If |isOutgoing| == true outgoing edges are looked for.
    // If |isOutgoing| == false it's the finish. So ingoing edges are looked for.
    graph.GetEdgeList(u, isOutgoing, useRoutingOptions, edges);
    for (auto const & edge : edges)
    {
      auto const & v = edge.GetTarget();
      if (marked.count(v) == 0)
      {
        q.push(v);
        marked.insert(v);
      }
    }
  }

  return marked.size() >= limit;
}
}  // namespace routing
