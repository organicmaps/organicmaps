#include "testing/testing.hpp"

#include "editor/user_stats.hpp"

namespace editor
{
namespace
{
UNIT_TEST(UserStats_Smoke)
{
  // This user made only two changes and the possibility of further changes is very low.
  UserStats userStats("Vladimir BI");
  TEST(userStats.GetUpdateStatus(), ());
  TEST(userStats.IsChangesCountInitialized(), ());
  TEST(userStats.IsRankInitialized(), ());
  TEST_EQUAL(userStats.GetChangesCount(), 2, ());
  TEST_GREATER_OR_EQUAL(userStats.GetRank(), 5762, ());
}
}  // namespace
}  // namespace editor
