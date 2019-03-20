#pragma once

#include "routing/base/routing_result.hpp"
#include "routing/road_graph.hpp"
#include "routing/router.hpp"

#include <string>

namespace routing
{
class TestAStarBidirectionalAlgo
{
public:
  enum class Result
  {
    OK,
    NoPath,
    Cancelled
  };

  Result CalculateRoute(IRoadGraph const & graph, Junction const & startPos,
                        Junction const & finalPos,
                        RoutingResult<IRoadGraph::Vertex, IRoadGraph::Weight> & path);
};

std::string DebugPrint(TestAStarBidirectionalAlgo::Result const & result);
}  // namespace routing
