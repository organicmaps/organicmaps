#pragma once

#include "platform/string_storage_base.hpp"

#include "base/macros.hpp"

#include "std/string.hpp"

namespace settings
{
/// Current location state mode. @See location::EMyPositionMode.
extern char const * kLocationStateMode;
/// Metric or Feet.
extern char const * kMeasurementUnits;

template <class T>
bool FromString(string const & str, T & outValue);
template <class T>
string ToString(T const & value);

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
WARN_UNUSED_RESULT bool Get(string const & key, Value & outValue)
{
  string strVal;
  return StringStorage::Instance().GetValue(key, strVal) && FromString(strVal, outValue);
}

template <class Value>
void TryGet(string const & key, Value & outValue)
{
    bool unused = Get(key, outValue);
    UNUSED_VALUE(unused);
}

/// Automatically saves setting to external file
template <class Value>
void Set(string const & key, Value const & value)
{
  StringStorage::Instance().SetValue(key, ToString(value));
}

inline void Delete(string const & key) { StringStorage::Instance().DeleteKeyAndValue(key); }
inline void Clear() { StringStorage::Instance().Clear(); }

/// Use this function for running some stuff once according to date.
/// @param[in]  date  Current date in format yymmdd.
bool IsFirstLaunchForDate(int date);
}

namespace marketing
{
class Settings : public platform::StringStorageBase
{
public:
  template <class Value>
  static void Set(string const & key, Value const & value)
  {
    Instance().SetValue(key, settings::ToString(value));
  }

  template <class Value>
  WARN_UNUSED_RESULT static bool Get(string const & key, Value & outValue)
  {
    string strVal;
    return Instance().GetValue(key, strVal) && settings::FromString(strVal, outValue);
  }

private:
  static Settings & Instance();
  Settings();
};
}  // namespace marketing
