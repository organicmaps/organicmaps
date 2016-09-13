#include "testing/testing.hpp"

#include "routing_benchmarks/helpers.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/bicycle_directions.hpp"
#include "routing/bicycle_model.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/road_graph.hpp"
#include "routing/road_graph_router.hpp"

#include "std/set.hpp"
#include "std/string.hpp"

namespace
{
// Test preconditions: files from the kMapFiles set with '.mwm'
// extension must be placed in ./omim/data folder.
set<string> const kMapFiles = {"Russia_Moscow"};

unique_ptr<routing::IVehicleModelFactory> MakeFactory()
{
  unique_ptr<routing::IVehicleModelFactory> factory(
      new SimplifiedModelFactory<routing::BicycleModel>());
  return factory;
}

class BicycleTest : public RoutingTest
{
public:
  BicycleTest() : RoutingTest(kMapFiles) {}

  // RoutingTest overrides:
  unique_ptr<routing::IRouter> CreateAStarRouter() override
  {
    auto getter = [&](m2::PointD const & pt) { return m_cig->GetRegionCountryId(pt); };
    unique_ptr<routing::IRoutingAlgorithm> algorithm(new routing::AStarRoutingAlgorithm());
    unique_ptr<routing::IDirectionsEngine> engine(new routing::BicycleDirectionsEngine(m_index));
    unique_ptr<routing::IRouter> router(new routing::RoadGraphRouter(
        "test-astar-bicycle", m_index, getter, routing::IRoadGraph::Mode::ObeyOnewayTag,
        MakeFactory(), move(algorithm), move(engine)));
    return router;
  }

  unique_ptr<routing::IRouter> CreateAStarBidirectionalRouter() override
  {
    auto getter = [&](m2::PointD const & pt) { return m_cig->GetRegionCountryId(pt); };
    unique_ptr<routing::IRoutingAlgorithm> algorithm(
        new routing::AStarBidirectionalRoutingAlgorithm());
    unique_ptr<routing::IDirectionsEngine> engine(new routing::BicycleDirectionsEngine(m_index));
    unique_ptr<routing::IRouter> router(new routing::RoadGraphRouter(
        "test-astar-bidirectional-bicycle", m_index, getter,
        routing::IRoadGraph::Mode::ObeyOnewayTag, MakeFactory(), move(algorithm), move(engine)));
    return router;
  }

  void GetNearestEdges(m2::PointD const & pt,
                       vector<pair<routing::Edge, routing::Junction>> & edges) override
  {
    routing::FeaturesRoadGraph graph(m_index, routing::IRoadGraph::Mode::ObeyOnewayTag,
                                     MakeFactory());
    graph.FindClosestEdges(pt, 1 /*count*/, edges);
  }
};

UNIT_CLASS_TEST(BicycleTest, Smoke)
{
  m2::PointD const start(37.565132927600473067, 67.527119454223551998);
  m2::PointD const final(37.560540032092660567, 67.530453499633722458);
  TestRouters(start, final);
}
}  // namespace
