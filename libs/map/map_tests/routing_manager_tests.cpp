#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_mark.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <vector>

namespace routing_manager_tests
{
namespace
{
RouteMarkData MakeRoutePoint(RouteMarkType type, size_t intermediateIndex, double coord)
{
  RouteMarkData data;
  data.m_pointType = type;
  data.m_intermediateIndex = intermediateIndex;
  data.m_position = m2::PointD(coord, coord);
  return data;
}

size_t GetIntermediatePointsCount(std::vector<RouteMarkData> const & points)
{
  return std::count_if(points.begin(), points.end(),
                       [](RouteMarkData const & d) { return d.m_pointType == RouteMarkType::Intermediate; });
}

void FillRouteToLimit(RoutingManager & routingManager)
{
  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 0.0)), ());
  for (size_t i = 0; i < RoutePointsLayout::kMaxIntermediatePointsCount; ++i)
  {
    TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, i, static_cast<double>(i + 1)),
                                      false /* reorderIntermediatePoints */),
         (i));
  }
  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 101.0)), ());
}

void TestSavedRouteOrder(bool myPositionStart, size_t passedStops, bool hasCurrentPosition,
                         size_t intermediateCount = 3)
{
  auto & platform = GetPlatform();
  auto const testSettingsDir = base::JoinPath(platform.WritableDir(), "routing_manager_save_load_test");
  platform::tests_support::ScopedDirCleanup settingsDir(testSettingsDir);
  // Initialize global settings before redirecting the saved route to its test directory.
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto const originalSettingsDir = platform.SettingsDir();
  platform.SetSettingsDir(testSettingsDir);
  SCOPE_GUARD(restoreSettingsDir, [&] { platform.SetSettingsDir(originalSettingsDir); });
  auto & routingManager = framework.GetRoutingManager();
  auto & bookmarks = framework.GetBookmarkManager();

  auto start = MakeRoutePoint(RouteMarkType::Start, 0, 1.0);
  start.m_isMyPosition = myPositionStart;
  bookmarks.MyPositionMark().SetUserPosition(start.m_position, true /* hasPosition */);
  TEST(routingManager.AddRoutePoint(std::move(start), false /* reorderIntermediatePoints */), ());
  for (size_t i = 0; i < intermediateCount; ++i)
  {
    auto point = MakeRoutePoint(RouteMarkType::Intermediate, i, static_cast<double>(i + 2));
    point.m_title = "Stop " + std::to_string(i);
    point.m_isPassed = i < passedStops;
    TEST(routingManager.AddRoutePoint(std::move(point), false /* reorderIntermediatePoints */), (i));
  }
  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 110.0),
                                    false /* reorderIntermediatePoints */),
       ());

  auto expected = routingManager.GetRoutePoints();
  expected.erase(expected.begin(), expected.begin() + passedStops);
  expected.front().m_pointType = RouteMarkType::Start;
  expected.front().m_intermediateIndex = 0;
  for (size_t i = 1; i + 1 < expected.size(); ++i)
    expected[i].m_intermediateIndex = i - 1;

  routingManager.SaveRoutePoints();
  routingManager.RemoveRoutePoints();
  m2::PointD const currentPosition(20.0, 20.0);
  bookmarks.MyPositionMark().SetUserPosition(currentPosition, hasCurrentPosition);
  bool const restoreMyPosition = myPositionStart && passedStops == 0;
  if (restoreMyPosition && hasCurrentPosition)
    expected.front().m_position = currentPosition;

  bool loaded = false;
  // Saving and loading run in order on the file thread. Failures can also call back from that thread.
  routingManager.LoadRoutePoints([&](bool success)
  {
    platform.RunTask(Platform::Thread::Gui, [&, success]
    {
      loaded = success;
      testing::StopEventLoop();
    });
  });
  testing::RunEventLoop();
  TEST(loaded, ());

  auto const actual = routingManager.GetRoutePoints();
  TEST_EQUAL(actual.size(), expected.size(), ());
  for (size_t i = 0; i < actual.size(); ++i)
  {
    TEST(actual[i].m_pointType == expected[i].m_pointType, (i));
    TEST_EQUAL(actual[i].m_intermediateIndex, expected[i].m_intermediateIndex, (i));
    TEST_EQUAL(actual[i].m_position, expected[i].m_position, (i));
    TEST_EQUAL(actual[i].m_title, expected[i].m_title, (i));
  }
  TEST_EQUAL(actual.front().m_isMyPosition, restoreMyPosition && hasCurrentPosition, ());
  TEST_EQUAL(actual.front().m_replaceWithMyPositionAfterRestart, restoreMyPosition && !hasCurrentPosition, ());
}
}  // namespace

UNIT_TEST(RoutingManager_SaveLoadRoutePointsPreservesOrder)
{
  TestSavedRouteOrder(false /* myPositionStart */, 0 /* passedStops */, false /* hasCurrentPosition */);
  TestSavedRouteOrder(false /* myPositionStart */, 0 /* passedStops */, false /* hasCurrentPosition */,
                      RoutePointsLayout::kMaxIntermediatePointsCount);
}

UNIT_TEST(RoutingManager_SaveLoadRoutePointsPreservesRemainingStops)
{
  TestSavedRouteOrder(true /* myPositionStart */, 1 /* passedStops */, true /* hasCurrentPosition */);
  TestSavedRouteOrder(true /* myPositionStart */, 3 /* passedStops */, true /* hasCurrentPosition */);
}

UNIT_TEST(RoutingManager_SaveLoadRoutePointsRestoresMyPosition)
{
  TestSavedRouteOrder(true /* myPositionStart */, 0 /* passedStops */, false /* hasCurrentPosition */);
  TestSavedRouteOrder(true /* myPositionStart */, 0 /* passedStops */, true /* hasCurrentPosition */);
}

UNIT_TEST(RoutingManager_ContinueRouteToPointAtLimitKeepsFinish)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();

  FillRouteToLimit(routingManager);

  auto const pointsBefore = routingManager.GetRoutePoints();
  TEST_EQUAL(pointsBefore.size(), RoutePointsLayout::kMaxRoutePointsCount, ());
  TEST_EQUAL(GetIntermediatePointsCount(pointsBefore), RoutePointsLayout::kMaxIntermediatePointsCount, ());
  TEST(pointsBefore.back().m_pointType == RouteMarkType::Finish, ());

  auto newFinish = MakeRoutePoint(RouteMarkType::Finish, 0, 102.0);
  TEST(!routingManager.ContinueRouteToPoint(std::move(newFinish)), ());

  auto const pointsAfter = routingManager.GetRoutePoints();
  TEST_EQUAL(pointsAfter.size(), pointsBefore.size(), ());
  TEST_EQUAL(GetIntermediatePointsCount(pointsAfter), RoutePointsLayout::kMaxIntermediatePointsCount, ());
  TEST(pointsAfter.back().m_pointType == RouteMarkType::Finish, ());
  TEST_EQUAL(pointsAfter.back().m_position.x, pointsBefore.back().m_position.x, ());
  TEST_EQUAL(pointsAfter.back().m_position.y, pointsBefore.back().m_position.y, ());
}

UNIT_TEST(RoutingManager_AddRoutePointAtLimitReportsFailure)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();

  FillRouteToLimit(routingManager);

  auto const pointsBefore = routingManager.GetRoutePoints();
  TEST_EQUAL(pointsBefore.size(), RoutePointsLayout::kMaxRoutePointsCount, ());
  TEST(!routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, 100, 102.0)), ());
  auto const pointsAfter = routingManager.GetRoutePoints();
  TEST_EQUAL(pointsAfter.size(), pointsBefore.size(), ());
  TEST_EQUAL(GetIntermediatePointsCount(pointsAfter), GetIntermediatePointsCount(pointsBefore), ());
  TEST_EQUAL(pointsAfter.front().m_position, pointsBefore.front().m_position, ());
  TEST_EQUAL(pointsAfter.back().m_position, pointsBefore.back().m_position, ());
}

// If route marks are wiped between IsRoutingActive() and ContinueRouteToPoint(),
// the latter must return false without mutating the (empty) layout.
UNIT_TEST(RoutingManager_ContinueRouteToPointWithoutFinishFailsCleanly)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();

  TEST_EQUAL(routingManager.GetRoutePointsCount(), 0, ());

  auto newFinish = MakeRoutePoint(RouteMarkType::Finish, 0, 1.0);
  TEST(!routingManager.ContinueRouteToPoint(std::move(newFinish)), ());
  TEST_EQUAL(routingManager.GetRoutePointsCount(), 0, ());
}

UNIT_TEST(RoutingManager_OptimizeRoutePointsMatchesIncrementalOptimization)
{
  std::vector<double> const intermediateCoordinates = {1.0, 2.0, 3.0};

  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  auto & routingManager = framework.GetRoutingManager();
  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 0.0)), ());
  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 10.0)), ());
  for (double const coordinate : intermediateCoordinates)
    TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, 0, coordinate)), (coordinate));
  auto const expectedPoints = routingManager.GetRoutePoints();

  routingManager.RemoveRoutePoints();
  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Start, 0, 0.0)), ());
  TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Finish, 0, 10.0)), ());
  for (size_t i = 0; i < intermediateCoordinates.size(); ++i)
  {
    TEST(routingManager.AddRoutePoint(MakeRoutePoint(RouteMarkType::Intermediate, i, intermediateCoordinates[i]),
                                      false /* reorderIntermediatePoints */),
         (i));
  }

  routingManager.OptimizeRoutePoints();
  auto const actualPoints = routingManager.GetRoutePoints();

  // CheckpointPredictor is tested separately; keep this integration fixture non-trivial so a no-op cannot pass.
  TEST_EQUAL(expectedPoints.size(), intermediateCoordinates.size() + 2, ());
  bool expectedOrderChanged = false;
  for (size_t i = 0; i < intermediateCoordinates.size(); ++i)
  {
    auto const coordinate = intermediateCoordinates[i];
    if (expectedPoints[i + 1].m_position != m2::PointD(coordinate, coordinate))
    {
      expectedOrderChanged = true;
      break;
    }
  }
  TEST(expectedOrderChanged, ());

  TEST_EQUAL(actualPoints.size(), expectedPoints.size(), ());
  for (size_t i = 0; i < actualPoints.size(); ++i)
  {
    TEST_EQUAL(static_cast<int>(actualPoints[i].m_pointType), static_cast<int>(expectedPoints[i].m_pointType), (i));
    TEST_EQUAL(actualPoints[i].m_intermediateIndex, expectedPoints[i].m_intermediateIndex, (i));
    TEST_EQUAL(actualPoints[i].m_position, expectedPoints[i].m_position, (i));
  }
}
}  // namespace routing_manager_tests
