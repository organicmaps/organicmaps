#include "routing/road_graph.hpp"
#include "routing/routing_algorithm.hpp"
#include "routing/base/astar_algorithm.hpp"

#include "base/assert.hpp"

#include "indexer/mercator.hpp"

namespace routing
{

namespace
{
double constexpr KMPH2MPS = 1000.0 / (60 * 60);

inline double TimeBetweenSec(Junction const & j1, Junction const & j2, double speedMPS)
{
  ASSERT(speedMPS > 0.0, ());
  return MercatorBounds::DistanceOnEarth(j1.GetPoint(), j2.GetPoint()) / speedMPS;
}

/// A class which represents an weighted edge used by RoadGraph.
class WeightedEdge
{
public:
  WeightedEdge(Junction const & target, double weight) : target(target), weight(weight) {}

  inline Junction const & GetTarget() const { return target; }

  inline double GetWeight() const { return weight; }

private:
  Junction const target;
  double const weight;
};

/// A wrapper around IRoadGraph, which makes it possible to use IRoadGraph with astar algorithms.
class RoadGraph
{
public:
  using TVertexType = Junction;
  using TEdgeType = WeightedEdge;

  RoadGraph(IRoadGraph const & roadGraph)
    : m_roadGraph(roadGraph)
    , m_maxSpeedMPS(roadGraph.GetMaxSpeedKMPH() * KMPH2MPS)
  {}

  void GetOutgoingEdgesList(Junction const & v, vector<WeightedEdge> & adj) const
  {
    IRoadGraph::TEdgeVector edges;
    m_roadGraph.GetOutgoingEdges(v, edges);

    adj.clear();
    adj.reserve(edges.size());

    for (auto const & e : edges)
    {
      ASSERT_EQUAL(v, e.GetStartJunction(), ());

      double const speedMPS = m_roadGraph.GetSpeedKMPH(e) * KMPH2MPS;
      adj.emplace_back(e.GetEndJunction(), TimeBetweenSec(e.GetStartJunction(), e.GetEndJunction(), speedMPS));
    }
  }

  void GetIngoingEdgesList(Junction const & v, vector<WeightedEdge> & adj) const
  {
    IRoadGraph::TEdgeVector edges;
    m_roadGraph.GetIngoingEdges(v, edges);

    adj.clear();
    adj.reserve(edges.size());

    for (auto const & e : edges)
    {
      ASSERT_EQUAL(v, e.GetEndJunction(), ());

      double const speedMPS = m_roadGraph.GetSpeedKMPH(e) * KMPH2MPS;
      adj.emplace_back(e.GetStartJunction(), TimeBetweenSec(e.GetStartJunction(), e.GetEndJunction(), speedMPS));
    }
  }

  double HeuristicCostEstimate(Junction const & v, Junction const & w) const
  {
    return TimeBetweenSec(v, w, m_maxSpeedMPS);
  }

private:
  IRoadGraph const & m_roadGraph;
  double const m_maxSpeedMPS;
};

typedef AStarAlgorithm<RoadGraph> TAlgorithmImpl;

IRoutingAlgorithm::Result Convert(TAlgorithmImpl::Result value)
{
  switch (value)
  {
  case TAlgorithmImpl::Result::OK: return IRoutingAlgorithm::Result::OK;
  case TAlgorithmImpl::Result::NoPath: return IRoutingAlgorithm::Result::NoPath;
  case TAlgorithmImpl::Result::Cancelled: return IRoutingAlgorithm::Result::Cancelled;
  }
  ASSERT(false, ("Unexpected TAlgorithmImpl::Result value:", value));
  return IRoutingAlgorithm::Result::NoPath;
}
}  // namespace

string DebugPrint(IRoutingAlgorithm::Result const & value)
{
  switch (value)
  {
  case IRoutingAlgorithm::Result::OK:
    return "OK";
  case IRoutingAlgorithm::Result::NoPath:
    return "NoPath";
  case IRoutingAlgorithm::Result::Cancelled:
    return "Cancelled";
  }
  return string();
}

// *************************** Base class for AStar based routing algorithms ******************************

AStarRoutingAlgorithmBase::AStarRoutingAlgorithmBase(TRoutingVisualizerFn routingVisualizerFn)
{
  if (routingVisualizerFn != nullptr)
    m_onVisitJunctionFn = [=](Junction const & junction){ routingVisualizerFn(junction.GetPoint()); };
  else
    m_onVisitJunctionFn = nullptr;
}

// *************************** AStar routing algorithm implementation *************************************

AStarRoutingAlgorithm::AStarRoutingAlgorithm(TRoutingVisualizerFn routingVisualizerFn)
  : AStarRoutingAlgorithmBase(routingVisualizerFn)
{
}

IRoutingAlgorithm::Result AStarRoutingAlgorithm::CalculateRoute(IRoadGraph const &  graph,
                                                                Junction const & startPos, Junction const & finalPos,
                                                                vector<Junction> & path)
{
  my::Cancellable const & cancellable = *this;
  TAlgorithmImpl::Result const res = TAlgorithmImpl().FindPath(RoadGraph(graph), startPos, finalPos, path, cancellable, m_onVisitJunctionFn);
  return Convert(res);
}

// *************************** AStar-bidirectional routing algorithm implementation ***********************

AStarBidirectionalRoutingAlgorithm::AStarBidirectionalRoutingAlgorithm(TRoutingVisualizerFn routingVisualizerFn)
  : AStarRoutingAlgorithmBase(routingVisualizerFn)
{
}

IRoutingAlgorithm::Result AStarBidirectionalRoutingAlgorithm::CalculateRoute(IRoadGraph const &  graph,
                                                                             Junction const & startPos, Junction const & finalPos,
                                                                             vector<Junction> & path)
{
  my::Cancellable const & cancellable = *this;
  TAlgorithmImpl::Result const res = TAlgorithmImpl().FindPathBidirectional(RoadGraph(graph), startPos, finalPos, path, cancellable, m_onVisitJunctionFn);
  return Convert(res);
}

}  // namespace routing
