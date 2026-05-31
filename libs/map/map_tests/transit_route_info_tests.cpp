#include "testing/testing.hpp"

#include "map/transit/transit_display.hpp"

#include <string>
#include <vector>

namespace
{
uint32_t constexpr kRed = 0xFFFF0000;
uint32_t constexpr kBlue = 0xFF0000FF;

TransitStepInfo MakeTransitStep(std::string const & line, uint32_t color, std::string const & startStop,
                                std::string const & endStop)
{
  return TransitStepInfo(TransitType::Subway, 100.0 /* distance */, 60 /* time */, line, color,
                         0 /* intermediateIndex */, startStop, endStop);
}

TransitStepInfo MakePedestrianStep(double distance, int time)
{
  return TransitStepInfo(TransitType::Pedestrian, distance, time);
}

UNIT_TEST(TransitRouteInfo_AddStep_SingleEdge)
{
  TransitRouteInfo info;
  info.AddStep(MakeTransitStep("5", kRed, "A", "B"));

  TEST_EQUAL(info.m_steps.size(), 1, ());
  auto const & step = info.m_steps.front();
  TEST_EQUAL(step.m_startStopName, "A", ());
  TEST_EQUAL(step.m_endStopName, "B", ());
  TEST(step.m_intermediateStopNames.empty(), ());
  TEST_EQUAL(step.m_stopCount, 1, ());
}

UNIT_TEST(TransitRouteInfo_AddStep_MergesSameLine)
{
  TransitRouteInfo info;
  info.AddStep(MakeTransitStep("5", kRed, "A", "B"));
  info.AddStep(MakeTransitStep("5", kRed, "B", "C"));
  info.AddStep(MakeTransitStep("5", kRed, "C", "D"));

  TEST_EQUAL(info.m_steps.size(), 1, ());
  auto const & step = info.m_steps.front();
  TEST_EQUAL(step.m_startStopName, "A", ());
  TEST_EQUAL(step.m_endStopName, "D", ());
  TEST_EQUAL(step.m_intermediateStopNames, std::vector<std::string>({"B", "C"}), ());
  TEST_EQUAL(step.m_stopCount, 3, ());
  TEST_ALMOST_EQUAL_ABS(step.m_distanceInMeters, 300.0, 1e-9, ());
  TEST_EQUAL(step.m_timeInSec, 180, ());
}

UNIT_TEST(TransitRouteInfo_AddStep_DifferentLinesDoNotMerge)
{
  TransitRouteInfo info;
  info.AddStep(MakeTransitStep("5", kRed, "A", "B"));
  info.AddStep(MakeTransitStep("6", kBlue, "B", "C"));

  TEST_EQUAL(info.m_steps.size(), 2, ());
  TEST_EQUAL(info.m_steps[0].m_endStopName, "B", ());
  TEST_EQUAL(info.m_steps[1].m_startStopName, "B", ());
  TEST_EQUAL(info.m_steps[1].m_endStopName, "C", ());
  TEST_EQUAL(info.m_steps[1].m_stopCount, 1, ());
}

UNIT_TEST(TransitRouteInfo_AddStep_SameNumberDifferentColorDoNotMerge)
{
  TransitRouteInfo info;
  info.AddStep(MakeTransitStep("5", kRed, "A", "B"));
  info.AddStep(MakeTransitStep("5", kBlue, "B", "C"));

  TEST_EQUAL(info.m_steps.size(), 2, ());
}

UNIT_TEST(TransitRouteInfo_AddStep_PedestrianBreaksMerge)
{
  TransitRouteInfo info;
  info.AddStep(MakeTransitStep("5", kRed, "A", "B"));
  info.AddStep(MakePedestrianStep(50.0, 30));
  info.AddStep(MakeTransitStep("5", kRed, "C", "D"));

  TEST_EQUAL(info.m_steps.size(), 3, ());
  TEST_EQUAL(info.m_steps[0].m_endStopName, "B", ());
  TEST_EQUAL(info.m_steps[2].m_startStopName, "C", ());
  TEST_EQUAL(info.m_steps[2].m_endStopName, "D", ());
}

UNIT_TEST(TransitRouteInfo_AddStep_PedestrianAggregates)
{
  TransitRouteInfo info;
  info.AddStep(MakePedestrianStep(40.0, 20));
  info.AddStep(MakePedestrianStep(60.0, 30));

  TEST_EQUAL(info.m_steps.size(), 1, ());
  TEST_EQUAL(info.m_steps.front().m_stopCount, 0, ());
  TEST_ALMOST_EQUAL_ABS(info.m_totalPedestrianDistInMeters, 100.0, 1e-9, ());
  TEST_EQUAL(info.m_totalPedestrianTimeInSec, 50, ());
}

UNIT_TEST(TransitRouteInfo_AddStep_EmptyEndStopNameDoesNotCorruptLeg)
{
  TransitRouteInfo info;
  info.AddStep(MakeTransitStep("5", kRed, "A", "B"));
  info.AddStep(MakeTransitStep("5", kRed, "", ""));
  info.AddStep(MakeTransitStep("5", kRed, "C", "D"));

  TEST_EQUAL(info.m_steps.size(), 1, ());
  auto const & step = info.m_steps.front();
  TEST_EQUAL(step.m_startStopName, "A", ());
  TEST_EQUAL(step.m_endStopName, "D", ());
  TEST_EQUAL(step.m_intermediateStopNames, std::vector<std::string>({"B"}), ());
  TEST_EQUAL(step.m_stopCount, 3, ());
}
}  // namespace
