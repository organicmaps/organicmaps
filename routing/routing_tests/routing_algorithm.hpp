#pragma once

#include "routing/base/routing_result.hpp"
#include "routing/road_graph.hpp"
#include "routing/router.hpp"

#include "base/cancellable.hpp"

#include "std/functional.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace routing
{

// IRoutingAlgorithm is an abstract interface of a routing algorithm,
// which searches the optimal way between two junctions on the graph
class IRoutingAlgorithm
{
public:
  enum class Result
  {
    OK,
    NoPath,
    Cancelled
  };

  virtual Result CalculateRoute(IRoadGraph const & graph, Junction const & startPos,
                                Junction const & finalPos, RouterDelegate const & delegate,
                                RoutingResult<IRoadGraph::Vertex, IRoadGraph::Weight> & path) = 0;
};

string DebugPrint(IRoutingAlgorithm::Result const & result);

// AStar routing algorithm implementation
class AStarRoutingAlgorithm : public IRoutingAlgorithm
{
public:
  // IRoutingAlgorithm overrides:
  Result CalculateRoute(IRoadGraph const & graph, Junction const & startPos,
                        Junction const & finalPos, RouterDelegate const & delegate,
                        RoutingResult<IRoadGraph::Vertex, IRoadGraph::Weight> & path) override;
};

// AStar-bidirectional routing algorithm implementation
class AStarBidirectionalRoutingAlgorithm : public IRoutingAlgorithm
{
public:
  // IRoutingAlgorithm overrides:
  Result CalculateRoute(IRoadGraph const & graph, Junction const & startPos,
                        Junction const & finalPos, RouterDelegate const & delegate,
                        RoutingResult<IRoadGraph::Vertex, IRoadGraph::Weight> & path) override;
};

}  // namespace routing
