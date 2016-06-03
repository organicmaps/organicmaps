#include "testing/testing.hpp"

#include "editor/user_stats.hpp"

#include "platform/platform_tests_support/writable_dir_changer.hpp"

namespace editor
{
namespace
{
auto constexpr kEditorTestDir = "editor-tests";
auto constexpr kUserName = "Vladimir BI";  // TODO(mgsergio, Zverik): Make a test user account.

UNIT_TEST(UserStatsLoader_Smoke)
{
  WritableDirChanger wdc(kEditorTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);

  {
     UserStatsLoader statsLoader;
     TEST(!statsLoader.GetStats(kUserName), ());
  }

  {
    // This user made only two changes and the possibility of further changes is very low.
    UserStatsLoader statsLoader;

    statsLoader.Update(kUserName);
    auto const userStats = statsLoader.GetStats(kUserName);

    TEST(userStats, ());
    int32_t rank, changesCount;
    TEST(userStats.GetRank(rank), ());
    TEST(userStats.GetChangesCount(changesCount), ());
  }

  // This test checks if user stats info was stored in setting.
  // NOTE: there Update function is not called.
  {
    UserStatsLoader statsLoader;

    TEST_EQUAL(statsLoader.GetUserName(), kUserName, ());
    auto const userStats = statsLoader.GetStats(kUserName);

    TEST(userStats, ());
    int32_t rank, changesCount;
    TEST(userStats.GetRank(rank), ());
    TEST(userStats.GetChangesCount(changesCount), ());
  }
}
}  // namespace
}  // namespace editor
