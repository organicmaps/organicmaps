#include "testing/testing.hpp"

#include "storage/downloading_policy.hpp"

#include "platform/platform.hpp"

#include <chrono>

namespace downloading_policy_tests
{
namespace
{
class TestDownloadingPolicy : public StorageDownloadingPolicy
{
public:
  using StorageDownloadingPolicy::StorageDownloadingPolicy;
  bool IsDownloadingAllowed() override { return m_allowed; }

  bool m_allowed = true;
};
}  // namespace

UNIT_TEST(DownloadingPolicy_TerrainTerminalFailures)
{
  TestDownloadingPolicy policy;
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());

  // A non-retryable block must surface immediately, without spending the transport retry budget.
  policy.ScheduleTerrainRetry({}, {}, true /* hasNonRetryableFailures */);
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());
  TEST(!policy.IsAutoRetryDownloadFailed(), ());
  // Cancellation removes the failed block and resets the terrain error gate.
  policy.ScheduleTerrainRetry({}, {});
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());

  policy.m_allowed = false;
  policy.ScheduleTerrainRetry({"West"}, {});
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({}, {});
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());
}

UNIT_TEST(DownloadingPolicy_TerrainTerminalFailureDoesNotBlockOtherTransportRetries)
{
  TestDownloadingPolicy policy;
  policy.ScheduleTerrainRetry({}, {}, true /* hasNonRetryableFailures */);
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());
  TEST(!policy.IsTerrainTransportRetryFailed(), ());

  // Another region's 404 must not make this region's transport failure terminal.
  policy.ScheduleTerrainRetry({"Other"}, {}, true /* hasNonRetryableFailures */);
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());
  TEST(!policy.IsTerrainTransportRetryFailed(), ());

  policy.m_allowed = false;
  policy.ScheduleTerrainRetry({"Other"}, {}, true /* hasNonRetryableFailures */);
  TEST(policy.IsTerrainTransportRetryFailed(), ());
  policy.ScheduleTerrainRetry({}, {}, true /* hasNonRetryableFailures */);
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());
  TEST(!policy.IsTerrainTransportRetryFailed(), ());
  policy.ScheduleTerrainRetry({}, {});
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());
  TEST(!policy.IsTerrainTransportRetryFailed(), ());
}

UNIT_TEST(DownloadingPolicy_TerrainRetryExhaustion)
{
  Platform::ThreadRunner runner;
  TestDownloadingPolicy policy(std::chrono::milliseconds(1));
  size_t attempts = 0;
  auto const retry = [&](storage::CountriesSet const &)
  {
    ++attempts;
    testing::StopEventLoop();
  };
  for (size_t attempt = 0; attempt < 3; ++attempt)
  {
    TEST(!policy.IsTerrainAutoRetryDownloadFailed(), (attempt));
    TEST(!policy.IsAutoRetryDownloadFailed(), (attempt));
    policy.ScheduleTerrainRetry({"West"}, retry);
    testing::RunEventLoop();
  }
  TEST_EQUAL(attempts, 3, ());
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());
  TEST(policy.IsTerrainTransportRetryFailed(), ());
  TEST(!policy.IsAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({"West"}, retry);
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());

  // Finishing the maps' batch must not hide an exhausted terrain failure.
  policy.ScheduleRetry({}, {});
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({}, {});
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());

  policy.ScheduleTerrainRetry({"West"}, retry);
  testing::RunEventLoop();
  TEST_EQUAL(attempts, 4, ());
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({}, {});
}

UNIT_TEST(DownloadingPolicy_RetryFailuresStayInTheirFamily)
{
  TestDownloadingPolicy policy;
  // A terminal terrain error must not turn a later map failure into a terminal error.
  policy.ScheduleTerrainRetry({}, {}, true /* hasNonRetryableFailures */);
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());
  TEST(!policy.IsAutoRetryDownloadFailed(), ());
  policy.ScheduleRetry({}, {});
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());
  TEST(!policy.IsAutoRetryDownloadFailed(), ());

  // Blocking a map retry affects only maps; cancelling terrain must preserve that map error.
  policy.ScheduleTerrainRetry({}, {});
  policy.m_allowed = false;
  policy.ScheduleRetry({"Map"}, {});
  TEST(policy.IsAutoRetryDownloadFailed(), ());
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({"Terrain"}, {});
  TEST(policy.IsAutoRetryDownloadFailed(), ());
  TEST(policy.IsTerrainAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({}, {});
  TEST(policy.IsAutoRetryDownloadFailed(), ());
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());
}

UNIT_TEST(DownloadingPolicy_MapRetryExhaustionDoesNotAffectTerrain)
{
  Platform::ThreadRunner runner;
  TestDownloadingPolicy policy(std::chrono::milliseconds(1));
  size_t attempts = 0;
  auto const retry = [&](storage::CountriesSet const &)
  {
    ++attempts;
    testing::StopEventLoop();
  };
  for (size_t attempt = 0; attempt < 3; ++attempt)
  {
    TEST(!policy.IsAutoRetryDownloadFailed(), (attempt));
    policy.ScheduleRetry({"Map"}, retry);
    testing::RunEventLoop();
  }
  TEST_EQUAL(attempts, 3, ());
  TEST(policy.IsAutoRetryDownloadFailed(), ());
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({}, {});
  TEST(policy.IsAutoRetryDownloadFailed(), ());
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());
  policy.ScheduleRetry({}, {});
  TEST(!policy.IsAutoRetryDownloadFailed(), ());
  TEST(!policy.IsTerrainAutoRetryDownloadFailed(), ());
}
}  // namespace downloading_policy_tests
