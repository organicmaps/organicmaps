#include "routing/routing_algorithm.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/astar_progress.hpp"
#include "routing/road_graph.hpp"

#include "base/assert.hpp"

#include "geometry/mercator.hpp"

namespace routing
{

namespace
{
uint32_t constexpr kVisitPeriod = 4;
float constexpr kProgressInterval = 2;

double constexpr KMPH2MPS = 1000.0 / (60 * 60);

inline double TimeBetweenSec(Junction const & j1, Junction const & j2, double speedMPS)
{
  ASSERT(speedMPS > 0.0, ());
  ASSERT_NOT_EQUAL(j1.GetAltitude(), feature::kInvalidAltitude, ());
  ASSERT_NOT_EQUAL(j2.GetAltitude(), feature::kInvalidAltitude, ());

  double const distanceM = MercatorBounds::DistanceOnEarth(j1.GetPoint(), j2.GetPoint());
  double const altitudeDiffM =
      static_cast<double>(j2.GetAltitude()) - static_cast<double>(j1.GetAltitude());
  return sqrt(distanceM * distanceM + altitudeDiffM * altitudeDiffM) / speedMPS;
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
  using Vertex = Junction;
  using Edge = WeightedEdge;
  using Weight = double;

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

// *************************** AStar routing algorithm implementation *************************************

IRoutingAlgorithm::Result AStarRoutingAlgorithm::CalculateRoute(IRoadGraph const & graph,
                                                                Junction const & startPos,
                                                                Junction const & finalPos,
                                                                RouterDelegate const & delegate,
                                                                RoutingResult<IRoadGraph::Vertex, IRoadGraph::Weight> & path)
{
  AStarProgress progress(0, 95);
  uint32_t visitCount = 0;

  auto onVisitJunctionFn = [&](Junction const & junction, Junction const & /* target */) {
    if (++visitCount % kVisitPeriod != 0)
      return;

    delegate.OnPointCheck(junction.GetPoint());
    auto const lastValue = progress.GetLastValue();
    auto const newValue = progress.GetProgressForDirectedAlgo(junction.GetPoint());
    if (newValue - lastValue > kProgressInterval)
      delegate.OnProgress(newValue);

  };

  my::Cancellable const & cancellable = delegate;
  progress.Initialize(startPos.GetPoint(), finalPos.GetPoint());
  RoadGraph roadGraph(graph);
  TAlgorithmImpl::Params params(roadGraph, startPos, finalPos, nullptr /* prevRoute */, cancellable,
                                onVisitJunctionFn, {} /* checkLength */);
  TAlgorithmImpl::Result const res = TAlgorithmImpl().FindPath(params, path);
  return Convert(res);
}

// *************************** AStar-bidirectional routing algorithm implementation ***********************

IRoutingAlgorithm::Result AStarBidirectionalRoutingAlgorithm::CalculateRoute(
    IRoadGraph const & graph, Junction const & startPos, Junction const & finalPos,
    RouterDelegate const & delegate, RoutingResult<IRoadGraph::Vertex, IRoadGraph::Weight> & path)
{
  AStarProgress progress(0, 95);
  uint32_t visitCount = 0;

  auto onVisitJunctionFn = [&](Junction const & junction, Junction const & target) {
    if (++visitCount % kVisitPeriod != 0)
      return;

    delegate.OnPointCheck(junction.GetPoint());
    auto const lastValue = progress.GetLastValue();
    auto const newValue =
        progress.GetProgressForBidirectedAlgo(junction.GetPoint(), target.GetPoint());
    if (newValue - lastValue > kProgressInterval)
      delegate.OnProgress(newValue);
  };

  my::Cancellable const & cancellable = delegate;
  progress.Initialize(startPos.GetPoint(), finalPos.GetPoint());
  RoadGraph roadGraph(graph);
  TAlgorithmImpl::Params params(roadGraph, startPos, finalPos, {} /* prevRoute */, cancellable,
                                onVisitJunctionFn, {} /* checkLength */);
  TAlgorithmImpl::Result const res = TAlgorithmImpl().FindPathBidirectional(params, path);
  return Convert(res);
}

}  // namespace routing
