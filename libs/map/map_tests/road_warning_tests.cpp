#include "testing/testing.hpp"

#include "map/routing_manager.hpp"
#include "map/routing_mark.hpp"

#include "routing/fake_feature_ids.hpp"
#include "routing/router.hpp"
#include "routing/routing_options.hpp"

#include "indexer/data_source.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include <initializer_list>
#include <optional>

namespace road_warning_tests
{
namespace
{
using routing::RouterType;
using routing::RoutingOptions;
using Road = RoutingOptions::Road;

RoutingOptions MakeOptions(std::initializer_list<Road> const roads)
{
  RoutingOptions options;
  for (auto const r : roads)
    options.Add(r);
  return options;
}

class TestRoute : public routing::RouteBase
{
public:
  TestRoute(std::vector<routing::RouteSegment> segments, bool remaining)
  {
    SetRouteSegments(std::move(segments));
    size_t const begin = remaining ? 2 : 0;
    geometry::PointWithAltitude const origin({}, 0);
    auto const start = begin == 0 ? origin : m_routeSegments[begin - 1].GetJunction();
    if (remaining)
      m_subrouteAttrs.emplace_back(origin, start, 0, begin);
    m_subrouteAttrs.emplace_back(start, m_routeSegments.back().GetJunction(), begin, m_routeSegments.size());
    SetCurrentSubrouteIdx(remaining ? 1 : 0);
  }
};
}  // namespace

UNIT_TEST(RoadWarning_IsWarningShownFor)
{
  using enum RoadWarningMarkType;

  // Toll / Dirty / lift gate: car only.
  for (auto const t : {Toll, Dirty, LiftGate})
  {
    TEST(IsWarningShownFor(t, RouterType::Vehicle), (t));
    TEST(!IsWarningShownFor(t, RouterType::Pedestrian), (t));
    TEST(!IsWarningShownFor(t, RouterType::Bicycle), (t));
  }

  // Steps: pedestrian / bicycle only.
  TEST(IsWarningShownFor(Steps, RouterType::Pedestrian), ());
  TEST(IsWarningShownFor(Steps, RouterType::Bicycle), ());
  TEST(!IsWarningShownFor(Steps, RouterType::Vehicle), ());

  // Ferry / gate: car, pedestrian and bicycle.
  for (auto const r : {RouterType::Vehicle, RouterType::Pedestrian, RouterType::Bicycle})
  {
    TEST(IsWarningShownFor(Ferry, r), (r));
    TEST(IsWarningShownFor(Gate, r), (r));
  }

  // Transit / ruler: no warnings at all.
  for (auto const r : {RouterType::Transit, RouterType::Ruler})
    for (auto const t : {Toll, Ferry, Dirty, Steps, Gate, LiftGate})
      TEST(!IsWarningShownFor(t, r), (t, r));
}

UNIT_TEST(RoadWarning_ChooseRoadWarning)
{
  using enum RoadWarningMarkType;

  // No road options -> nothing to warn about.
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({}), RouterType::Vehicle), Count, ());
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Motorway, Road::Usual}), RouterType::Vehicle), Count, ());

  // Car priority: Toll > Ferry > Dirty.
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Toll, Road::Ferry, Road::Dirty}), RouterType::Vehicle), Toll, ());
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Ferry, Road::Dirty}), RouterType::Vehicle), Ferry, ());
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Dirty}), RouterType::Vehicle), Dirty, ());

  // Regression: an unpaved staircase carries {Dirty, Steps} (the generator tags steps having a
  // surface/smoothness with an unpaved psurface). Filtering by router type first means a pedestrian/
  // cyclist still gets the Steps warning instead of it being dropped because Dirty is checked first.
  auto const unpavedSteps = MakeOptions({Road::Dirty, Road::Steps});
  TEST_EQUAL(ChooseRoadWarning(unpavedSteps, RouterType::Pedestrian), Steps, ());
  TEST_EQUAL(ChooseRoadWarning(unpavedSteps, RouterType::Bicycle), Steps, ());
  // Steps are not shown to cars; only Dirty remains there (steps don't occur on car routes anyway).
  TEST_EQUAL(ChooseRoadWarning(unpavedSteps, RouterType::Vehicle), Dirty, ());

  // Bare steps: shown to pedestrian/bicycle, nothing for car.
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Steps}), RouterType::Pedestrian), Steps, ());
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Steps}), RouterType::Bicycle), Steps, ());
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Steps}), RouterType::Vehicle), Count, ());

  // Toll / Dirty are car-only.
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Toll}), RouterType::Pedestrian), Count, ());
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Dirty}), RouterType::Pedestrian), Count, ());

  // Ferry is shown to everyone.
  for (auto const r : {RouterType::Vehicle, RouterType::Pedestrian, RouterType::Bicycle})
    TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Ferry}), r), Ferry, (r));

  // Transit / ruler never warn.
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Ferry, Road::Steps}), RouterType::Transit), Count, ());
  TEST_EQUAL(ChooseRoadWarning(MakeOptions({Road::Dirty}), RouterType::Ruler), Count, ());
}

UNIT_TEST(RoadWarning_IsAvoidableRoadWarning)
{
  using enum RoadWarningMarkType;

  // Backed by a car driving-option toggle.
  TEST(IsAvoidableRoadWarning(Toll), ());
  TEST(IsAvoidableRoadWarning(Ferry), ());
  TEST(IsAvoidableRoadWarning(Dirty), ());

  // No avoid option -> must not surface the "driving options" affordance.
  TEST(!IsAvoidableRoadWarning(Steps), ());
  TEST(!IsAvoidableRoadWarning(Gate), ());
  TEST(!IsAvoidableRoadWarning(LiftGate), ());
}

UNIT_TEST(RoutingManager_RoadWarningBoundaries)
{
  class Delegate final : public RoutingManager::Delegate
  {
    void OnRouteFollow(routing::RouterType) override {}
  } delegate;
  FrozenDataSource dataSource;
  // Join the extrapolator's background tasks before destroying the manager.
  std::optional<RoutingManager> managerHolder;
  Platform::ThreadRunner const runner;
  auto & manager = managerHolder.emplace(RoutingManager::Callbacks([&dataSource]() -> DataSource &
  { return dataSource; }, nullptr, nullptr, nullptr, nullptr),
                                         delegate);
  manager.m_numMwmIDs = std::make_shared<routing::NumMwmIds>();
  manager.m_numMwmIDs->RegisterFile(platform::CountryFile("RoadWarningTest"));
  manager.m_currentRouterType = routing::RouterType::Vehicle;

  std::vector<routing::RouteSegment> segments;
  for (size_t i = 0; i < 8; ++i)
  {
    m2::PointD const point(0.01 * i, 0);
    auto const featureId = i == 0 ? routing::FakeFeatureIds::kIndexGraphStarterId : 0;
    segments.emplace_back(routing::Segment(0, featureId, i, true), routing::turns::TurnItem(),
                          geometry::PointWithAltitude(point, 0), routing::RouteSegment::RoadNameInfo());
    segments.back().SetDistancesAndTime(mercator::DistanceOnEarth({}, point), point.x, 100.0 * i);
  }
  segments[2].SetRoadTypes(MakeOptions({Road::Toll}));
  segments[3].SetRoadTypes(MakeOptions({Road::Ferry}));
  segments[4].SetRoadTypes(MakeOptions({Road::Ferry}));
  segments[6].SetRoadTypes(MakeOptions({Road::Toll}));
  segments[7].SetRoadTypes(MakeOptions({Road::Toll}));

  for (bool const remaining : {false, true})
  {
    TestRoute const route(segments, remaining);
    auto const index = route.GetCurrentSubrouteIdx();
    std::vector<routing::RouteSegment> subroute;
    route.GetSubrouteInfo(index, subroute);
    RoutingManager::RoadWarningsCollection warnings;
    manager.CollectRoadWarnings(route, index, subroute, warnings);
    TEST_EQUAL(warnings.size(), 2, (remaining));
    auto const & tolls = warnings.at(RoadWarningMarkType::Toll);
    TEST_EQUAL(tolls.size(), 2, (remaining));
    TEST_EQUAL(tolls[0].m_startPoint, segments[1].GetJunction().GetPoint(), (remaining));
    TEST_ALMOST_EQUAL_ABS(
        tolls[0].m_distance,
        mercator::DistanceOnEarth(segments[1].GetJunction().GetPoint(), segments[2].GetJunction().GetPoint()), 1e-8,
        (remaining));
    TEST_EQUAL(tolls[1].m_startPoint, segments[5].GetJunction().GetPoint(), (remaining));
    TEST_ALMOST_EQUAL_ABS(
        tolls[1].m_distance,
        mercator::DistanceOnEarth(segments[5].GetJunction().GetPoint(), segments[7].GetJunction().GetPoint()), 1e-8,
        (remaining));
    auto const & ferries = warnings.at(RoadWarningMarkType::Ferry);
    TEST_EQUAL(ferries.size(), 1, (remaining));
    TEST_EQUAL(ferries[0].m_startPoint, segments[2].GetJunction().GetPoint(), (remaining));
    TEST_ALMOST_EQUAL_ABS(
        ferries[0].m_distance,
        mercator::DistanceOnEarth(segments[2].GetJunction().GetPoint(), segments[4].GetJunction().GetPoint()), 1e-8,
        (remaining));
  }
}
}  // namespace road_warning_tests
