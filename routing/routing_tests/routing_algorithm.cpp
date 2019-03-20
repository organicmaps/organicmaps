#include "routing/routing_tests/routing_algorithm.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/maxspeeds.hpp"
#include "routing/routing_helpers.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

namespace routing
{
using namespace std;

namespace
{
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

  explicit RoadGraph(IRoadGraph const & roadGraph)
    : m_roadGraph(roadGraph), m_maxSpeedMPS(KMPH2MPS(roadGraph.GetMaxSpeedKMpH()))
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

      double const speedMPS = KMPH2MPS(
          m_roadGraph.GetSpeedKMpH(e, {true /* forward */, false /* in city */, Maxspeed()}));
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

      double const speedMPS = KMPH2MPS(
          m_roadGraph.GetSpeedKMpH(e, {true /* forward */, false /* in city */, Maxspeed()}));
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

TestAStarBidirectionalAlgo::Result Convert(AStarAlgorithm<RoadGraph>::Result value)
{
  switch (value)
  {
  case AStarAlgorithm<RoadGraph>::Result::OK: return TestAStarBidirectionalAlgo::Result::OK;
  case AStarAlgorithm<RoadGraph>::Result::NoPath: return TestAStarBidirectionalAlgo::Result::NoPath;
  case AStarAlgorithm<RoadGraph>::Result::Cancelled: return TestAStarBidirectionalAlgo::Result::Cancelled;
  }

  UNREACHABLE();
  return TestAStarBidirectionalAlgo::Result::NoPath;
}
}  // namespace

string DebugPrint(TestAStarBidirectionalAlgo::Result const & value)
{
  switch (value)
  {
  case TestAStarBidirectionalAlgo::Result::OK:
    return "OK";
  case TestAStarBidirectionalAlgo::Result::NoPath:
    return "NoPath";
  case TestAStarBidirectionalAlgo::Result::Cancelled:
    return "Cancelled";
  }

  UNREACHABLE();
  return string();
}

// *************************** AStar-bidirectional routing algorithm implementation ***********************
TestAStarBidirectionalAlgo::Result TestAStarBidirectionalAlgo::CalculateRoute(
    IRoadGraph const & graph, Junction const & startPos, Junction const & finalPos,
    RoutingResult<IRoadGraph::Vertex, IRoadGraph::Weight> & path)
{
  RoadGraph roadGraph(graph);
  base::Cancellable const cancellable;
  AStarAlgorithm<RoadGraph>::Params params(roadGraph, startPos, finalPos, {} /* prevRoute */,
                                           cancellable, {} /* onVisitJunctionFn */, {} /* checkLength */);
  AStarAlgorithm<RoadGraph>::Result const res = AStarAlgorithm<RoadGraph>().FindPathBidirectional(params, path);
  return Convert(res);
}
}  // namespace routing
