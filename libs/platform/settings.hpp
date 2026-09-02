#pragma once

#include "platform/string_storage_base.hpp"

#include "base/macros.hpp"

#include <functional>
#include <string>

namespace settings
{
/// Metric or Imperial.
extern std::string_view kMeasurementUnits;
extern std::string_view kDeveloperMode;
extern std::string_view kMapLanguageCode;
// The following two settings are configured externally at the metaserver.
extern std::string_view kDonateUrl;
extern std::string_view kNY;

template <class T>
bool FromString(std::string const & str, T & outValue);

template <class T>
std::string ToString(T const & value);

class StringStorage : public platform::StringStorageBase
{
public:
  static StringStorage & Instance();

private:
  StringStorage();
};

/// Retrieve setting
/// @return false if setting is absent
template <class Value>
[[nodiscard]] bool Get(std::string_view key, Value & outValue)
{
  std::string strVal;
  return StringStorage::Instance().GetValue(key, strVal) && FromString(strVal, outValue);
}

inline bool IsEnabled(std::string_view key)
{
  bool val;
  return Get(key, val) && val;
}

template <class Value>
void TryGet(std::string_view key, Value & outValue)
{
  bool unused = Get(key, outValue);
  UNUSED_VALUE(unused);
}

/// Automatically saves setting to external file
template <class Value>
void Set(std::string_view key, Value const & value)
{
  StringStorage::Instance().SetValue(key, ToString(value));
}

/// Automatically saves settings to external file
inline void Update(std::map<std::string, std::string> const & settings)
{
  StringStorage::Instance().Update(settings);
}

inline void Delete(std::string_view key)
{
  StringStorage::Instance().DeleteKeyAndValue(key);
}
inline void Clear()
{
  StringStorage::Instance().Clear();
}

class UsageStats
{
  static uint64_t TimeSinceEpoch();

  std::function<uint64_t()> m_now;
  // Anchor for foreground-duration aggregation. Advanced at the end of
  // EnterBackground so a second EnterBackground without an intervening
  // EnterForeground (Android transient background) doesn't double-count time.
  uint64_t m_enterForegroundTime = 0;
  uint64_t m_totalForegroundTime = 0;
  uint64_t m_sessionsCount = 0;
  // True once the current foreground period has been counted as a session;
  // reset by EnterForeground. Guards against incrementing m_sessionsCount
  // multiple times on Android transient backgrounding.
  bool m_committedThisSession = false;

  std::string_view m_firstLaunch, m_lastBackground, m_totalForeground, m_sessions;

  StringStorage & m_ss;

public:
  UsageStats();
  // Testing seam: inject a fake clock so tests are deterministic and instant.
  explicit UsageStats(std::function<uint64_t()> nowFn);

  void EnterForeground();
  void EnterBackground();

  bool IsLoyalUser() const;
};

}  // namespace settings
