#include "testing/testing.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/base/routing_result.hpp"
#include "routing/index_router.hpp"
#include "routing/routing_helpers.hpp"

#include "traffic/traffic_cache.hpp"

#include "defines.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace routing
{
struct IndexRouterTest
{
  static RouterResultCode RefineLeaps(std::vector<Segment> const & input, IndexGraphStarter & starter,
                                      RouterDelegate const & delegate, RoutingResult<Segment, RouteWeight> & result)
  {
    auto progress = std::make_shared<AStarProgress>();
    IndexRouter::RoutesCalculator calculator(starter, delegate);
    return IndexRouter::ProcessLeapsJoints(input, starter, progress, calculator, result);
  }
};
}  // namespace routing

namespace index_router_tests
{
using namespace routing;
using namespace routing_test;

std::unique_ptr<SingleVehicleWorldGraph> BuildRepeatedMapLeapGraph()
{
  traffic::TrafficCache const trafficCache;
  auto estimator = CreateEstimatorForCar(trafficCache);
  auto indexLoader = std::make_unique<TestIndexGraphLoader>();
  auto const addMap =
      [&](NumMwmId mwmId, std::vector<std::pair<int, int>> const & intervals, std::vector<Joint> const & joints)
  {
    auto loader = std::make_unique<TestGeometryLoader>();
    for (uint32_t i = 0; i < intervals.size(); ++i)
    {
      auto const [from, to] = intervals[i];
      loader->AddRoad(i, false /* oneWay */, 50.0 /* speed */, {{0.001 * from, 0}, {0.001 * to, 0}});
      loader->SetPassThroughAllowed(i, true);
    }
    indexLoader->AddGraph(mwmId, routing_test::BuildIndexGraph(std::move(loader), estimator, joints));
  };
  addMap(0, {{0, 1}, {1, 2}, {4, 5}, {5, 6}, {2, 4}},
         {MakeJoint({{0, 0}}), MakeJoint({{0, 1}, {1, 0}}), MakeJoint({{1, 1}, {4, 0}}), MakeJoint({{4, 1}, {2, 0}}),
          MakeJoint({{2, 1}, {3, 0}}), MakeJoint({{3, 1}})});
  addMap(1, {{1, 2}, {2, 4}, {4, 5}, {5, 6}, {6, 7}},
         {MakeJoint({{0, 0}}), MakeJoint({{0, 1}, {1, 0}}), MakeJoint({{1, 1}, {2, 0}}), MakeJoint({{2, 1}, {3, 0}}),
          MakeJoint({{3, 1}, {4, 0}}), MakeJoint({{4, 1}})});
  auto ids = std::make_shared<NumMwmIds>();
  ids->RegisterFile(platform::CountryFile("Test_West"));
  ids->RegisterFile(platform::CountryFile("Test_East"));
  MwmHierarchyHandler hierarchy(ids, [](std::string const & name) { return name == "Test" ? COUNTRIES_ROOT : "Test"; });
  return std::make_unique<SingleVehicleWorldGraph>(nullptr /* crossMwmGraph */, std::move(indexLoader), estimator,
                                                   std::move(hierarchy));
}

UNIT_CLASS_TEST(RestrictionTest, LeapCompositionRepeatedMaps)
{
  Init(BuildRepeatedMapLeapGraph());
  auto const start = MakeFakeEnding({Segment(0, 0, 0, true)}, {0, 0}, *m_graph);
  auto const finish = MakeFakeEnding({Segment(1, 4, 0, true)}, {0.007, 0}, *m_graph);
  SetStarter(start, finish);
  m_starter->EnablePassThroughStateRouting(false);
  RouterDelegate delegate;
  RoutingResult<Segment, RouteWeight> result;
  std::vector<Segment> const direct = {m_starter->GetStartSegment(), Segment(0, 1, 0, true), Segment(1, 0, 0, true),
                                       m_starter->GetFinishSegment()};
  TEST_EQUAL(IndexRouterTest::RefineLeaps(direct, *m_starter, delegate, result), RouterResultCode::NoError, ());
  TEST(!result.Empty(), ());

  // Each twin pair shares geometry; every individual map piece is connected and has positive length.
  // Visiting both endpoint maps twice also produces an invalid loop-removal pair, (start=5, finish=2).
  std::vector<Segment> const repeated = {
      m_starter->GetStartSegment(), Segment(0, 1, 0, true), Segment(1, 0, 0, true), Segment(1, 2, 0, true),
      Segment(0, 2, 0, true),       Segment(0, 3, 0, true), Segment(1, 3, 0, true), m_starter->GetFinishSegment()};
  result.Clear();
  TEST_EQUAL(IndexRouterTest::RefineLeaps(repeated, *m_starter, delegate, result), RouterResultCode::NoError, ());
  TEST(!result.Empty(), ());
}
}  // namespace index_router_tests
