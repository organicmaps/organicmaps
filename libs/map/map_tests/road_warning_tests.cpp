#include "testing/testing.hpp"

#include "map/routing_mark.hpp"

#include "routing/router.hpp"
#include "routing/routing_options.hpp"

#include <initializer_list>

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
}  // namespace road_warning_tests
