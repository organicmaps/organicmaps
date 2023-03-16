#pragma once

#include "platform/string_storage_base.hpp"

#include "base/macros.hpp"

#include <string>

namespace settings
{
/// Metric or Imperial.
extern char const * kMeasurementUnits;

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
[[nodiscard]] bool Get(std::string const & key, Value & outValue)
{
  std::string strVal;
  return StringStorage::Instance().GetValue(key, strVal) && FromString(strVal, outValue);
}

template <class Value>
void TryGet(std::string const & key, Value & outValue)
{
    bool unused = Get(key, outValue);
    UNUSED_VALUE(unused);
}

/// Automatically saves setting to external file
template <class Value>
void Set(std::string const & key, Value const & value)
{
  StringStorage::Instance().SetValue(key, ToString(value));
}

/// Automatically saves settings to external file
inline void Update(std::map<std::string, std::string> const & settings)
{
  StringStorage::Instance().Update(settings);
}

inline void Delete(std::string const & key) { StringStorage::Instance().DeleteKeyAndValue(key); }
inline void Clear() { StringStorage::Instance().Clear(); }

class UsageStats
{
  static uint64_t TimeSinceEpoch();
  uint64_t m_enterForegroundTime = 0;
  uint64_t m_totalForegroundTime = 0;
  uint64_t m_sessionsCount = 0;

  std::string m_firstLaunch, m_lastBackground, m_totalForeground, m_sessions;

  StringStorage & m_ss;

public:
  UsageStats();

  void EnterForeground();
  void EnterBackground();
};

} // namespace settings

/*
namespace marketing
{
class Settings : public platform::StringStorageBase
{
public:
  template <class Value>
  static void Set(std::string const & key, Value const & value)
  {
    Instance().SetValue(key, settings::ToString(value));
  }

  template <class Value>
  [[nodiscard]] static bool Get(std::string const & key, Value & outValue)
  {
    std::string strVal;
    return Instance().GetValue(key, strVal) && settings::FromString(strVal, outValue);
  }

private:
  static Settings & Instance();
  Settings();
};
}  // namespace marketing
*/
