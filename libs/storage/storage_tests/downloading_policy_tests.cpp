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
  TEST(!policy.IsAutoRetryDownloadFailed(), ());

  // A non-retryable block must surface immediately, without spending the transport retry budget.
  policy.ScheduleTerrainRetry({}, {}, true /* hasNonRetryableFailures */);
  TEST(policy.IsAutoRetryDownloadFailed(), ());
  // Cancellation removes the failed block and resets the terrain error gate.
  policy.ScheduleTerrainRetry({}, {});
  TEST(!policy.IsAutoRetryDownloadFailed(), ());

  policy.m_allowed = false;
  policy.ScheduleTerrainRetry({"West"}, {});
  TEST(policy.IsAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({}, {});
  TEST(!policy.IsAutoRetryDownloadFailed(), ());
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
    TEST(!policy.IsAutoRetryDownloadFailed(), (attempt));
    policy.ScheduleTerrainRetry({"West"}, retry);
    testing::RunEventLoop();
  }
  TEST_EQUAL(attempts, 3, ());
  TEST(policy.IsAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({"West"}, retry);
  TEST(policy.IsAutoRetryDownloadFailed(), ());

  // Finishing the maps' batch must not hide an exhausted terrain failure.
  policy.ScheduleRetry({}, {});
  TEST(policy.IsAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({}, {});
  TEST(!policy.IsAutoRetryDownloadFailed(), ());

  policy.ScheduleTerrainRetry({"West"}, retry);
  testing::RunEventLoop();
  TEST_EQUAL(attempts, 4, ());
  TEST(!policy.IsAutoRetryDownloadFailed(), ());
  policy.ScheduleTerrainRetry({}, {});
}
}  // namespace downloading_policy_tests
