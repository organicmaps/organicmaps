#include "testing/testing.hpp"

#include "editor/user_stats.hpp"

#include "platform/platform_tests_support/writable_dir_changer.hpp"

namespace editor
{
namespace
{
auto constexpr kEditorTestDir = "editor-tests";
// This user has made only 9 changes and then renamed himself, so there will be no further edits from him.
auto constexpr kUserName = "Nikita Bokiy";

UNIT_TEST(UserStatsLoader_Smoke)
{
  WritableDirChanger wdc(kEditorTestDir, WritableDirChanger::SettingsDirPolicy::UseWritableDir);

  {
     UserStatsLoader statsLoader;
     TEST(!statsLoader.GetStats(kUserName), ());
  }

  {
    UserStatsLoader statsLoader;

    statsLoader.Update(kUserName);
    auto const userStats = statsLoader.GetStats(kUserName);

    TEST(userStats, ());
    int32_t rank, changesCount;
    TEST(userStats.GetRank(rank), ());
    TEST(userStats.GetChangesCount(changesCount), ());

    TEST_GREATER_OR_EQUAL(rank, 2100, ());
    TEST_EQUAL(changesCount, 9, ());
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

    TEST_GREATER_OR_EQUAL(rank, 2100, ());
    TEST_EQUAL(changesCount, 9, ());
  }
}
}  // namespace
}  // namespace editor
