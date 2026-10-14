#include "testing/testing.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/base/routing_result.hpp"
#include "routing/index_graph_starter_joints.hpp"
#include "routing/index_router.hpp"
#include "routing/routing_helpers.hpp"

#include "traffic/traffic_cache.hpp"

#include "defines.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <type_traits>
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

namespace pass_through_state_tests
{
using namespace routing;
using namespace routing_test;

using State = PassThroughState;

std::unique_ptr<SingleVehicleWorldGraph> BuildSequenceGraph(std::vector<bool> const & publicRoads)
{
  auto loader = std::make_unique<TestGeometryLoader>();
  std::vector<Joint> joints = {MakeJoint({{0, 0}})};
  for (uint32_t i = 0; i < publicRoads.size(); ++i)
  {
    loader->AddRoad(i, false /* oneWay */, 50.0 /* speed */, {{0.001 * i, 0}, {0.001 * (i + 1), 0}});
    loader->SetPassThroughAllowed(i, publicRoads[i]);
    if (i + 1 < publicRoads.size())
      joints.push_back(MakeJoint({{i, 1}, {i + 1, 0}}));
  }
  joints.push_back(MakeJoint({{static_cast<uint32_t>(publicRoads.size() - 1), 1}}));
  traffic::TrafficCache const trafficCache;
  return BuildWorldGraph(std::move(loader), CreateEstimatorForCar(trafficCache), joints);
}

int CountChargedChanges(std::vector<bool> const & publicRoads)
{
  int crossings = 0;
  for (size_t i = 1; i < publicRoads.size(); ++i)
    crossings += publicRoads[i - 1] != publicRoads[i];
  return std::max(0, crossings - !publicRoads.front() - !publicRoads.back());
}

UNIT_CLASS_TEST(RestrictionTest, NonPassThroughStateTransitions)
{
  Init(BuildSequenceGraph({true, false}));
  SetStarter(MakeFakeEnding(0, 0, {0, 0}, *m_graph), MakeFakeEnding(1, 0, {0.002, 0}, *m_graph));
  auto const road = [](bool isPublic) { return Segment(kTestNumMwmId, isPublic ? 0 : 1, 0, true); };

  m_starter->EnablePassThroughStateRouting();
  for (bool const isPublic : {false, true})
  {
    auto const segment = road(isPublic);
    auto const states = m_starter->GetPassThroughStates(segment);
    TEST_EQUAL(states.size(), isPublic ? 2 : 3, ());
    for (State const state : {State::InitialService, State::NoPaidExit, State::OnePaidExit})
    {
      bool const allowed = state != State::InitialService || !isPublic;
      TEST_EQUAL(std::find(states.begin(), states.end(), state) != states.end(), allowed, ());
    }
  }

  struct Transition
  {
    bool m_fromPublic;
    bool m_toPublic;
    State m_state;
    std::optional<State> m_expected;
  };
  Transition const cases[] = {
      {false, false, State::InitialService, State::InitialService},
      {false, true, State::InitialService, State::NoPaidExit},
      {false, false, State::NoPaidExit, State::NoPaidExit},
      {false, true, State::NoPaidExit, State::OnePaidExit},
      {false, false, State::OnePaidExit, State::OnePaidExit},
      {false, true, State::OnePaidExit, std::nullopt},
      {true, false, State::NoPaidExit, State::NoPaidExit},
      {true, true, State::NoPaidExit, State::NoPaidExit},
      {true, false, State::OnePaidExit, State::OnePaidExit},
      {true, true, State::OnePaidExit, State::OnePaidExit},
  };
  for (auto const & test : cases)
    TEST_EQUAL(m_starter->AdvancePassThroughState(test.m_state, road(test.m_fromPublic), road(test.m_toPublic)),
               test.m_expected, (test.m_fromPublic, test.m_toPublic, test.m_state));
}

UNIT_CLASS_TEST(RestrictionTest, NonPassThroughStateSequences)
{
  Init(BuildSequenceGraph({true, false}));
  SetStarter(MakeFakeEnding(0, 0, {0, 0}, *m_graph), MakeFakeEnding(1, 0, {0.002, 0}, *m_graph));
  m_starter->EnablePassThroughStateRouting();
  auto const road = [](bool isPublic) { return Segment(kTestNumMwmId, isPublic ? 0 : 1, 0, true); };

  // Exhaust every short road-category sequence against the crossing-minus-endpoints policy.
  for (size_t length = 1; length <= 9; ++length)
  {
    for (size_t mask = 0; mask < (size_t{1} << length); ++mask)
    {
      std::vector<bool> publicRoads(length);
      for (size_t i = 0; i < length; ++i)
        publicRoads[i] = (mask & (size_t{1} << i)) != 0;
      int const expected = CountChargedChanges(publicRoads);
      std::optional<State> state = publicRoads.front() ? State::NoPaidExit : State::InitialService;
      RouteWeight weight(static_cast<double>(length));
      for (size_t i = 1; i < length && state; ++i)
      {
        auto const next = m_starter->AdvancePassThroughState(*state, road(publicRoads[i - 1]), road(publicRoads[i]));
        if (next)
          weight += RouteWeight(0.0, IndexGraphStarter::GetPassThroughPenalty(*state, *next), 0, 0, 0.0);
        state = next;
      }
      TEST_EQUAL(state.has_value(), expected <= 2, (length, mask));
      if (!state)
        continue;
      TEST_EQUAL(weight.GetNumPassThroughChanges(), expected, (length, mask));
      TEST_EQUAL(weight.GetIntegratedWeight(), length + expected * RouteWeight::s_PassThroughPenaltyS, (length, mask));
    }
  }
}

UNIT_CLASS_TEST(RestrictionTest, NonPassThroughSequenceRouteWeights)
{
  size_t constexpr kRoadCount = 5;
  for (size_t mask = 0; mask < (size_t{1} << kRoadCount); ++mask)
  {
    std::vector<bool> publicRoads(kRoadCount);
    for (size_t i = 0; i < kRoadCount; ++i)
      publicRoads[i] = (mask & (size_t{1} << i)) != 0;
    Init(BuildSequenceGraph(publicRoads));
    auto const start = MakeFakeEnding(0, 0, {0, 0}, *m_graph);
    auto const finish = MakeFakeEnding(kRoadCount - 1, 0, {0.001 * kRoadCount, 0}, *m_graph);
    int const expected = CountChargedChanges(publicRoads);
    for (bool const reverse : {false, true})
    {
      SetStarter(reverse ? finish : start, reverse ? start : finish);
      m_starter->EnablePassThroughStateRouting();
      auto const check = [&](auto & graph, auto const & from, auto const & to)
      {
        using Graph = std::remove_reference_t<decltype(graph)>;
        using Algorithm = AStarAlgorithm<typename Graph::Vertex, typename Graph::Edge, typename Graph::Weight>;
        typename Algorithm::template ParamsForTests<AStarLengthChecker> params(graph, from, to,
                                                                               AStarLengthChecker(*m_starter));
        RoutingResult<typename Graph::Vertex, RouteWeight> result;
        auto const code = Algorithm{}.FindPathBidirectional(params, result);
        TEST_EQUAL(code, expected <= 2 ? Algorithm::Result::OK : Algorithm::Result::NoPath, (mask, reverse));
        if (code != Algorithm::Result::OK)
          return;
        TEST_EQUAL(result.m_distance.GetNumPassThroughChanges(), expected, (mask, reverse));
        TEST_ALMOST_EQUAL_ABS(result.m_distance.GetIntegratedWeight() - result.m_distance.GetWeight(),
                              static_cast<double>(expected * RouteWeight::s_PassThroughPenaltyS), 1e-8,
                              (mask, reverse));
      };
      check(*m_starter, m_starter->GetStartSegment(), m_starter->GetFinishSegment());
      m_graph->SetMode(WorldGraphMode::JointSingleMwm);
      IndexGraphStarterJoints<IndexGraphStarter> jointStarter(*m_starter, m_starter->GetStartSegment(),
                                                              m_starter->GetFinishSegment());
      check(jointStarter, jointStarter.GetStartJoint(), jointStarter.GetFinishJoint());
      m_graph->SetMode(WorldGraphMode::NoLeaps);
    }
  }
}

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
}  // namespace pass_through_state_tests
