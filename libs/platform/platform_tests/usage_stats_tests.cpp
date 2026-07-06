#include "testing/testing.hpp"

#include "platform/settings.hpp"

#include <string>
#include <string_view>

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

// Smoke test: the default constructor must plumb a working real-time clock
// through the injection point. Guards against a broken delegation to
// TimeSinceEpoch (empty std::function would trigger a CHECK in the ctor).
UNIT_TEST(UsageStats_DefaultConstructor_UsesRealClock)
{
  UsageStatsKeysGuard guard;

  settings::UsageStats stats;
  stats.EnterForeground();
  stats.EnterBackground();
  TEST_EQUAL(ReadSessions(), 1, ("Default ctor must produce a functional UsageStats"));
}

// Android transient background: ActivityLifecycleCallbacks can fire two
// EnterBackground without an EnterForeground between (PL debounces away
// the intermediate onStop/onStart). The foreground window must not be
// aggregated twice.
UNIT_TEST(UsageStats_EnterBackgroundTwice_DoesNotDoubleCountForeground)
{
  UsageStatsKeysGuard guard;

  uint64_t fakeNow = 1000;
  settings::UsageStats stats([&fakeNow]() { return fakeNow; });

  stats.EnterForeground();

  fakeNow += 2;
  stats.EnterBackground();
  TEST_EQUAL(ReadTotalForeground(), 2, ("First EnterBackground aggregates the 2s foreground window"));

  fakeNow += 2;
  stats.EnterBackground();
  // Without the fix, the second EnterBackground would re-aggregate the whole
  // 4s window from the original EnterForeground; with the fix it adds only
  // the 2s delta since the previous EnterBackground.
  TEST_EQUAL(ReadTotalForeground(), 4, ("Second EnterBackground adds only the incremental 2s"));
}

// The session counter must increment only once per foreground period, even if
// EnterBackground is called multiple times due to Android transient
// backgrounding. EnterForeground re-arms the counter for the next real session.
UNIT_TEST(UsageStats_EnterBackgroundTwice_DoesNotDoubleCountSessions)
{
  UsageStatsKeysGuard guard;

  uint64_t fakeNow = 1000;
  settings::UsageStats stats([&fakeNow]() { return fakeNow; });

  stats.EnterForeground();

  fakeNow += 1;
  stats.EnterBackground();
  TEST_EQUAL(ReadSessions(), 1, ("First EnterBackground must record one session"));

  // Repeat EnterBackground without an intervening EnterForeground (transient).
  fakeNow += 1;
  stats.EnterBackground();
  TEST_EQUAL(ReadSessions(), 1, ("Repeat EnterBackground without EnterForeground must not increment sessions"));

  // A real return to foreground re-arms the session counter.
  stats.EnterForeground();
  fakeNow += 1;
  stats.EnterBackground();
  TEST_EQUAL(ReadSessions(), 2, ("EnterForeground must re-arm session counting"));
}
