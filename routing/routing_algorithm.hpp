#pragma once

#include "base/cancellable.hpp"

#include "routing/road_graph.hpp"
#include "routing/router.hpp"

#include "std/functional.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace routing
{

// IRoutingAlgorithm is an abstract interface of a routing algorithm,
// which searches the optimal way between two junctions on the graph
class IRoutingAlgorithm : public my::Cancellable
{
public:
  enum class Result
  {
    OK,
    NoPath,
    Cancelled
  };

  virtual Result CalculateRoute(IRoadGraph const &  graph,
                                Junction const & startPos, Junction const & finalPos,
                                vector<Junction> & path) = 0;
};

string DebugPrint(IRoutingAlgorithm::Result const & result);

// Base class for AStar-based routing algorithms
class AStarRoutingAlgorithmBase : public IRoutingAlgorithm
{
protected:
  AStarRoutingAlgorithmBase(TRoutingVisualizerFn routingVisualizerFn);

  std::function<void(Junction const &)> m_onVisitJunctionFn;
};

// AStar routing algorithm implementation
class AStarRoutingAlgorithm : public AStarRoutingAlgorithmBase
{
public:
  explicit AStarRoutingAlgorithm(TRoutingVisualizerFn routingVisualizerFn = nullptr);

  // IRoutingAlgorithm overrides:
  Result CalculateRoute(IRoadGraph const &  graph,
                        Junction const & startPos, Junction const & finalPos,
                        vector<Junction> & path) override;
};

// AStar-bidirectional routing algorithm implementation
class AStarBidirectionalRoutingAlgorithm : public AStarRoutingAlgorithmBase
{
public:
  explicit AStarBidirectionalRoutingAlgorithm(TRoutingVisualizerFn routingVisualizerFn = nullptr);

  // IRoutingAlgorithm overrides:
  Result CalculateRoute(IRoadGraph const &  graph,
                        Junction const & startPos, Junction const & finalPos,
                        vector<Junction> & path) override;
};

}  // namespace routing
