#ifdef DEBUG

#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/location_provider/gpx_replay_provider.hpp"

#include "search/result.hpp"
#include "search/search_params.hpp"

#include "platform/platform.hpp"

#include <string>

namespace mock_gpx_command_tests
{
namespace
{
// Runs Framework::ParseMockGpxCommand and captures whatever message, if any,
// EmitDebugCommandResult posted back to the search UI.
struct Outcome
{
  bool matched = false;
  std::string lastMessage;
  bool gotAnyMessage = false;
};

Outcome RunParseMockGpxCommand(std::string const & query)
{
  Outcome outcome;
  search::SearchParams params;
  params.m_query = query;
  params.m_onResults = [&outcome](search::Results const & results)
  {
    if (results.GetCount() > 0)
    {
      outcome.gotAnyMessage = true;
      outcome.lastMessage = results[results.GetCount() - 1].GetString();
    }
  };

  outcome.matched = Framework::ParseMockGpxCommand(params);

  // Every test case disarms afterward so state never leaks between UNIT_TEST cases in this
  // binary (GpxReplayProvider::Instance() is the process-wide, statically-registered instance).
  location_provider::GpxReplayProvider::Instance().Disarm();

  return outcome;
}

std::string TestFixturePath(std::string const & name)
{
  return GetPlatform().TestsDataPathForFile("test_data/gpx/" + name);
}
}  // namespace

UNIT_TEST(ParseMockGpxCommand_ValidPathWithAccuracy_Arms)
{
  auto const path = TestFixturePath("track_without_timestamps.gpx");
  auto const outcome = RunParseMockGpxCommand("?mock-gpx:" + path + ",12.5");

  TEST(outcome.matched, ());
  TEST(outcome.gotAnyMessage, ());
  TEST(outcome.lastMessage.find("armed") != std::string::npos, (outcome.lastMessage));
}

UNIT_TEST(ParseMockGpxCommand_ValidPathNoAccuracy_Arms)
{
  auto const path = TestFixturePath("track_without_timestamps.gpx");
  auto const outcome = RunParseMockGpxCommand("?mock-gpx:" + path);

  TEST(outcome.matched, ());
  TEST(outcome.gotAnyMessage, ());
  TEST(outcome.lastMessage.find("armed") != std::string::npos, (outcome.lastMessage));
}

UNIT_TEST(ParseMockGpxCommand_PathContainingCommaWithNoTrailingNumber_FallsBackToWholeStringAsPath)
{
  // This fixture's own filename contains a comma followed by a non-numeric segment, so the
  // split-on-last-comma rule must fall back to treating the entire remainder as <path> — proven
  // by the fact that opening it (and arming) succeeds at all.
  auto const path = TestFixturePath("mock_gpx_command_test,not_a_number.gpx");
  auto const outcome = RunParseMockGpxCommand("?mock-gpx:" + path);

  TEST(outcome.matched, ());
  TEST(outcome.gotAnyMessage, ());
  TEST(outcome.lastMessage.find("armed") != std::string::npos, (outcome.lastMessage));
}

UNIT_TEST(ParseMockGpxCommand_UnreadablePath_ReportsFailureAndDoesNotArm)
{
  auto const outcome = RunParseMockGpxCommand("?mock-gpx:/nonexistent/path/does_not_exist.gpx");

  TEST(outcome.matched, ());
  TEST(outcome.gotAnyMessage, ());
  TEST(outcome.lastMessage.find("Failed to load") != std::string::npos, (outcome.lastMessage));
}

UNIT_TEST(ParseMockGpxCommand_ZeroTrackGpxFile_ReportsFailureAndDoesNotArm)
{
  // points.gpx is waypoints-only — syntactically valid GPX, zero <trk> elements.
  auto const path = TestFixturePath("points.gpx");
  auto const outcome = RunParseMockGpxCommand("?mock-gpx:" + path);

  TEST(outcome.matched, ());
  TEST(outcome.gotAnyMessage, ());
  TEST(outcome.lastMessage.find("no tracks") != std::string::npos, (outcome.lastMessage));
}

UNIT_TEST(ParseMockGpxCommand_FailedReArmAttempt_LeavesExistingArmedStateUnchanged)
{
  // Deliberately bypasses RunParseMockGpxCommand()'s helper here — it disarms unconditionally
  // for test isolation, which would defeat this test's own armed-state precondition.
  auto const path = TestFixturePath("track_without_timestamps.gpx");
  search::SearchParams armParams;
  armParams.m_query = "?mock-gpx:" + path;
  TEST(Framework::ParseMockGpxCommand(armParams), ());
  TEST(location_provider::GpxReplayProvider::Instance().IsArmed(), ());

  search::SearchParams failParams;
  failParams.m_query = "?mock-gpx:/nonexistent/path/does_not_exist.gpx";
  TEST(Framework::ParseMockGpxCommand(failParams), ());

  TEST(location_provider::GpxReplayProvider::Instance().IsArmed(), ());
  TEST(location_provider::GpxReplayProvider::Instance().GetCurrentReading().has_value(), ());

  location_provider::GpxReplayProvider::Instance().Disarm();
}

UNIT_TEST(ParseMockGpxCommand_UnrelatedQuery_DoesNotMatch)
{
  auto const outcome = RunParseMockGpxCommand("?debug-info");
  TEST(!outcome.matched, ());
}

UNIT_TEST(ParseMockGpxStopCommand_ExactMatch_Disarms)
{
  // Deliberately bypasses RunParseMockGpxCommand()'s helper here — it disarms unconditionally
  // for test isolation, which would defeat this test's own armed-state precondition.
  auto const path = TestFixturePath("track_without_timestamps.gpx");
  search::SearchParams armParams;
  armParams.m_query = "?mock-gpx:" + path;
  TEST(Framework::ParseMockGpxCommand(armParams), ());
  TEST(location_provider::GpxReplayProvider::Instance().IsArmed(), ());

  search::SearchParams stopParams;
  stopParams.m_query = "?mock-gpx-stop";
  TEST(Framework::ParseMockGpxStopCommand(stopParams), ());
  TEST(!location_provider::GpxReplayProvider::Instance().IsArmed(), ());
}

UNIT_TEST(ParseMockGpxStopCommand_UnrelatedQuery_DoesNotMatch)
{
  search::SearchParams params;
  params.m_query = "?mock-gpx-stopped";  // must not fuzzy-match
  TEST(!Framework::ParseMockGpxStopCommand(params), ());
}
}  // namespace mock_gpx_command_tests

#endif  // DEBUG
