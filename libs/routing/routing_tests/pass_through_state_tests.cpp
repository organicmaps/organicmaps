#include "testing/testing.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing/base/routing_result.hpp"
#include "routing/index_graph_starter_joints.hpp"
#include "routing/routing_helpers.hpp"

#include "traffic/traffic_cache.hpp"

#include <algorithm>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

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

UNIT_TEST(NonPassThroughStateTransitions)
{
  for (bool const isService : {false, true})
  {
    auto const states = IndexGraphStarter::GetPassThroughStates(isService);
    TEST_EQUAL(states.size(), isService ? 3 : 2, ());
    for (State const state : {State::InitialService, State::NoPaidExit, State::OnePaidExit})
    {
      bool const allowed = state != State::InitialService || isService;
      TEST_EQUAL(std::find(states.begin(), states.end(), state) != states.end(), allowed, ());
    }
  }

  struct Transition
  {
    bool m_exitsService;
    State m_state;
    std::optional<State> m_expected;
  };
  Transition const cases[] = {
      {false, State::InitialService, State::InitialService}, {true, State::InitialService, State::NoPaidExit},
      {false, State::NoPaidExit, State::NoPaidExit},         {true, State::NoPaidExit, State::OnePaidExit},
      {false, State::OnePaidExit, State::OnePaidExit},       {true, State::OnePaidExit, std::nullopt},
  };
  for (auto const & test : cases)
    TEST_EQUAL(IndexGraphStarter::AdvancePassThroughState(test.m_state, test.m_exitsService), test.m_expected,
               (test.m_exitsService, test.m_state));
}

UNIT_TEST(NonPassThroughStateSequences)
{
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
        bool const exitsService = !publicRoads[i - 1] && publicRoads[i];
        auto const next = IndexGraphStarter::AdvancePassThroughState(*state, exitsService);
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

}  // namespace pass_through_state_tests
