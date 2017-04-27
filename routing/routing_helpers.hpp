#pragma once

#include "routing/directions_engine.hpp"
#include "routing/road_graph.hpp"
#include "routing/route.hpp"
#include "routing/traffic_stash.hpp"

#include "traffic/traffic_info.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "base/cancellable.hpp"

#include "std/queue.hpp"
#include "std/set.hpp"
#include "std/shared_ptr.hpp"
#include "std/vector.hpp"

namespace routing
{
/// \returns true when there exists a routing mode where the feature with |types| can be used.
template <class TTypes>
bool IsRoad(TTypes const & types)
{
  return CarModel::AllLimitsInstance().HasRoadType(types) ||
         PedestrianModel::AllLimitsInstance().HasRoadType(types) ||
         BicycleModel::AllLimitsInstance().HasRoadType(types);
}

void ReconstructRoute(IDirectionsEngine & engine, RoadGraphBase const & graph,
                      shared_ptr<TrafficStash> const & trafficStash,
                      my::Cancellable const & cancellable, vector<Junction> & path, Route & route);

/// \brief Checks is edge connected with world graph. Function does BFS while it finds some number of edges,
/// if graph ends before this number is reached then junction is assumed as not connected to the world graph.
template <typename GraphEdge, typename Graph, typename GraphVertex, typename GetVertexByEdgeFn, typename GetOutgoingEdgesFn>
bool CheckGraphConnectivity(Graph & graph, GraphVertex const & vertex, size_t limit,
                            GetVertexByEdgeFn && getVertexByEdgeFn, GetOutgoingEdgesFn && getOutgoingEdgesFn)
{
  queue<GraphVertex> q;
  q.push(vertex);

  set<GraphVertex> visited;
  visited.insert(vertex);

  vector<GraphEdge> edges;
  while (!q.empty() && visited.size() < limit)
  {
    GraphVertex const u = q.front();
    q.pop();

    edges.clear();
    getOutgoingEdgesFn(graph, u, edges);
    for (GraphEdge const edge : edges)
    {
      GraphVertex const & v = getVertexByEdgeFn(edge);
      if (visited.count(v) == 0)
      {
        q.push(v);
        visited.insert(v);
      }
    }
  }

  return visited.size() >= limit;
}
}  // namespace rouing
