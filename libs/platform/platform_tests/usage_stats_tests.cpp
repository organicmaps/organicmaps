#include "testing/testing.hpp"

#include "platform/settings.hpp"

#include <chrono>
#include <string>
#include <string_view>
#include <thread>

namespace
{
constexpr std::string_view kFirstLaunchKey = "US_FirstLaunch";
constexpr std::string_view kLastBackgroundKey = "US_LastBackground";
constexpr std::string_view kTotalForegroundKey = "US_TotalForeground";
constexpr std::string_view kSessionsKey = "US_SessionsCount";

// Snapshots and restores on-disk UsageStats keys around a test; needed
// because the class writes through the process-wide StringStorage singleton.
class UsageStatsKeysGuard
{
public:
  UsageStatsKeysGuard()
  {
    auto & ss = settings::StringStorage::Instance();
    m_hadFirstLaunch = ss.GetValue(kFirstLaunchKey, m_savedFirstLaunch);
    m_hadLastBackground = ss.GetValue(kLastBackgroundKey, m_savedLastBackground);
    m_hadTotalForeground = ss.GetValue(kTotalForegroundKey, m_savedTotalForeground);
    m_hadSessions = ss.GetValue(kSessionsKey, m_savedSessions);
    ss.DeleteKeyAndValue(kFirstLaunchKey);
    ss.DeleteKeyAndValue(kLastBackgroundKey);
    ss.DeleteKeyAndValue(kTotalForegroundKey);
    ss.DeleteKeyAndValue(kSessionsKey);
  }

  ~UsageStatsKeysGuard()
  {
    auto & ss = settings::StringStorage::Instance();
    Restore(ss, kFirstLaunchKey, m_hadFirstLaunch, m_savedFirstLaunch);
    Restore(ss, kLastBackgroundKey, m_hadLastBackground, m_savedLastBackground);
    Restore(ss, kTotalForegroundKey, m_hadTotalForeground, m_savedTotalForeground);
    Restore(ss, kSessionsKey, m_hadSessions, m_savedSessions);
  }

private:
  static void Restore(settings::StringStorage & ss, std::string_view key, bool had, std::string const & val)
  {
    if (had)
      ss.SetValue(key, std::string(val));
    else
      ss.DeleteKeyAndValue(key);
  }

  std::string m_savedFirstLaunch, m_savedLastBackground, m_savedTotalForeground, m_savedSessions;
  bool m_hadFirstLaunch = false, m_hadLastBackground = false, m_hadTotalForeground = false, m_hadSessions = false;
};

uint64_t ReadStored(std::string_view key)
{
  std::string s;
  if (!settings::StringStorage::Instance().GetValue(key, s))
    return 0;
  uint64_t v = 0;
  settings::FromString(s, v);
  return v;
}

uint64_t ReadTotalForeground()
{
  return ReadStored(kTotalForegroundKey);
}

uint64_t ReadSessions()
{
  return ReadStored(kSessionsKey);
}
}  // namespace

// Android transient background: ActivityLifecycleCallbacks can fire two
// EnterBackground without an EnterForeground between (PL debounces away
// the intermediate onStop/onStart). The foreground window must not be
// aggregated twice.
UNIT_TEST(UsageStats_EnterBackgroundTwice_DoesNotDoubleCountForeground)
{
  UsageStatsKeysGuard guard;

  settings::UsageStats stats;
  stats.EnterForeground();

  std::this_thread::sleep_for(std::chrono::seconds(2));
  stats.EnterBackground();
  uint64_t const totalAfterFirst = ReadTotalForeground();
  TEST_GREATER_OR_EQUAL(totalAfterFirst, 1, ("First EnterBackground must aggregate >= 1 sec"));

  std::this_thread::sleep_for(std::chrono::seconds(2));
  stats.EnterBackground();
  uint64_t const totalAfterSecond = ReadTotalForeground();

  uint64_t const delta = totalAfterSecond - totalAfterFirst;
  // Without the fix, delta is ~4 sec (full window from the original foreground
  // start). With the fix it is the time since the previous EnterBackground.
  TEST_LESS_OR_EQUAL(
      delta, 3,
      ("Second EnterBackground re-aggregated the original foreground window", totalAfterFirst, totalAfterSecond));
}

// The session counter must increment only once per foreground period, even if
// EnterBackground is called multiple times due to Android transient
// backgrounding. EnterForeground re-arms the counter for the next real session.
UNIT_TEST(UsageStats_EnterBackgroundTwice_DoesNotDoubleCountSessions)
{
  UsageStatsKeysGuard guard;

  settings::UsageStats stats;
  stats.EnterForeground();

  std::this_thread::sleep_for(std::chrono::seconds(1));
  stats.EnterBackground();
  uint64_t const sessionsAfterFirst = ReadSessions();
  TEST_EQUAL(sessionsAfterFirst, 1, ("First EnterBackground must record one session"));

  // Repeat EnterBackground without an intervening EnterForeground (transient).
  std::this_thread::sleep_for(std::chrono::seconds(1));
  stats.EnterBackground();
  uint64_t const sessionsAfterRepeat = ReadSessions();
  TEST_EQUAL(sessionsAfterRepeat, 1,
             ("Repeat EnterBackground without EnterForeground must not increment sessions", sessionsAfterFirst,
              sessionsAfterRepeat));

  // A real return to foreground re-arms the session counter.
  stats.EnterForeground();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  stats.EnterBackground();
  uint64_t const sessionsAfterReentry = ReadSessions();
  TEST_EQUAL(sessionsAfterReentry, 2,
             ("EnterForeground must re-arm session counting", sessionsAfterRepeat, sessionsAfterReentry));
}
